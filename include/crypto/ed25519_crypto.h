#pragma once
#include "hal/i_crypto.h"
namespace MiniCore {
class Ed25519Crypto : public ICrypto {
public:
    Status createKeypair(
        PublicKey& publicKey,
        PrivateKey& privateKey,
        const Seed& seed) override;
        Status derivePublicKey(
        PublicKey& publicKey,
        const PrivateKey& privateKey) override;
        Status sign(
        uint8_t* signature,
        const uint8_t* message,
        size_t messageLen,
        const PublicKey& publicKey,
        const PrivateKey& privateKey) override;
};
}
