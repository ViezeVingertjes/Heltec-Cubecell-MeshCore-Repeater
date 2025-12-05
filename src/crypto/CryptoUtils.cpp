#include "CryptoUtils.h"
#include "AES128.h"
#include "SHA256.h"

namespace MeshCrypto {

void CryptoUtils::sha256(uint8_t *hash, size_t hashLen, const uint8_t *msg,
                         size_t msgLen) {
  SHA256 sha;
  sha.update(msg, msgLen);
  uint8_t full[32];
  sha.finalize(full);
  memcpy(hash, full, hashLen);
}

int CryptoUtils::encrypt(const uint8_t *sharedSecret, uint8_t *dest,
                         const uint8_t *src, int srcLen) {
  AES128 aes;
  aes.setKey(sharedSecret);

  uint8_t *dp = dest;
  const uint8_t *sp = src;
  int len = srcLen;

  while (len >= 16) {
    aes.encryptBlock(dp, sp);
    dp += 16;
    sp += 16;
    len -= 16;
  }

  if (len > 0) {
    uint8_t tmp[16];
    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp, sp, len);
    aes.encryptBlock(dp, tmp);
    dp += 16;
  }

  return dp - dest;
}

int CryptoUtils::decrypt(const uint8_t *sharedSecret, uint8_t *dest,
                         const uint8_t *src, int srcLen) {
  if (srcLen % 16 != 0) {
    return 0;
  }
  AES128 aes;
  aes.setKey(sharedSecret);

  uint8_t *dp = dest;
  const uint8_t *sp = src;
  const uint8_t *end = src + srcLen;

  while (sp < end) {
    aes.decryptBlock(dp, sp);
    dp += 16;
    sp += 16;
  }

  return srcLen;
}

int CryptoUtils::encryptThenMAC(const uint8_t *sharedSecret, uint8_t *dest,
                                const uint8_t *src, int srcLen) {
  uint8_t *cipher = dest + MeshCoreCompat::CIPHER_MAC_SIZE;
  int encLen = encrypt(sharedSecret, cipher, src, srcLen);

  uint8_t mac[32];
  SHA256::hmac(sharedSecret, MeshCoreCompat::PUB_KEY_SIZE, cipher, encLen, mac);
  memcpy(dest, mac, MeshCoreCompat::CIPHER_MAC_SIZE);

  return MeshCoreCompat::CIPHER_MAC_SIZE + encLen;
}

int CryptoUtils::MACThenDecrypt(const uint8_t *sharedSecret, uint8_t *dest,
                                const uint8_t *src, int srcLen) {
  if (srcLen <= MeshCoreCompat::CIPHER_MAC_SIZE) {
    return 0;
  }

  const uint8_t *cipher = src + MeshCoreCompat::CIPHER_MAC_SIZE;
  int cipherLen = srcLen - MeshCoreCompat::CIPHER_MAC_SIZE;

  uint8_t mac[32];
  SHA256::hmac(sharedSecret, MeshCoreCompat::PUB_KEY_SIZE, cipher, cipherLen,
               mac);
  if (memcmp(mac, src, MeshCoreCompat::CIPHER_MAC_SIZE) != 0) {
    return 0;
  }

  return decrypt(sharedSecret, dest, cipher, cipherLen);
}

static int base64Value(char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  if (c == '=')
    return 0;
  return -1;
}

int base64Decode(const char *input, uint8_t *output, size_t maxLen) {
  size_t len = strlen(input);
  size_t outIndex = 0;
  uint32_t buffer = 0;
  int bitsCollected = 0;

  for (size_t i = 0; i < len; ++i) {
    char c = input[i];
    if (c == '=') {
      break;
    }

    int val = base64Value(c);
    if (val < 0) {
      continue;
    }

    buffer = (buffer << 6) | val;
    bitsCollected += 6;
    if (bitsCollected >= 8) {
      bitsCollected -= 8;
      if (outIndex >= maxLen) {
        return outIndex;
      }
      output[outIndex++] = static_cast<uint8_t>(buffer >> bitsCollected);
      buffer &= (1 << bitsCollected) - 1;
    }
  }

  return static_cast<int>(outIndex);
}

} // namespace MeshCrypto

