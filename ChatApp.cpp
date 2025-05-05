#include "ChatApp.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 12345
#define MAX_MSG_LEN 512

ChatApp::ChatApp(bool isServer, const std::string& ip)
        : isServer(isServer), ipAddress(ip) {}

ChatApp::~ChatApp() {
    if (connfd != -1) close(connfd);
    if (sockfd != -1) close(sockfd);
    if (recvThread.joinable()) recvThread.join();
    teardownUI();
}

void ChatApp::setupUI() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    curs_set(1);
    keypad(stdscr, TRUE);
    start_color();

    init_pair(1, COLOR_CYAN, COLOR_BLACK);  // self
    init_pair(2, COLOR_GREEN, COLOR_BLACK); // peer
    init_pair(3, COLOR_WHITE, COLOR_BLACK); // input

    int chatHeight = LINES - 4;
    winChat = newwin(chatHeight, COLS, 0, 0);
    winInput = newwin(4, COLS, chatHeight, 0);

    scrollok(winChat, TRUE);

    box(winChat, 0, 0);
    mvwprintw(winChat, 0, 2, " Чат ");
    wrefresh(winChat);

    box(winInput, 0, 0);
    mvwprintw(winInput, 0, 2, " Введите сообщение: ");
    wrefresh(winInput);
}

void ChatApp::teardownUI() {
    delwin(winChat);
    delwin(winInput);
    endwin();
}

void ChatApp::startConnection() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);

    if (isServer) {
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
        listen(sockfd, 1);
        mtx.lock();
        mvwprintw(winChat, 1, 1, "Ожидание подключения...");
        wrefresh(winChat);
        mtx.unlock();
        socklen_t len = sizeof(addr);
        connfd = accept(sockfd, (struct sockaddr*)&addr, &len);
    } else {
        inet_pton(AF_INET, ipAddress.c_str(), &addr.sin_addr);
        if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            mtx.lock();
            mvwprintw(winChat, 1, 1, "Не удалось подключиться к серверу.");
            wrefresh(winChat);
            mtx.unlock();
            exit(1);
        }
        connfd = sockfd;
    }
}

void ChatApp::printMessage(const std::wstring& msg, bool isSelf) {
    wattron(winChat, COLOR_PAIR(isSelf ? 1 : 2));
    wprintw(winChat, "[%s] ", isSelf ? "Вы" : "Собеседник");
    wattroff(winChat, COLOR_PAIR(isSelf ? 1 : 2));
    wprintw(winChat, "%ls\n", msg.c_str());
    wrefresh(winChat);
}

void ChatApp::handleInput() {

    while (true) {
        werase(winInput);
        box(winInput, 0, 0);
        mvwprintw(winInput, 0, 2, " Введите сообщение: ");
        wmove(winInput, 1, 1);
        wrefresh(winInput);

        std::wstring input;
        wint_t ch;
        int x = 1;

        while (true) {
            int res = wget_wch(winInput, &ch);
            if (res == ERR) continue;

            if (ch == L'\n') {
                break;
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (!input.empty()) {
                    input.pop_back();
                    x = std::max(1, x - 1);
                }
            } else if (iswprint(ch)) {
                if (input.length() < MAX_MSG_LEN - 1) {
                    input += ch;
                    x++;
                }
            }

            werase(winInput);
            box(winInput, 0, 0);
            mvwprintw(winInput, 0, 2, " Введите сообщение: ");
            mvwprintw(winInput, 1, 1, "%ls", input.c_str());
            wmove(winInput, 1, x);
            wrefresh(winInput);
        }

        if (input == L"/exit") break;

        char utf8msg[MAX_MSG_LEN * 4];
        std::wcstombs(utf8msg, input.c_str(), sizeof(utf8msg));

        send(connfd, utf8msg, strlen(utf8msg), 0);

        std::lock_guard<std::mutex> lock(mtx);
        printMessage(input, true);
    }
}

void ChatApp::handleReceive() {
    char buffer[MAX_MSG_LEN * 4];
    wchar_t wbuffer[MAX_MSG_LEN];
    int n;

    while ((n = recv(connfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        std::mbstowcs(wbuffer, buffer, MAX_MSG_LEN);

        std::lock_guard<std::mutex> lock(mtx);
        printMessage(wbuffer, false);
    }

    std::lock_guard<std::mutex> lock(mtx);
    wprintw(winChat, "Собеседник отключился.\n");
    wrefresh(winChat);
}

void ChatApp::run() {
    setupUI();
    startConnection();
    recvThread = std::thread(&ChatApp::handleReceive, this);
    handleInput();
}
