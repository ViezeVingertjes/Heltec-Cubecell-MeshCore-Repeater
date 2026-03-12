#include "crypto/crypto_types.h"
namespace MiniCore {
bool LocalIdentity::isValid() const {
    for (size_t i = 0; i < PRV_KEY_SIZE; ++i) {
        if (privateKey.bytes[i] != 0) {
            return true;
        }
    }
    return false;
}
}
