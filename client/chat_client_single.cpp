#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>   // htons, htonl
#include <netinet/in.h>  // sockaddr_in
#include <unistd.h>      // close
#include <cstring>       // memset

ssize_t readline_client(std::string & out);

int main () {

    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
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
    std::cout << "Please enter your name: ";
    std::getline(std::cin, line);
    line += "\n";
    write(sockfd, line.c_str(), line.size());
    std::string buf;
    ssize_t n = 0;
    while(true) {
        n = readline_client(buf);
        write(sockfd, buf.c_str(), buf.size());
        if(n == 1) break; 
    }
    std::cout << std::endl << "Leaving the chatRoom..." << std::endl;
    close(sockfd);
    return 0;
}

ssize_t readline_client(std::string & out) {
    out.clear();
    char buf;
    ssize_t a = 0;
    while(true) {
        std::cin.read(&buf, 1);
        out.push_back(buf);
        ++a;
        if(buf == '\n' ) return a;
    }
    return a;
}