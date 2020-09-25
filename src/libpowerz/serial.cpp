#include "serial.h"

#include <fcntl.h>
#include <fmt/format.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <chrono>

namespace powerz {

    std::string SystemError::ToString() {
        if (err_no >= 0) {
            return fmt::format("{} (errno={}, {})", msg, err_no, strerror(err_no));
        } else {
            return msg;
        }
    }

    std::variant<Serial, SystemError> Serial::Connect(std::string_view tty_dev_view) {
        std::string tty_dev{tty_dev_view};
        Serial ret{};

        errno = 0;
        int fd = open(tty_dev.c_str(), O_RDWR | O_NONBLOCK);
        if (fd < 0) {
            auto e = errno;
            return SystemError(fmt::format("failed to open device {}", tty_dev), e);
        }
        ret.fd_cleanup_ = RAIIHolder{
                [fd]() { close(fd); }
        };
        ret.fd_ = fd;

        termios term_opt{};
        cfmakeraw(&term_opt);
        cfsetospeed(&term_opt, B9600);
        cfsetispeed(&term_opt, B9600);

        if (0 != tcsetattr(fd, TCSANOW, &term_opt)) {
            return SystemError("tcsetattr() failed", errno);
        }

        termios term_opt_readback{};
        if (0 != tcgetattr(fd, &term_opt_readback)) {
            return SystemError("tcgetattr() failed", errno);
        }

        if (0 != memcmp(&term_opt, &term_opt_readback, sizeof(term_opt))) {
            return SystemError("tty attribute verification failed");
        }

        return ret;
    }


    std::optional<SystemError>
    Serial::Command(std::string_view cmd, void *buf, size_t reply_length, uint64_t timeout_us) {
        auto now = std::chrono::high_resolution_clock::now;

        errno = 0;
        auto written = write(fd_, cmd.data(), cmd.size());
        if (written != cmd.size()) {
            auto e = errno;
            return SystemError(fmt::format("write(fd={},cmd=\"{}\") returned {}", fd_, cmd, written), e);
        }
        auto etime = now() + std::chrono::microseconds(timeout_us);

        size_t total_red = 0;
        constexpr size_t TMP_BUF_SIZE = 4096;
        char tbuf[TMP_BUF_SIZE];
        while (total_red < reply_length) {
            usleep(1000);
            errno = 0;
            auto red = read(fd_, tbuf, TMP_BUF_SIZE);
            if (red <= 0) {
                if (errno == EAGAIN) {
                    if (timeout_us == 0) continue;
                    if (now() > etime)
                        return SystemError(fmt::format(
                                "expecting {} bytes but only {} bytes are received in {} us.",
                                reply_length, total_red, timeout_us));
                    continue;
                }
                return SystemError("read() failed", errno);
            }
            if (total_red + red > reply_length) {
                return SystemError(fmt::format("read() length error, expecting {} but received {}",
                                               reply_length, total_red + red));
            }
            memcpy(static_cast<char *>(buf) + total_red, tbuf, red);
            total_red += red;
        }

        return {};
    }

}
