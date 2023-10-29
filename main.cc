#include "channel.h"
#include <thread>
#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

int main() {
    auto [tx, rx] = make_channel<std::string>();
    std::thread t{[](Channel<std::string> tx) {
        std::this_thread::sleep_for(100ms);
        for (int i = 0; i < 100; ++i) {
            tx.send(std::to_string(i));
        }
    }, std::move(tx)};

    for (;;) {
        auto result = rx.recv();
        if (!result) break;
        std::cout << *result << "\n";
    }

    t.join();

    return 0;
}