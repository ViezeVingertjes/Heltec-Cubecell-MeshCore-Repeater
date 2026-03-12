#include "app/application.h"
#include "core/version.h"
#include "core/config.h"
#include "hal/i_log.h"
#include "util/secure_wipe.h"
#include "crypto/identity_manager.h"
#include "crypto/ed25519_crypto.h"
#include "packet/packet.h"
#include "discovery/discovery.h"
#include "advert/advert.h"
#include "routing/packet_router.h"
#include "radio/radio_controller.h"
#include "power/power_save.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#ifdef __asr650x__
#include <Arduino.h>
#endif
namespace MiniCore {
namespace {
using Config::SERIAL_BAUD_RATE;
using Config::STARTUP_DELAY_MS;
using Config::NOISE_FLOOR_SAMPLE_INTERVAL_MS;
using Config::NOISE_FLOOR_LOG_INTERVAL_MS;
using Config::DISCOVER_RESP_DELAY_BASE_MS;
using Config::DISCOVER_RESP_DELAY_SPREAD_MS;
using Config::ADVERT_ENQUEUE_DELAY_MAX_MS;
struct TxEventHandlerImpl : IRadioControllerEvents {
    Application* app{nullptr};
    TxEntry* pendingEntry{nullptr};
    void onTransmitComplete(bool success) override {
        if (app == nullptr) return;
        auto c = app->context();
        if (pendingEntry != nullptr) {
            c.txQueue.release(pendingEntry);
            pendingEntry = nullptr;
        }
        if (success) {
            app->logInfo("[TX] complete q:%u\r\n", c.txQueue.count());
        } else {
            app->logInfo("[TX] failed (CS retries: %u)\r\n", c.csRetryCount);
        }
        c.csRetryCount = 0;
    }
    void onChannelBusy() override {
        if (app == nullptr) return;
        auto c = app->context();
        ++c.csRetryCount;
        if (c.radioController != nullptr && c.radioController->isNoiseFloorValid()) {
            app->logInfo("[CS] busy (rssi>%d), retry %u/%u\r\n",
                         c.radioController->getThreshold(),
                         c.csRetryCount, Config::CS_MAX_RETRIES);
        } else {
            app->logInfo("[CS] busy (uncalibrated), retry %u/%u\r\n",
                         c.csRetryCount, Config::CS_MAX_RETRIES);
        }
    }
};
struct PacketRouterEventHandlerImpl : IPacketRouterEvents {
    Application* app{nullptr};
    void onAdvertReceived(uint8_t senderHash, uint32_t timestamp) override {
        if (app == nullptr) return;
        auto c = app->context();
        uint32_t localTime = c.device.timer().millis() / 1000;
        c.timeSync.addSample(senderHash, timestamp, localTime);
    }
    void onDiscoveryRequest(const DiscoverRequest& request, int8_t snr) override {
        if (app == nullptr) return;
        auto c = app->context();
        if (!c.hasIdentity) return;
        if (!matchesNodeType(request, ADV_TYPE_REPEATER)) {
            app->logInfo("[RX] CTL cmd:0x%02X\r\n", Config::CTL_TYPE_NODE_DISCOVER_REQ);
            return;
        }
        uint8_t respPayload[DISCOVER_RESP_MAX_SIZE];
        uint8_t respLen = 0;
        int8_t scaledSnr = static_cast<int8_t>(snr * 4);
        if (createDiscoverResponse(c.localIdentity, request.tag, scaledSnr,
                                    request.prefixOnly, respPayload, respLen).isOk()) {
            Packet respPkt;
            respPkt.header = Packet::makeHeader(RouteType::Direct, PayloadType::Control);
            respPkt.pathLen = 0;
            respPkt.payloadLen = respLen;
            std::memcpy(respPkt.payload, respPayload, respLen);
            uint32_t delay = DISCOVER_RESP_DELAY_BASE_MS + (c.selfHash % DISCOVER_RESP_DELAY_SPREAD_MS);
            if (c.txQueue.enqueuePacket(respPkt, c.device.timer().millis() + delay, 0).isOk()) {
                app->logInfo("[RX] CTL discover -> resp @+%lu\r\n", static_cast<unsigned long>(delay));
            }
        }
    }
    void onPacketForward(const Packet& packet, uint32_t delayMs, uint8_t priority) override {
        if (app == nullptr) return;
        auto c = app->context();
        uint32_t scheduledTime = c.device.timer().millis() + delayMs;
        if (c.txQueue.enqueuePacket(packet, scheduledTime, priority).isOk()) {
            if (c.powerSaveController != nullptr) {
                c.powerSaveController->onPacketRepeated();
            }
            char label[ROUTE_PAYLOAD_LABEL_SIZE];
            formatRouteAndPayloadLabel(packet, label, sizeof(label));
            app->logInfo("[Q] %s p:%u @+%lu\r\n", label, packet.pathLen, static_cast<unsigned long>(delayMs));
        }
    }
    void onPacketDroppedDuplicate(const Packet& packet) override {
        if (app == nullptr) return;
        char label[ROUTE_PAYLOAD_LABEL_SIZE];
        formatRouteAndPayloadLabel(packet, label, sizeof(label));
        app->logInfo("[DUP] %s p:%u (not transmitted)\r\n", label, packet.pathLen);
    }
};
struct RadioEventHandlerImpl : IRadioEvents {
    Application* app{nullptr};
    void onRxDone(const RxPacket& rxPacket) override {
        if (app == nullptr) return;
        auto c = app->context();
        ++c.packetCount;
        Packet pkt;
        auto status = decodePacket(rxPacket.data, rxPacket.size, pkt);
        if (status.isOk()) {
            logPacketReceived(pkt, rxPacket);
            if (c.packetRouter != nullptr) {
                c.packetRouter->processPacket(pkt, rxPacket);
            }
        } else {
            logDecodeError(rxPacket, status);
        }
    }
    void onRxTimeout() override {}
    void onRxError() override {}
    void onTxDone() override {}
    void onTxTimeout() override {}
private:
    void logPacketReceived(const Packet& pkt, const RxPacket& rxPacket) {
        if (app == nullptr) return;
        auto c = app->context();
        char stats[PACKET_STATS_LABEL_SIZE];
        formatPacketStats(pkt, stats, sizeof(stats));
        if (pkt.getPayloadType() == PayloadType::Advert) {
            uint32_t timestamp = 0;
            uint8_t senderHash = 0;
            if (extractAdvertTimestamp(pkt.payload, pkt.payloadLen, timestamp, senderHash).isOk()) {
                if (c.timeSync.hasConsensus()) {
                    app->logInfo("[RX] %s t:%lu rssi:%d (sync:%d off:%+ld)\r\n",
                                 stats, static_cast<unsigned long>(timestamp), rxPacket.rssi,
                                 c.timeSync.uniqueSenderCount(), static_cast<long>(c.timeSync.getOffset()));
                } else {
                    app->logInfo("[RX] %s t:%lu rssi:%d (%d/8 off:%+ld)\r\n",
                                 stats, static_cast<unsigned long>(timestamp), rxPacket.rssi,
                                 c.timeSync.uniqueSenderCount(), static_cast<long>(c.timeSync.getOffset()));
                }
            }
        } else {
            app->logInfo("[RX] %s rssi:%d snr:%d\r\n", stats, rxPacket.rssi, rxPacket.snr);
        }
    }
    void logDecodeError(const RxPacket& rxPacket, Status status) {
        auto partialInfo = extractPartialPacketInfo(rxPacket.data, rxPacket.size, status);
        if (partialInfo.hasHeader) {
            app->logInfo("[RX] %uB (decode err) %s:%s p:%u err:%d rssi:%d snr:%d\r\n",
                         rxPacket.size, routeTypeName(partialInfo.routeType),
                         payloadTypeName(partialInfo.payloadType), partialInfo.pathLen,
                         static_cast<int>(partialInfo.decodeError), rxPacket.rssi, rxPacket.snr);
        } else {
            app->logInfo("[RX] %uB (decode err: no header) err:%d rssi:%d snr:%d\r\n",
                         rxPacket.size, static_cast<int>(partialInfo.decodeError),
                         rxPacket.rssi, rxPacket.snr);
        }
    }
};
}
struct ApplicationHandlers {
    std::unique_ptr<TxEventHandlerImpl> tx;
    std::unique_ptr<PacketRouterEventHandlerImpl> router;
    std::unique_ptr<RadioEventHandlerImpl> radio;
};
void Application::logInfo(const char* fmt, ...) {
    char buf[Config::LOG_LINE_MAX_SIZE];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    device_.log().log(LogLevel::Info, buf);
}
void Application::logError(const char* fmt, ...) {
    char buf[Config::LOG_LINE_MAX_SIZE];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    device_.log().log(LogLevel::Error, buf);
}
Application::Application(IDevice& device)
    : device_(device)
    , packetHistory_()
    , repeaterConfig_{.enabled = true,
                      .maxFloodPath = Config::DEFAULT_MAX_FLOOD_PATH,
                      .dropRepeaterAdverts = Config::DROP_REPEATER_ADVERTS}
    , repeater_(packetHistory_, repeaterConfig_)
    , crypto_(std::make_unique<Ed25519Crypto>())
    , handlers_(std::make_unique<ApplicationHandlers>())
{
    handlers_->tx = std::make_unique<TxEventHandlerImpl>();
    handlers_->router = std::make_unique<PacketRouterEventHandlerImpl>();
    handlers_->radio = std::make_unique<RadioEventHandlerImpl>();
    handlers_->tx->app = this;
    handlers_->router->app = this;
    handlers_->radio->app = this;
    txHandler_ = handlers_->tx.get();
    routerHandler_ = handlers_->router.get();
    radioHandler_ = handlers_->radio.get();
}
Application::~Application() {
    if (hasIdentity_) {
        secureWipe(localIdentity_.privateKey.bytes, Config::PRV_KEY_SIZE);
    }
    if (radioController_ != nullptr) {
        device_.radio().setEventHandler(nullptr);
    }
    powerSaveController_.reset();
    radioController_.reset();
    packetRouter_.reset();
    txHandler_ = nullptr;
    routerHandler_ = nullptr;
    radioHandler_ = nullptr;
    handlers_.reset();
    crypto_.reset();
}
void Application::init() {
#ifdef __asr650x__
    Serial.begin(SERIAL_BAUD_RATE);
    delay(STARTUP_DELAY_MS);
#endif
    logInfo("MiniCore v%d.%d.%d [%s]\r\n",
            FIRMWARE_VERSION.major, FIRMWARE_VERSION.minor, FIRMWARE_VERSION.patch,
            device_.name());
    auto initResult = device_.init();
    if (!initResult.isOk()) {
        logError("[E] Device init: %d\r\n", static_cast<int>(initResult.error()));
        return;
    }
#ifdef __asr650x__
    logInfo("Hold USER button for factory reset (3 sec)...\r\n");
    if (device_.checkFactoryResetRequest(3000)) {
        logInfo("[!] Factory reset - clearing identity...\r\n");
        auto resetStatus = device_.factoryReset();
        if (!resetStatus.isOk()) {
            logError("[E] Factory reset failed: %d\r\n", static_cast<int>(resetStatus.error()));
        } else {
            logInfo("[!] Reset complete. Rebooting...\r\n");
        }
        delay(1000);
        device_.reboot();
    }
#endif
    IdentityManager identityManager(device_.storage(), device_.rng(), *crypto_, device_.log());
    auto result = identityManager.loadOrCreate();
    if (result.isOk()) {
        localIdentity_ = result.value();
        hasIdentity_ = true;
        selfHash_ = localIdentity_.publicKey.bytes[0];
        snprintf(nodeName_, Config::NODE_NAME_BUF_SIZE, "Cubecell %02X", selfHash_);
        logInfo("Node: %s\r\n", nodeName_);
    } else {
        logError("[E] Identity: %d\r\n", static_cast<int>(result.error()));
    }
    uint8_t advertState[Config::ADVERT_STATE_STORAGE_SIZE];
    auto readStatus = device_.storage().readBlock(Config::IDENTITY_STORAGE_SIZE, advertState, sizeof(advertState));
    if (readStatus.isOk()) {
        advertScheduler_.loadFrom(advertState);
        if (advertScheduler_.lastAdvertTime() > 0) {
            logInfo("Advert: last=%lu\r\n", static_cast<unsigned long>(advertScheduler_.lastAdvertTime()));
        }
    }
    packetRouter_ = std::make_unique<PacketRouter>(repeater_);
    packetRouter_->setSelfHash(selfHash_);
    packetRouter_->setEventHandler(routerHandler_);
    auto& radio = device_.radio();
    auto config = RadioConfig::meshCoreDefaults();
    radio.init(config);
    radioController_ = std::make_unique<RadioController>(radio, device_.timer());
    radioController_->setEventHandler(txHandler_);
    radioController_->setRxEventHandler(radioHandler_);
    radioController_->configureCarrierSense(config.bandwidth);
    radio.setEventHandler(radioController_.get());
    radio.startReceive();
    powerSaveController_ = std::make_unique<PowerSaveController>(radio, device_.timer());
    powerSaveController_->setEnabled(Config::POWER_SAVE_ENABLED);
    logInfo("Radio: %.3fMHz SF%d CS:on PS:%s\r\n",
            config.frequencyHz / 1000000.0f, config.spreadingFactor,
            Config::POWER_SAVE_ENABLED ? "on" : "off");
}
void Application::loop() {
    device_.processLoop();
    if (powerSaveController_ != nullptr) {
        bool wasSleeping = powerSaveController_->isSleeping();
        powerSaveController_->process();
        if (wasSleeping && !powerSaveController_->isSleeping()) {
            logInfo("[PS] wake\r\n");
        } else if (!wasSleeping && powerSaveController_->isSleeping()) {
            logInfo("[PS] sleep\r\n");
        }
    }
    bool sleeping = powerSaveController_ != nullptr && powerSaveController_->isSleeping();
    uint32_t now = device_.timer().millis();
    if (!sleeping && hasIdentity_ && timeSync_.hasConsensus()) {
        uint32_t currentTime = timeSync_.adjustedTime(now / 1000);
        if (advertScheduler_.isDue(currentTime, true)) {
            Packet advertPkt;
            if (createAdvertPacket(localIdentity_, currentTime, nodeName_, *crypto_, advertPkt).isOk()) {
                uint32_t delay = selfHash_ % ADVERT_ENQUEUE_DELAY_MAX_MS;
                if (txQueue_.enqueuePacket(advertPkt, now + delay, 10).isOk()) {
                    advertScheduler_.markSent(currentTime);
                    uint8_t advertStateOut[Config::ADVERT_STATE_STORAGE_SIZE];
                    advertScheduler_.saveTo(advertStateOut);
                    auto writeStatus = device_.storage().writeBlock(Config::IDENTITY_STORAGE_SIZE,
                                                                   advertStateOut, sizeof(advertStateOut));
                    if (writeStatus.isOk()) {
                        auto commitStatus = device_.storage().commit();
                        if (!commitStatus.isOk()) {
                            logError("[E] Advert state commit: %d\r\n", static_cast<int>(commitStatus.error()));
                        }
                    } else {
                        logError("[E] Advert state write: %d\r\n", static_cast<int>(writeStatus.error()));
                    }
                    logInfo("[ADV] t=%lu @+%lu\r\n", static_cast<unsigned long>(currentTime), static_cast<unsigned long>(delay));
                }
            }
        }
    }
    if (!sleeping && radioController_ != nullptr) {
        radioController_->process(now);
        if (!radioController_->isBusy() && radioController_->isCarrierSenseEnabled()) {
            if (now - lastNoiseFloorSampleTime_ >= NOISE_FLOOR_SAMPLE_INTERVAL_MS) {
                lastNoiseFloorSampleTime_ = now;
                radioController_->updateNoiseFloor();
                if (!noiseFloorCalibrated_ && radioController_->isNoiseFloorValid()) {
                    noiseFloorCalibrated_ = true;
                    logInfo("[CS] calibrated: floor=%d thresh=%d\r\n",
                            radioController_->getNoiseFloor(), radioController_->getThreshold());
                }
                if (noiseFloorCalibrated_ && now - lastNoiseFloorLogTime_ > NOISE_FLOOR_LOG_INTERVAL_MS) {
                    lastNoiseFloorLogTime_ = now;
                    logInfo("[CS] floor=%d\r\n", radioController_->getNoiseFloor());
                }
            }
        }
        if (!radioController_->isBusy()) {
            auto* entry = txQueue_.getNextReady(now);
            if (entry != nullptr) {
                auto sendStatus = radioController_->transmitWithCS(entry->data, entry->length);
                if (sendStatus.isOk()) {
                    setTxHandlerPendingEntry(entry);
                    logInfo("[TX] %uB q:%u\r\n", entry->length, txQueue_.count() - 1);
                } else {
                    txQueue_.release(entry);
                    logError("[TX ERR] %uB err:%d\r\n", entry->length, static_cast<int>(sendStatus.error()));
                }
            }
        }
    }
    device_.idle();
}
void Application::setTxHandlerPendingEntry(TxEntry* entry) {
    auto* impl = static_cast<TxEventHandlerImpl*>(txHandler_);
    if (impl != nullptr) {
        impl->pendingEntry = entry;
    }
}
}
