#pragma once
#include "crypto/crypto_types.h"
#include "core/result.h"
namespace MiniCore {
class ICrypto {
public:
    virtual ~ICrypto() = default;
        virtual Status createKeypair(
        PublicKey& publicKey,
        PrivateKey& privateKey,
        const Seed& seed) = 0;
        virtual Status derivePublicKey(
        PublicKey& publicKey,
        const PrivateKey& privateKey) = 0;
        virtual Status sign(
        uint8_t* signature,
        const uint8_t* message,
        size_t messageLen,
        const PublicKey& publicKey,
        const PrivateKey& privateKey) = 0;
};
}
