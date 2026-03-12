#include <unity.h>
#include "crypto/crypto_types.h"
#include "crypto/ed25519_crypto.h"
using namespace MiniCore;
void test_ed25519_creates_keypair() {
    Ed25519Crypto crypto;
    Seed seed{};
    for (size_t i = 0; i < SEED_SIZE; ++i) {
        seed.bytes[i] = static_cast<uint8_t>(i + 1);
    }
        PublicKey pubKey{};
    PrivateKey privKey{};
        auto result = crypto.createKeypair(pubKey, privKey, seed);
    TEST_ASSERT_TRUE(result.isOk());
        bool pubNonZero = false;
    bool privNonZero = false;
    for (size_t i = 0; i < PUB_KEY_SIZE; ++i) {
        if (pubKey.bytes[i] != 0) pubNonZero = true;
    }
    for (size_t i = 0; i < PRV_KEY_SIZE; ++i) {
        if (privKey.bytes[i] != 0) privNonZero = true;
    }
    TEST_ASSERT_TRUE(pubNonZero);
    TEST_ASSERT_TRUE(privNonZero);
}
void test_ed25519_same_seed_same_keys() {
    Ed25519Crypto crypto;
    Seed seed{};
    for (size_t i = 0; i < SEED_SIZE; ++i) {
        seed.bytes[i] = static_cast<uint8_t>(i + 42);
    }
        PublicKey pub1{}, pub2{};
    PrivateKey priv1{}, priv2{};
        crypto.createKeypair(pub1, priv1, seed);
    crypto.createKeypair(pub2, priv2, seed);
        TEST_ASSERT_EQUAL_MEMORY(pub1.bytes, pub2.bytes, PUB_KEY_SIZE);
    TEST_ASSERT_EQUAL_MEMORY(priv1.bytes, priv2.bytes, PRV_KEY_SIZE);
}
void test_ed25519_derives_public_key() {
    Ed25519Crypto crypto;
    Seed seed{};
    for (size_t i = 0; i < SEED_SIZE; ++i) {
        seed.bytes[i] = static_cast<uint8_t>(i + 99);
    }
        PublicKey pubFromCreate{};
    PrivateKey privKey{};
    crypto.createKeypair(pubFromCreate, privKey, seed);
        PublicKey pubFromDerive{};
    auto result = crypto.derivePublicKey(pubFromDerive, privKey);
        TEST_ASSERT_TRUE(result.isOk());
    TEST_ASSERT_EQUAL_MEMORY(pubFromCreate.bytes, pubFromDerive.bytes, PUB_KEY_SIZE);
}
