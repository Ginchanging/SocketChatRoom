#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>

int main () {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_fd == -1) {
        std::cerr << "Error!" << std::endl;
        return 1;
    }

    std::cout << listen_fd << std::endl;
    return 0;
}