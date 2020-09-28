#include "kt001.h"

#include <fmt/format.h>

#include <utility>

namespace powerz {
KT001::KT001(Serial ser) : serial_(std::move(ser)) {}

std::optional<SystemError> KT001::Handshake() {
    char buf[6];
    auto err = serial_.Command("This is control", buf, 6);
    if (err) return err;
    if (0 != memcmp(buf, "Roger\0", 6))
        return SystemError(fmt::format("handshake reply unexpected: {}", buf));
    return {};
}

std::optional<std::string> KT001::GetFwVersion(SystemError* err) {
    char buf[7];
    auto maybe_err = serial_.Command("Get FW Version", buf, 7);
    if (maybe_err) {
        if (err) *err = std::move(*maybe_err);
        return {};
    }
    return std::string(buf, 7);
}
}  // namespace powerz