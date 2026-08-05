#include "linkscreen/windows/host_server.h"

#include <exception>
#include <iostream>

int main() {
    try {
        linkscreen::windows::HostServer server;
        return server.run_once() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "fatal host error: " << error.what() << '\n';
        return 1;
    }
}

