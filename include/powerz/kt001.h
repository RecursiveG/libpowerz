#ifndef LIBPOWERZ_KT001_H
#define LIBPOWERZ_KT001_H

#include <optional>
#include <string>

#include "serial.h"

namespace powerz {
class KT001 {
  public:
    explicit KT001(Serial ser);
    std::optional<std::string> RawCommand(std::string_view cmd,
                                          SystemError* err = nullptr) {
        return serial_.UnboundedCommand(cmd, err);
    }

    // Return empty if handshake is successful.
    std::optional<SystemError> Handshake();

    std::optional<std::string> GetFwVersion(SystemError* err = nullptr);

  private:
    Serial serial_;
};
}  // namespace powerz

#endif  // LIBPOWERZ_KT001_H
