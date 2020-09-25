#ifndef LIBPOWERZ_KT001_H
#define LIBPOWERZ_KT001_H

#include "serial.h"
#include <cstring>
#include <fmt/format.h>

namespace powerz {
    class KT001 {
    public:
        explicit KT001(Serial ser) : serial_(std::move(ser)) {}

        // return true if handshake success
        std::optional<SystemError> Handshake() {
            char buf[6];
            auto err = serial_.Command("This is control", buf, 6);
            if (err) return err;
            if (0 != memcmp(buf, "Roger\0", 6))
                return SystemError(fmt::format(
                        "handshake reply unexpected: {}", buf
                ));
            return {};
        }

        std::variant<std::string, SystemError> GetFwVersion() {
            char buf[7];
            auto err = serial_.Command("Get FW Version", buf, 7);
            if (err) return *err;
            return std::string(buf, 7);
        }

    private:
        Serial serial_;
    };
}

#endif //LIBPOWERZ_KT001_H
