#pragma once
#ifdef NATIVE_BUILD
    #include <cstdint>
    #include <cstddef>
    using millis_t = uint32_t;
    inline millis_t millis_native() { return 0; }
    #define millis() millis_native()
#else
    #include <Arduino.h>
    using millis_t = decltype(millis());
#endif
