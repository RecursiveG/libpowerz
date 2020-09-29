#include <powerz/kt001.h>
#include <powerz/serial.h>

#include <ctime>
#include <fstream>
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
        if (cmd == "exit") {
            break;
        } else if (cmd == "get_meter_data") {
            auto data = unwrap(kt001.GetMeterReading(&sys_err), &sys_err);
            printf("Voltage: %6.05fV\n", data.voltage_v);
            printf("Current: %6.05fA\n", data.current_a);
            printf("  Power: %6.05fW\n", data.power_w);
            printf("     D+: %6.05fV\n", data.volt_dplus_v);
            printf("     D-: %6.05fV\n", data.volt_dminus_v);
        } else if (cmd == "get_ext_record") {
            auto data = unwrap(kt001.GetRecordExistence(&sys_err), &sys_err);
            auto print = [&data](RecordIndex idx) {
                cout << "Record #" << idx
                     << (data[idx] ? " exists" : " not exists") << endl;
            };
            print(RECORD_0);
            print(RECORD_1);
            print(RECORD_2);
            print(RECORD_3);
        } else if (cmd == "get_screenshot") {
            KT001::Screenshot ss =
                unwrap(kt001.GetScreenshot(&sys_err), &sys_err);
            char time_str[80];
            time_t t = time(nullptr);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d-%H%M%S",
                     localtime(&t));
            string filename = string{time_str} + ".ppm";
            cout << "Writing to " << filename << endl;
            ofstream img(filename.c_str());
            img << "P3" << endl;
            img << ss.width << " " << ss.height << endl;
            img << "255" << endl;
            for (size_t y = 0; y < ss.height; y++) {
                for (size_t x = 0; x < ss.width; x++) {
                    size_t idx = y * ss.width + x;
                    img << static_cast<int>(ss.data[idx * 3]) << " "
                        << static_cast<int>(ss.data[idx * 3 + 1]) << " "
                        << static_cast<int>(ss.data[idx * 3 + 2]) << endl;
                }
            }
            img.close();
            cout << "Written." << endl;
        } else {
            auto maybe_reply = kt001.RawCommand(cmd, &sys_err);
            if (maybe_reply) {
                cout << "Received " << maybe_reply->length()
                     << " bytes of data:" << endl;
                hexdump(*maybe_reply, cout, false);
            } else {
                cout << sys_err.ToString() << endl;
            }
        }
    }
    return 0;
}

