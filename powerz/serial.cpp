#include "serial.h"

#include <fcntl.h>
#include <fmt/format.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <cstring>

namespace powerz {

std::string SystemError::ToString() {
    if (err_no >= 0) {
        return fmt::format("{} (errno={}, {})", msg, err_no, strerror(err_no));
    } else {
        return msg;
    }
}

std::optional<Serial> Serial::Connect(std::string_view tty_dev_view,
                                      SystemError *err) {
    auto DeclareErr = [err](SystemError e) -> std::optional<Serial> {
        if (err) *err = std::move(e);
        return {};
    };

    std::string tty_dev{tty_dev_view};
    Serial ret{};

    errno = 0;
    int fd = open(tty_dev.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        auto e = errno;
        return DeclareErr(
            {fmt::format("failed to open device {}", tty_dev), e});
    }
    ret.fd_cleanup_ = RAIIHolder{[fd]() { close(fd); }};
    ret.fd_ = fd;

    termios term_opt{};
    cfmakeraw(&term_opt);
    cfsetospeed(&term_opt, B9600);
    cfsetispeed(&term_opt, B9600);

    if (0 != tcsetattr(fd, TCSANOW, &term_opt)) {
        return DeclareErr({"tcsetattr() failed", errno});
    }

    termios term_opt_readback{};
    if (0 != tcgetattr(fd, &term_opt_readback)) {
        return DeclareErr({"tcgetattr() failed", errno});
    }

    if (0 != memcmp(&term_opt, &term_opt_readback, sizeof(term_opt))) {
        return DeclareErr(SystemError{"tty attribute verification failed"});
    }

    return ret;
}

std::optional<SystemError> Serial::Command(std::string_view cmd, void *buf,
                                           size_t reply_length,
                                           uint64_t timeout_us) {
    auto now = std::chrono::high_resolution_clock::now;

    errno = 0;
    auto written = write(fd_, cmd.data(), cmd.size());
    if (written != cmd.size()) {
        auto e = errno;
        return SystemError(fmt::format("write(fd={},cmd=\"{}\") returned {}",
                                       fd_, cmd, written),
                           e);
    }
    auto etime = now() + std::chrono::microseconds(timeout_us);

    size_t total_red = 0;
    constexpr size_t TMP_BUF_SIZE = 4096;
    char tbuf[TMP_BUF_SIZE];
    while (total_red < reply_length) {
        usleep(1000);  // don't poll that fast
        errno = 0;
        auto red = read(fd_, tbuf, TMP_BUF_SIZE);
        if (red <= 0) {
            if (errno == EAGAIN) {
                if (timeout_us == 0) continue;
                if (now() > etime)
                    return SystemError(
                        fmt::format("expecting {} bytes but only {} bytes are "
                                    "received in {} us.",
                                    reply_length, total_red, timeout_us));
                continue;
            }
            return SystemError("read() failed", errno);
        }
        if (total_red + red > reply_length) {
            return SystemError(
                fmt::format("read() length error, expecting {} but received {}",
                            reply_length, total_red + red));
        }
        memcpy(static_cast<char *>(buf) + total_red, tbuf, red);
        total_red += red;
    }

    return {};
}

std::optional<std::string> Serial::UnboundedCommand(std::string_view cmd,
                                                    SystemError *err,
                                                    uint64_t wait_ms,
                                                    uint64_t timeout_s) {
    auto written = write(fd_, cmd.data(), cmd.size());
    if (written != cmd.size()) {
        auto e = errno;
        if (err)
            *err = {fmt::format("write(fd={},cmd=\"{}\") returned {}", fd_, cmd,
                                written),
                    e};
        return {};
    }
    return WaitForSilence(err, wait_ms, timeout_s * 1000);
}

std::optional<std::string> Serial::WaitForSilence(SystemError *err,
                                                  uint64_t wait_ms,
                                                  uint64_t timeout_ms) {
    auto DeclareErr = [err](SystemError e) -> std::optional<std::string> {
        if (err) *err = std::move(e);
        return {};
    };
    auto now = std::chrono::high_resolution_clock::now;
    using std::chrono::milliseconds;

    std::string reply;
    constexpr size_t TMP_BUF_SIZE = 4096;
    char tbuf[TMP_BUF_SIZE];
    auto last_polled = now();
    while (true) {
        usleep(1000);  // don't poll that fast
        errno = 0;
        auto red = read(fd_, tbuf, TMP_BUF_SIZE);
        if (red <= 0) {
            if (errno == EAGAIN) {
                if (reply.empty()) {
                    if (now() > last_polled + milliseconds(timeout_ms)) {
                        return DeclareErr(SystemError{
                            fmt::format("No reply received in {} milliseconds.",
                                        timeout_ms)});
                    }
                } else {
                    if (now() > last_polled + milliseconds(wait_ms)) {
                        return reply;
                    }
                }
                continue;
            }
            return DeclareErr({"read() failed", errno});
        }
        reply += std::string{tbuf, static_cast<unsigned long>(red)};
        last_polled = now();
    }
}

}  // namespace powerz
