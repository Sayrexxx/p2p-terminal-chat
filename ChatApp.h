#include <string>
#include <thread>
#include <mutex>
#include <ncursesw/ncurses.h>

class ChatApp {
public:
    ChatApp(bool isServer, const std::string& ip = "");
    ~ChatApp();
    void run();

private:
    int sockfd = -1;
    int connfd = -1;
    bool isServer;
    std::string ipAddress;

    WINDOW* winChat = nullptr;
    WINDOW* winInput = nullptr;

    std::thread recvThread;
    std::mutex mtx;

    void setupUI();
    void teardownUI();
    void startConnection();
    void handleInput();
    void handleReceive();
    void printMessage(const std::wstring& msg, bool isSelf);
};
