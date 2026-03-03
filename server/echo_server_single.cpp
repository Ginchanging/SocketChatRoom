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

    // ====== 第二步：循环读客户端发来的数据，然后原样写回去（回显） ======
    const int BUF_SIZE = 1024;
    char buf[BUF_SIZE];

    while (true) {
        // TODO(1): 调用 read，从 client_fd 读数据到 buf
        // ssize_t n = ...;
        ssize_t n = read(client_fd, buf, BUF_SIZE);

        if (n < 0) {
            perror("read");
            break;  // 读出错，退出循环
        }

        if (n == 0) {
            std::cout << "client closed the connection" << std::endl;
            break;  // 客户端关闭连接
        }

        // 现在 buf 中有 n 字节数据，我们原样写回去
        // 简化起见，我们先假设 write 一次性写完（教学阶段可以先这样）
        // TODO(2): 调用 write，把 buf 里的 n 字节写回 client_fd
        // ssize_t written = ...;
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