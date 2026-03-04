#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>   // htons, htonl
#include <netinet/in.h>  // sockaddr_in
#include <unistd.h>      // close
#include <cstring>       // memset
#include <cerrno>

ssize_t readline(int fd, std::string & out);


int main () {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_fd == -1) {
        std::cerr << "Error!" << std::endl;
        return 1;
    }

    std::cout << "Listen_ld: " << listen_fd << std::endl;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) == -1) {
        std::cerr << "bind" << std::endl;
        close(listen_fd);
        return 1;
    }
    if (listen(listen_fd, 5) == -1) {
        std::cerr << "Error" << std::endl;
        close(listen_fd);
        return 1;
    }

    std::cout << "listening on 0.0.0.0:5000 ..." << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);

    std::cout << "waiting for a client to connect..." << std::endl;

    int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept");
        close(listen_fd);
        return 1;
    }
    std::cout << "one client connected! fd = " << client_fd << std::endl;
    std::string nickname;
    if (readline(client_fd, nickname) <= 0) {
        std::cout << "user did't send any message and disconected." << std::endl;
        close(client_fd);
        close(listen_fd);
        return 0;
    }

    std::cout << "user[" << nickname << "] join in the dialogue" << std::endl;

    std::string msg;

    while (true) {
        ssize_t n = readline(client_fd, msg);

        if (n <= 0) {
            std::cout << "user[" << nickname << "] leave the chatroom." << std::endl;
            break;
        }
        std::cout << "[" << nickname << "]: " << msg << std::endl;
    }
    close(client_fd);
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