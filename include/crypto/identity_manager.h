#pragma once
#include "crypto/crypto_types.h"
#include "core/result.h"
#include "hal/i_storage.h"
#include "hal/i_rng.h"
#include "hal/i_log.h"
#include "hal/i_crypto.h"
namespace MiniCore {
class IdentityManager {
public:
    IdentityManager(IStorage& storage, IRng& rng, ICrypto& crypto, ILog& log);
        Result<LocalIdentity> loadOrCreate();
    Result<LocalIdentity> load();
    Status save(const LocalIdentity& identity);
    [[nodiscard]] bool hasIdentity() const;
        static size_t getStorageAddress();
    static size_t getStorageSize();
private:
    template<typename T>
    bool logIfError(const Result<T>& r, const char* msg) const {
        if (r.isError()) {
            log_.error(msg);
            return true;
        }
        return false;
    }
    bool logIfError(Status s, const char* msg) const {
        if (s.isError()) {
            log_.error(msg);
            return true;
        }
        return false;
    }
    IStorage& storage_;
    IRng& rng_;
    ICrypto& crypto_;
    ILog& log_;
        Result<LocalIdentity> generate();
    Status writeToStorage(const LocalIdentity& identity);
    Result<LocalIdentity> readFromStorage();
    [[nodiscard]] bool isStorageEmpty() const;
};
}
