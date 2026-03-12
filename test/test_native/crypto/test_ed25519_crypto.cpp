#include <unity.h>
#include "core/config.h"
#include "crypto/crypto_types.h"
#include "crypto/ed25519_crypto.h"
#include <cstring>
using namespace MiniCore;
using Config::SIGNATURE_SIZE;
void test_ed25519_create_keypair_succeeds() {
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
void test_ed25519_same_seed_produces_same_keys() {
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
void test_ed25519_derive_public_key_matches_create() {
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
void test_ed25519_sign_succeeds() {
    Ed25519Crypto crypto;
    Seed seed{};
    for (size_t i = 0; i < SEED_SIZE; ++i) {
        seed.bytes[i] = static_cast<uint8_t>(i + 7);
    }
    PublicKey pubKey{};
    PrivateKey privKey{};
    crypto.createKeypair(pubKey, privKey, seed);
    const uint8_t message[] = "hello mesh";
    const size_t messageLen = sizeof(message) - 1;
    uint8_t signature[SIGNATURE_SIZE];
    auto result = crypto.sign(signature, message, messageLen, pubKey, privKey);
    TEST_ASSERT_TRUE(result.isOk());
    bool sigNonZero = false;
    for (size_t i = 0; i < SIGNATURE_SIZE; ++i) {
        if (signature[i] != 0) sigNonZero = true;
    }
    TEST_ASSERT_TRUE(sigNonZero);
}
void test_ed25519_sign_different_messages_different_signatures() {
    Ed25519Crypto crypto;
    Seed seed{};
    for (size_t i = 0; i < SEED_SIZE; ++i) {
        seed.bytes[i] = static_cast<uint8_t>(i + 11);
    }
    PublicKey pubKey{};
    PrivateKey privKey{};
    crypto.createKeypair(pubKey, privKey, seed);
    const uint8_t msg1[] = "msg1";
    const uint8_t msg2[] = "msg2";
    uint8_t sig1[SIGNATURE_SIZE], sig2[SIGNATURE_SIZE];
    crypto.sign(sig1, msg1, sizeof(msg1) - 1, pubKey, privKey);
    crypto.sign(sig2, msg2, sizeof(msg2) - 1, pubKey, privKey);
    TEST_ASSERT_FALSE(memcmp(sig1, sig2, SIGNATURE_SIZE) == 0);
}
