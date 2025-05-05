#include "ChatApp.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "server") {
        ChatApp app(true);
        app.run();
    } else if (argc == 3 && std::string(argv[1]) == "client") {
        ChatApp app(false, argv[2]);
        app.run();
    } else {
        std::cerr << "Использование:\n";
        std::cerr << "  " << argv[0] << " server           — запустить как сервер\n";
        std::cerr << "  " << argv[0] << " client <IP>      — запустить как клиент\n";
        return 1;
    }

    return 0;
}
