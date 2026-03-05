#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <vector>
#include <mutex>
#include <thread>
#include <string>
#include <algorithm>

ssize_t readline(int fd, std::string & out);
void broadcast_message(const std::string& from, const std::string& msg, int exclude_fd);
void handle_client(int client_fd);

std::vector<int> g_clients;
std::mutex g_clients_mutex;

int main () {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_fd == -1) {
        perror("Sock");
        return 1;
    }

    std::cout << "listen_fd: " << listen_fd << std::endl;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(5000);

    if(bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return 1;
    }
    int n = 0;
    if(listen(listen_fd, 5) == -1) {
        perror("listen");
        close(listen_fd);
        return 1;
    }
    std::cout << "listening on 0.0.0.0:5000 ..." << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    std::cout << "waiting for a client to connect ..." << std::endl;

    while(true) {
        int client_fd = accept(listen_fd, (sockaddr *)&client_addr, &client_len);
        if(client_fd == -1) {
            perror("accept");
            continue;
        }
        std::thread t(handle_client, client_fd);
        t.detach();
    }
    close(listen_fd);
    return 0;
    
}

ssize_t readline(int fd, std::string & out) {
    out.clear();
    char buf;
    while(true) {
        ssize_t m = read(fd, &buf, 1);

        if(m == 1) {
            if(buf == '\n') return (ssize_t)out.size();
            out.push_back(buf);
            continue;
        }
        if(m == 0) {
            if(!out.empty()) return (ssize_t)out.size();
            return 0;
        }
        if(errno == EINTR) continue;
        return -1;
    }
    return -1;
} 

void broadcast_message(const std::string& from, const std::string& msg, int exclude_fd) {
    std::string line = "[" + from + "] " + msg + "\n";

    std::lock_guard<std::mutex> lock(g_clients_mutex);
    for (int fd : g_clients) {
        if (fd == exclude_fd) continue;
        write(fd, line.c_str(), line.size());
    }
}

void handle_client(int client_fd) {
    std::string nickname;
    if (readline(client_fd, nickname) <= 0) {
        std::cout << "user did't send any message and disconected." << std::endl;
        close(client_fd);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_clients_mutex);
        g_clients.push_back(client_fd);
    }

    std::cout << "user [" << nickname << "] joins in the chatRoom. client_fd: " << client_fd << std::endl;
    broadcast_message("System", nickname + " join in the chatRoom", client_fd);

    std::string msg;
    while (true) {
        ssize_t n = readline(client_fd, msg);
        if(n <= 0) {
            break;
        }
        broadcast_message(nickname, msg, client_fd);
    }
    std::cout << "user [" << nickname << "] left the chatRoom." << std::endl;
    broadcast_message("System", nickname + " left the chatRoom.", client_fd);

    {
        std::lock_guard<std::mutex> lock(g_clients_mutex);
        g_clients.erase(std::remove(g_clients.begin(), g_clients.end(), client_fd), g_clients.end());
    }

    close(client_fd);
    return;
}