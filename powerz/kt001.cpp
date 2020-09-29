#include "kt001.h"

#include <fmt/format.h>

#include <array>
#include <memory>
#include <utility>

namespace powerz {
namespace {

using namespace std;
using fmt::format;
bool CheckErrorAndAssign(optional<SystemError> err, SystemError* e) {
    if (err && e != nullptr) {
        *e = move(*err);
        return true;
    }
    return false;
}
}  // namespace

KT001::KT001(Serial ser) : serial_(move(ser)) {}

optional<SystemError> KT001::Handshake() {
    char buf[6];
    auto err = serial_.Command("This is control", buf, 6);
    if (err) return err;
    if (0 != memcmp(buf, "Roger\0", 6))
        return SystemError(format("handshake reply unexpected: {}", buf));
    return {};
}

optional<string> KT001::GetFwVersion(SystemError* err) {
    char buf[7];
    if (CheckErrorAndAssign(serial_.Command("Get FW Version", buf, 7), err)) {
        return {};
    }
    return string(buf, 7);
}

optional<MeterReading> KT001::GetMeterReading(SystemError* err) {
    MeterReading reading{};
    if (CheckErrorAndAssign(
            serial_.Command("Get Meter Data", &reading, sizeof(reading)), err))
        return {};
    return reading;
}

optional<array<bool, 4>> KT001::GetRecordExistence(SystemError* err) {
    array<bool, 4> ret{};
    uint8_t buf[4];
    if (CheckErrorAndAssign(serial_.Command("Get Ext Record", buf, 4), err)) {
        return {};
    }
    ret[RECORD_0] = buf[0];
    ret[RECORD_1] = buf[1];
    ret[RECORD_2] = buf[2];
    ret[RECORD_3] = buf[3];
    return ret;
}

optional<bool> KT001::GetRecordExistence(RecordIndex idx, SystemError* err) {
    if (idx < 0 || idx > 3) {
        *err = SystemError("index out of range");
        return {};
    }
    auto maybe_arr = GetRecordExistence(err);
    if (maybe_arr) {
        return (*maybe_arr)[idx];
    } else {
        return {};
    }
}

namespace {
uint8_t kColorTable[16][3] = {
    {248, 0, 0},     {0, 0, 248},     {0, 252, 0},     {0, 252, 248},
    {0, 0, 0},       {248, 252, 248}, {248, 0, 248},   {248, 208, 64},
    {88, 92, 88},    {248, 252, 0},   {0, 252, 120},   {248, 112, 112},
    {248, 252, 144}, {120, 208, 248}, {248, 164, 248}, {248, 92, 0}};
}

optional<KT001::Screenshot> KT001::GetScreenshot(SystemError* err) {
    constexpr size_t kReplySize = 0x2000;
    auto buf = make_unique<uint8_t[]>(kReplySize);
    if (CheckErrorAndAssign(
            serial_.Command("Get Screenshot", buf.get(), kReplySize, 1e6),
            err)) {
        return {};
    }
    auto get_index = [&buf](size_t pos) -> uint8_t {
        uint8_t byte = buf[pos / 2];
        if (pos % 2 == 1) {
            return byte & 0xF;
        } else {
            return byte >> 4;
        }
    };
    KT001::Screenshot ss{};
    ss.width = 0x80;
    ss.height = 0x80;
    ss.data = make_unique<uint8_t[]>(ss.width * ss.height * 3);
    size_t p = 0;
    for (int64_t pixel = ss.width * ss.height - 1; pixel >= 0; pixel--) {
        uint8_t idx = get_index(pixel);
        ss.data[p++] = kColorTable[idx][0];
        ss.data[p++] = kColorTable[idx][1];
        ss.data[p++] = kColorTable[idx][2];
    }
    return ss;
}

}  // namespace powerz
