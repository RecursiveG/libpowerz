#include <string>
#include <iostream>
#include "serial.h"
#include "kt001.h"

using namespace std;
using namespace powerz;

template<typename T, typename E>
T unwrap(std::variant<T, E> var) {
    if (var.index() == 1) {
        std::cerr << get<1>(var).ToString();
        exit(1);
    }
    return get<0>(std::move(var));
}

template<typename E>
void unwrap_inverse(std::optional<E> maybe_err) {
    if (maybe_err) {
        std::cerr << maybe_err->ToString();
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    string tty_dev = "/dev/ttyACM0";

    KT001 kt001(unwrap(Serial::Connect(tty_dev)));
    unwrap_inverse(kt001.Handshake());
    cout << "Handshake success" << endl;
    auto ver = unwrap(kt001.GetFwVersion());
    cout << "FW version: " << ver << endl;
    return 0;
}
