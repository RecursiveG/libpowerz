#ifndef LIBPOWERZ_SERIAL_H
#define LIBPOWERZ_SERIAL_H

#include <cinttypes>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace powerz {
class SystemError {
  public:
    SystemError() = default;
    SystemError(std::string_view msg, int err_no) : err_no(err_no), msg(msg) {}
    explicit SystemError(std::string_view msg) : err_no(-1), msg(msg) {}
    std::string ToString();

  private:
    int err_no;
    std::string msg;
};

class RAIIHolder {
  public:
    RAIIHolder() = default;

    explicit RAIIHolder(std::function<void()> cleanup)
        : cleanup_(std::move(cleanup)) {}

    ~RAIIHolder() {
        if (cleanup_ != nullptr) cleanup_();
    }

    RAIIHolder(const RAIIHolder &) = delete;

    RAIIHolder &operator=(const RAIIHolder &) = delete;

    RAIIHolder(RAIIHolder &&another) noexcept
        : cleanup_(std::exchange(another.cleanup_, nullptr)) {}

    RAIIHolder &operator=(RAIIHolder &&another) noexcept {
        std::swap(another.cleanup_, cleanup_);
        return *this;
    }

  private:
    std::function<void()> cleanup_;
};

class Serial {
  public:
    static std::optional<Serial> Connect(std::string_view tty_dev_view,
                                         SystemError *err = nullptr);

    // Issue `cmd` to the device and fill exactly `reply_length` bytes into
    // `buf` Error is returned if it returns extra data or `timeout_us` has
    // elapsed. `buf` is assumed to be at least `reply_length` bytes long. set
    // `timeout_us` to 0 for unlimited time.
    std::optional<SystemError> Command(std::string_view cmd, void *buf,
                                       size_t reply_length,
                                       uint64_t timeout_us = 0);
    std::optional<std::string> UnboundedCommand(std::string_view cmd,
                                                SystemError *err = nullptr,
                                                uint64_t wait_ms = 100,
                                                uint64_t timeout_s = 3);

  private:
    Serial() = default;

    int fd_;
    RAIIHolder fd_cleanup_;
};
}  // namespace powerz

#endif  // LIBPOWERZ_SERIAL_H
