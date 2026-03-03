#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>   // htons, htonl
#include <netinet/in.h>  // sockaddr_in
#include <unistd.h>      // close
#include <cstring>       // memset

int main () {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_fd == -1) {
        std::cerr << "Error!" << std::endl;
        return 1;
    }

    std::cout << "Listen_ld: " << listen_fd << std::endl;
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listen_fd, (sockaddr *)&addr, sizeof(addr)) == -1) {
        std::cerr << "None" << std::endl;
        close(listen_fd);
        return 1;
    }
    if (listen(listen_fd, 5) == -1) {
        std::cerr << "Error" << std::endl;
        close(listen_fd);
        return 1;
    }

    std::cout << "listening on 0.0.0.0:5000 ..." << std::endl;

    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    std::cout << "waiting for a client to connect..." << std::endl;

    int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("accept");
        close(listen_fd);
        return 1;
    }

    std::cout << "one client connected! fd = " << client_fd << std::endl;

    const int BUF_SIZE = 1024;
    char buf[BUF_SIZE];

    while (true) {
        ssize_t n = read(client_fd, buf, BUF_SIZE);

        if (n < 0) {
            perror("read");
            break; 
        }

        if (n == 0) {
            std::cout << "client closed the connection" << std::endl;
            break; 
        }
        ssize_t written = write(client_fd, buf, n);
        if (written < 0) {
            perror("write");
            break;
        }
    }

    close(client_fd);
    close(listen_fd);
    return 0;
}