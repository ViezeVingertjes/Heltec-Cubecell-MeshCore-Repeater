#include "crypto/ed25519_crypto.h"
#define ED25519_NO_SEED 1
extern "C" {
#include <ed_25519.h>
}
namespace MiniCore {
Status Ed25519Crypto::createKeypair(
    PublicKey& publicKey,
    PrivateKey& privateKey,
    const Seed& seed)
{
    ed25519_create_keypair(publicKey.bytes, privateKey.bytes, seed.bytes);
    return {};
}
Status Ed25519Crypto::derivePublicKey(
    PublicKey& publicKey,
    const PrivateKey& privateKey)
{
    ed25519_derive_pub(publicKey.bytes, privateKey.bytes);
    return {};
}
Status Ed25519Crypto::sign(
    uint8_t* signature,
    const uint8_t* message,
    size_t messageLen,
    const PublicKey& publicKey,
    const PrivateKey& privateKey)
{
    ed25519_sign(signature, message, messageLen, publicKey.bytes, privateKey.bytes);
    return {};
}
}
