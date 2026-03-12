#include "radio/radio_controller.h"
namespace MiniCore {
RadioController::RadioController(IRadio& radio, ITimer& timer, const CarrierSenseConfig& config)
    : radio_(radio)
    , timer_(timer)
    , config_(config)
{
}
void RadioController::setEventHandler(IRadioControllerEvents* handler) {
    eventHandler_ = handler;
}
void RadioController::setRxEventHandler(IRadioEvents* handler) {
    rxEventHandler_ = handler;
}
Status RadioController::transmitWithCS(const uint8_t* data, uint8_t size) {
    if (isBusy()) {
        return ErrorCode::Busy;
    }
    if (size > Config::MAX_MTU_SIZE) {
        return ErrorCode::BufferTooSmall;
    }
    if (size > 0 && data == nullptr) {
        return ErrorCode::InvalidParameter;
    }
    pendingData_ = data;
    pendingSize_ = size;
    retryCount_ = 0;
    startCarrierSenseCheck();
    return Status();
}
void RadioController::process(uint32_t currentTimeMs) {
    if (state_ != RadioControllerState::AwaitingRetry) {
        return;
    }
    int32_t diff = static_cast<int32_t>(currentTimeMs - retryScheduledTime_);
    if (diff >= 0) {
        startCarrierSenseCheck();
    }
}
bool RadioController::isBusy() const {
    return state_ != RadioControllerState::Idle;
}
void RadioController::enableCarrierSense(bool enable) {
    carrierSenseEnabled_ = enable;
}
void RadioController::configureCarrierSense(LoraBandwidth bandwidth) {
    carrierSenseEnabled_ = true;
    noiseFloorEstimator_ = NoiseFloorEstimator(NoiseFloorEstimatorConfig::forBandwidth(bandwidth));
}
void RadioController::updateNoiseFloor() {
    if (carrierSenseEnabled_) {
        noiseFloorEstimator_.addSample(radio_.readInstantRssi());
    }
}
bool RadioController::isNoiseFloorValid() const {
    return noiseFloorEstimator_.isValid();
}
int16_t RadioController::getNoiseFloor() const {
    return noiseFloorEstimator_.getNoiseFloor();
}
int16_t RadioController::getThreshold() const {
    return noiseFloorEstimator_.getThreshold();
}
bool RadioController::isCarrierDetected() {
    if (!carrierSenseEnabled_ || !noiseFloorEstimator_.isValid()) {
        return false;
    }
    return noiseFloorEstimator_.isAboveThreshold(radio_.readInstantRssi());
}
bool RadioController::checkCarrierSense() {
    return isCarrierDetected();
}
void RadioController::notifyChannelBusy() {
    if (eventHandler_) {
        eventHandler_->onChannelBusy();
    }
}
void RadioController::notifyTxComplete(bool success) {
    if (eventHandler_) {
        eventHandler_->onTransmitComplete(success);
    }
}
void RadioController::notifyRxDone(const RxPacket& packet) {
    if (rxEventHandler_) {
        rxEventHandler_->onRxDone(packet);
    }
}
void RadioController::notifyRxTimeout() {
    if (rxEventHandler_) {
        rxEventHandler_->onRxTimeout();
    }
}
void RadioController::notifyRxError() {
    if (rxEventHandler_) {
        rxEventHandler_->onRxError();
    }
}
void RadioController::startCarrierSenseCheck() {
    if (checkCarrierSense()) {
        notifyChannelBusy();
        ++retryCount_;
        if (retryCount_ > config_.maxRetries) {
            performTransmit();
        } else {
            scheduleRetry(timer_.millis());
        }
        return;
    }
    performTransmit();
}
void RadioController::performTransmit() {
    state_ = RadioControllerState::Transmitting;
    radio_.send(pendingData_, pendingSize_);
}
void RadioController::scheduleRetry(uint32_t currentTimeMs) {
    state_ = RadioControllerState::AwaitingRetry;
    retryScheduledTime_ = currentTimeMs + config_.retryDelayMs;
}
void RadioController::reset() {
    state_ = RadioControllerState::Idle;
    pendingData_ = nullptr;
    pendingSize_ = 0;
    retryCount_ = 0;
    retryScheduledTime_ = 0;
}
void RadioController::onRxDone(const RxPacket& packet) {
    notifyRxDone(packet);
}
void RadioController::onRxTimeout() {
    notifyRxTimeout();
}
void RadioController::onRxError() {
    notifyRxError();
}
void RadioController::onTxDone() {
    if (state_ == RadioControllerState::Transmitting) {
        reset();
        notifyTxComplete(true);
    }
}
void RadioController::onTxTimeout() {
    if (state_ == RadioControllerState::Transmitting) {
        reset();
        notifyTxComplete(false);
    }
}
}
