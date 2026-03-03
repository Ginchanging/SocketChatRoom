#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    using std::cout, std::endl;
    if (argc != 3) {
        std::cout << "用法: " << argv[0] << " <server_ip> <port>\n";
        return 1;
    }

    std::string ip = argv[1];
    int port = 0;

    try {
        std::size_t pos = 0;
        port = std::stoi(argv[2], &pos);
        if(pos != std::string(argv[2]).size()) {
            throw std::invalid_argument("Unvaliable port!");
        }
        if(port < 1 || port > 65535) { 
            throw std::out_of_range("Unvaliable port nember");
        }
    } catch(const std::invalid_argument e) {
        cout << "invalid_argument: " << e.what() << endl;
        return 1;
    } catch(const std::out_of_range e) {
        cout << "out_of_range: " << e.what() << endl;
        return 1;
    } 
    std::cout << "Connecting to the server: " << ip << ":" << port << std::endl;
    return 0;
}