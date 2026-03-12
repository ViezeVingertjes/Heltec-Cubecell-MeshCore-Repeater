#pragma once
#include "hal/i_crypto.h"
#include "core/config.h"
#include <cstring>
namespace MiniCore::Mocks {
class MockCrypto : public ICrypto {
public:
    bool createKeypairCalled{false};
    bool derivePublicKeyCalled{false};
    bool signCalled{false};
    Status createKeypairResult{};
    Status derivePublicKeyResult{};
    Status signResult{};
    uint8_t signaturePattern{0x00};
        Status createKeypair(
        PublicKey& publicKey,
        PrivateKey& privateKey,
        const Seed& seed) override 
    {
        createKeypairCalled = true;
                if (createKeypairResult.isError()) {
            return createKeypairResult;
        }
                for (size_t i = 0; i < PRV_KEY_SIZE; ++i) {
            privateKey.bytes[i] = seed.bytes[i % SEED_SIZE] ^ static_cast<uint8_t>(i);
        }
                for (size_t i = 0; i < PUB_KEY_SIZE; ++i) {
            publicKey.bytes[i] = privateKey.bytes[i] ^ 0xFF;
        }
                return {};
    }
        Status derivePublicKey(
        PublicKey& publicKey,
        const PrivateKey& privateKey) override 
    {
        derivePublicKeyCalled = true;
                if (derivePublicKeyResult.isError()) {
            return derivePublicKeyResult;
        }
                for (size_t i = 0; i < PUB_KEY_SIZE; ++i) {
            publicKey.bytes[i] = privateKey.bytes[i] ^ 0xFF;
        }
                return {};
    }
        Status sign(
        uint8_t* signature,
        const uint8_t*,
        size_t,
        const PublicKey&,
        const PrivateKey&) override
    {
        signCalled = true;
                if (signResult.isError()) {
            return signResult;
        }
                std::memset(signature, signaturePattern, Config::SIGNATURE_SIZE);
        return {};
    }
        void setSignaturePattern(uint8_t pattern) {
        signaturePattern = pattern;
    }
        void reset() {
        createKeypairCalled = false;
        derivePublicKeyCalled = false;
        signCalled = false;
        createKeypairResult = {};
        derivePublicKeyResult = {};
        signResult = {};
        signaturePattern = 0x00;
    }
};
}
