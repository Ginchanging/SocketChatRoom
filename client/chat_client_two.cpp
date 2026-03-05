#include <iostream>
#include <string>
#include <atomic>
#include <thread>   // <- 必须无条件包含
#include <cstring>
#include <cstdint>

#ifdef _WIN32
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif

  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #include <errno.h>
#endif

// -------------------- thin net wrapper --------------------
namespace net {
#ifdef _WIN32
  using socket_t = SOCKET;
  static constexpr socket_t kInvalid = INVALID_SOCKET;
#else
  using socket_t = int;
  static constexpr socket_t kInvalid = -1;
#endif

  inline bool init() {
#ifdef _WIN32
    WSADATA wsa{};
    return WSAStartup(MAKEWORD(2,2), &wsa) == 0;
#else
    return true;
#endif
  }

  inline void cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
  }

  inline int last_error() {
#ifdef _WIN32
    return (int)WSAGetLastError();
#else
    return errno;
#endif
  }

  inline void close(socket_t s) {
#ifdef _WIN32
    closesocket(s);
#else
    ::close(s);
#endif
  }

  inline int shutdown_rdwr(socket_t s) {
#ifdef _WIN32
    return ::shutdown(s, SD_BOTH);
#else
    return ::shutdown(s, SHUT_RDWR);
#endif
  }

  inline socket_t socket_tcp() {
#ifdef _WIN32
    return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    return ::socket(AF_INET, SOCK_STREAM, 0);
#endif
  }

  // 关键修复点：Windows 不用 InetPtonA，改用 inet_addr（只处理 IPv4 字符串足够）
  inline bool parse_ipv4(const char* ip, in_addr* out_addr) {
#ifdef _WIN32
    // inet_addr 返回网络字节序的 IPv4 地址，失败返回 INADDR_NONE
    unsigned long a = ::inet_addr(ip);
    if (a == INADDR_NONE) return false;   // 注意：255.255.255.255 也会等于 INADDR_NONE（极少用）
    out_addr->s_addr = a;
    return true;
#else
    int r = ::inet_pton(AF_INET, ip, out_addr);
    return r == 1;
#endif
  }

  inline bool connect_ipv4(socket_t s, const char* ip, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (!parse_ipv4(ip, &addr.sin_addr)) return false;
    if (::connect(s, (sockaddr*)&addr, sizeof(addr)) != 0) return false;
    return true;
  }

  inline long long recv_some(socket_t s, void* buf, size_t len) {
#ifdef _WIN32
    int n = ::recv(s, (char*)buf, (int)len, 0);
    if (n == SOCKET_ERROR) return -1;
    return (long long)n;
#else
    while (true) {
      ssize_t n = ::recv(s, buf, len, 0);
      if (n < 0 && errno == EINTR) continue;
      return (long long)n;
    }
#endif
  }

  inline long long send_all(socket_t s, const void* buf, size_t len) {
    const char* p = (const char*)buf;
    size_t left = len;

    while (left > 0) {
#ifdef _WIN32
      int n = ::send(s, p, (int)left, 0);
      if (n == SOCKET_ERROR) return -1;
#else
      ssize_t n = ::send(s, p, left, 0);
      if (n < 0) {
        if (errno == EINTR) continue;
        return -1;
      }
#endif
      if (n == 0) return -1;
      p += n;
      left -= (size_t)n;
    }
    return (long long)len;
  }
} // namespace net

// -------------------- app logic --------------------
std::atomic<bool> g_running{true};

// 按行读取：内部缓存 + recv_some，跨平台
static bool read_line(net::socket_t sockfd, std::string& out, std::string& cache) {
  out.clear();

  while (true) {
    auto pos = cache.find('\n');
    if (pos != std::string::npos) {
      out = cache.substr(0, pos);
      cache.erase(0, pos + 1);

      // 兼容 Windows \r\n
      if (!out.empty() && out.back() == '\r') out.pop_back();
      return true;
    }

    char buf[4096];
    long long n = net::recv_some(sockfd, buf, sizeof(buf));
    if (n > 0) {
      cache.append(buf, (size_t)n);
      continue;
    }
    if (n == 0) { // 对端关闭
      if (!cache.empty()) {
        out = cache;
        cache.clear();
        if (!out.empty() && out.back() == '\r') out.pop_back();
        return true;
      }
      return false;
    }
    return false; // n < 0
  }
}

static void sender_thread_func(net::socket_t sockfd) {
  std::string line;
  while (g_running) {
    if (!std::getline(std::cin, line)) break;

    if (!line.empty() && line.back() == '\r') line.pop_back();

    if (line == "/quit") {
      g_running = false;
      net::shutdown_rdwr(sockfd); // 唤醒 receiver
      break;
    }

    line.push_back('\n');
    if (net::send_all(sockfd, line.data(), line.size()) < 0) {
      g_running = false;
      break;
    }
  }
}

static void receiver_thread_func(net::socket_t sockfd) {
  std::string line;
  std::string cache;
  while (g_running) {
    if (!read_line(sockfd, line, cache)) {
      g_running = false;
      break;
    }
    std::cout << line << std::endl;
  }
}

int main(int argc, char** argv) {
  // 用法：chat_client_two <ip> <port>
  const char* ip = "127.0.0.1";
  uint16_t port = 5000;

  if (argc >= 2) ip = argv[1];
  if (argc >= 3) port = (uint16_t)std::stoi(argv[2]);

  if (!net::init()) {
    std::cerr << "net::init failed, err=" << net::last_error() << "\n";
    return 1;
  }

  net::socket_t sockfd = net::socket_tcp();
  if (sockfd == net::kInvalid) {
    std::cerr << "socket failed, err=" << net::last_error() << "\n";
    net::cleanup();
    return 1;
  }

  if (!net::connect_ipv4(sockfd, ip, port)) {
    std::cerr << "connect failed, err=" << net::last_error() << "\n";
    net::close(sockfd);
    net::cleanup();
    return 1;
  }

  std::cout << "Connected to server " << ip << ":" << port << ".\n";

  // 发送昵称（协议第一行）
  std::string name;
  std::cout << "Please enter your name: ";
  std::getline(std::cin, name);
  if (!name.empty() && name.back() == '\r') name.pop_back();

  std::string first = name + "\n";
  if (net::send_all(sockfd, first.data(), first.size()) < 0) {
    std::cerr << "send name failed, err=" << net::last_error() << "\n";
    net::close(sockfd);
    net::cleanup();
    return 1;
  }

  std::thread sender(sender_thread_func, sockfd);
  std::thread receiver(receiver_thread_func, sockfd);

  sender.join();
  receiver.join();

  net::close(sockfd);
  net::cleanup();

  std::cout << "\nLeaving the chatRoom...\n";
  return 0;
}