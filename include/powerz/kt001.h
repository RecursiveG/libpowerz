#ifndef LIBPOWERZ_KT001_H
#define LIBPOWERZ_KT001_H

#include <array>
#include <cinttypes>
#include <memory>
#include <optional>
#include <string>

#include "serial.h"

namespace powerz {

struct MeterReading {
    float voltage_v;  // Volt
    float current_a;  // Ampere
    float power_w;    // Watt
    float volt_dplus_v;
    float volt_dminus_v;
} __attribute__((packed));
static_assert(sizeof(MeterReading) == 20);

enum RecordIndex {
    RECORD_0 = 0,
    RECORD_1,
    RECORD_2,
    RECORD_3,
};

class KT001 {
  public:
    struct Screenshot {
        uint32_t width;   // pixel
        uint32_t height;  // pixel
        // a series of R,G,B data in that order.
        // Starting from top-left corner, row-major.
        // {255,255,255} is white.
        std::unique_ptr<uint8_t[]> data;
    };

    explicit KT001(Serial ser);
    std::optional<std::string> RawCommand(std::string_view cmd,
                                          SystemError* err = nullptr) {
        return serial_.UnboundedCommand(cmd, err);
    }

    // Return empty if handshake is successful.
    std::optional<SystemError> Handshake();

    std::optional<std::string> GetFwVersion(SystemError* err = nullptr);
    std::optional<MeterReading> GetMeterReading(SystemError* err = nullptr);
    std::optional<std::array<bool, 4>> GetRecordExistence(
        SystemError* err = nullptr);
    std::optional<bool> GetRecordExistence(RecordIndex idx,
                                           SystemError* err = nullptr);
    std::optional<Screenshot> GetScreenshot(SystemError* err = nullptr);

  private:
    Serial serial_;
};
}  // namespace powerz

#endif  // LIBPOWERZ_KT001_H
