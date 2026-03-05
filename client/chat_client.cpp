#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <atomic>
#include <thread>
#include <errno.h>

std::atomic<bool> g_running{true};

ssize_t read_line(int fd, std::string & out) {
    out.clear();
    char ch;

    while (true) {
        ssize_t n = ::read(fd, &ch, 1); 

        if (n == 1) {
            if (ch == '\n') return (ssize_t)out.size();
            out.push_back(ch);
            continue;
        }

        if (n == 0) { 
            if (!out.empty()) return (ssize_t)out.size();
            return 0;
        }

        if (errno == EINTR) continue;
        return -1; 
    }
} 

void sender_thread_func(int sockfd) {
    std::string line;
    while(g_running) {
        if(!std::getline(std::cin, line)) {
            break;
        }
        if(line == "/quit") {
            g_running = false;
            shutdown(sockfd, SHUT_RDWR);
            break;
        }

        line.push_back('\n');
        write(sockfd, line.c_str(), line.size());

    }
}

void receiver_thread_func(int sockfd) {
    std::string line;
    while (g_running) {
        ssize_t n = read_line(sockfd, line);
        if(n <= 0) {
            g_running = false;
            break;
        }
        std::cout << line << std::endl;
    }
}

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
    
    std::thread sender(sender_thread_func, sockfd);
    std::thread receiver(receiver_thread_func, sockfd);

    sender.join();
    receiver.join();
    close(sockfd);
    std::cout << std::endl << "Leaving the chatRoom..." << std::endl;
    
    return 0;
}