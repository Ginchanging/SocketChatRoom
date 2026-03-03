#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>   // htons, htonl
#include <netinet/in.h>  // sockaddr_in
#include <unistd.h>      // close
#include <cstring>       // memset

int main () {

    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
     // TODO: 使用 inet_pton 将 "127.0.0.1" 转成二进制地址填充到 server_addr.sin_addr
    int res = inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    if(res == 0) {
        std::cerr << "Inet_pton : Invaliable Address";
        return 1;
    }
    else if(res == -1) {
        std::cerr << "Inet_pton";
        return 1;
    }
    
    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::perror("connect");
        close(sockfd);
        return 1;
    }

    std::cout << "Connected to server." << std::endl;

    std::string line;
    std::cout << "请输入一行文本: ";
    std::getline(std::cin, line);
    line += "\n";
    // TODO: 调用 send / write 将 line.c_str() 发送出去（记得加 '\n' 或自己处理长度）
    write(sockfd, line.c_str(), line.size());
    char buf[1024];
    // TODO: 调用 recv / read 收取服务器响应，并打印
    ssize_t n = read(sockfd, buf, sizeof(buf));
    if(n == -1) {
        perror("read");
    }
    else if(n == 0) {
        std::cout << "server closed";
    }
    else {
        std::cout.write(buf, n) << std::endl;
    }
    std::cout << std::endl;
    close(sockfd);
    return 0;
}