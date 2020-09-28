#include <powerz/kt001.h>
#include <powerz/serial.h>

#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "hexdump.h"

using namespace std;
using namespace powerz;

template <typename T>
T unwrap(optional<T> maybe, SystemError *err) {
    if (maybe) return move(*maybe);
    cerr << err->ToString() << endl;
    exit(2);
}

void unwrap_inverse(optional<SystemError> maybe_err) {
    if (maybe_err) {
        cerr << maybe_err->ToString() << endl;
        exit(2);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <tty_device>" << endl;
        return 1;
    }
    string tty_dev = argv[1];
    cout << "USB Multimeter device: " << tty_dev << endl;

    SystemError sys_err{};
    KT001 kt001(unwrap(Serial::Connect(tty_dev, &sys_err), &sys_err));
    cout << "Device connected." << endl;

    unwrap_inverse(kt001.Handshake());
    cout << "Handshake successful." << endl;

    std::string fw_ver = unwrap(kt001.GetFwVersion(&sys_err), &sys_err);
    cout << "Firmware version: " << fw_ver << endl;

    while (true) {
        string cmd;
        cout << "cmd > ";
        cout.flush();
        getline(cin, cmd);
        if (cmd == "exit") break;

        auto maybe_reply = kt001.RawCommand(cmd, &sys_err);
        if (maybe_reply) {
            cout << "Received " << maybe_reply->length()
                 << " bytes of data:" << endl;
            hexdump(*maybe_reply, cout, false);
        } else {
            cout << sys_err.ToString() << endl;
        }
    }
    return 0;
}
