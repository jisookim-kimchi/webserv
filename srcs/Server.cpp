
#include "../includes/Server.hpp"
#include <sys/epoll.h>
#include <arpa/inet.h>

#define MAX_EVENTS 256
#define BUF_SIZE 4096
Server::Server() {
}

Server::~Server() {
}

/*
    @brief assign operator disabled to prevent duplicating server instance.
*/
const Server& Server::operator=(const Server& other) {
    (void)other;
    return *this;
}

/*
    @brief Copy constructor disabled to prevent duplicating server instance.
*/
Server::Server(const Server& other) {
    (void)other;
}

/*
    @brief make socket for each ServerConfig and assign to listenSockets_ vector.
    @param serverConfigs Vector of ServerConfig objects.
*/
Server::Server(const std::vector<ServerConfig>& serverConfigs) : serverConfigs_(serverConfigs) {
    for (size_t i = 0; i < serverConfigs_.size(); i++) {
        const std::string& host = serverConfigs_[i].getHost();
        const std::vector<uint16_t>& ports = serverConfigs_[i].getPort();
        for (size_t j = 0; j < ports.size(); j++) {
            std::unique_ptr<ListenSocket> socket = std::make_unique<ListenSocket>();
            socket->createSocket();
            socket->setSocketOption();
            socket->setNonBlocking();
            socket->bind(ports[j], host);
            socket->listen();
            listenSockets_.push_back(std::move(socket));
        }
    }
}

bool Server::isListenSocket(int fd) {
    for (size_t i = 0; i < listenSockets_.size(); i++) {
        if (listenSockets_[i]->getFd() == fd)
            return true;
    }
    return false;
}

/*
    @brief run Server...
    단계 ~ 7단계: Server::run() 메인 이벤트 루프] 🔵 (지금 작성할 순서!)
    poll() / epoll() 감시 대상 등록
    대기 모드인 listenSockets_의 FD들을 poll() 감시 목록(pollfd 배열)에 넣고 읽기 이벤트(POLLIN)
   감시 시작 poll() 호출 (이벤트 대기) 손님(클라이언트)이 접속 벨을 누를 때까지 대기 accept() (손님
   맞이 및 1대1 통화기 생성) 손님이 오면 ListenSocket::accept()를 호출해 손님 전용 클라이언트 소켓
   FD를 새로 얻어냄 클라이언트와 HTTP 데이터 주고받기 (recv / send) 클라이언트 소켓으로 HTTP 요청
   읽기 및 HTTP 응답 전송
*/
void Server::run() {
    int epollFd = epoll_create(1);  // checkpoint installed...
    if (epollFd == -1) {
        throw std::runtime_error("epoll created failed");
    }
    for (size_t i = 0; i < listenSockets_.size(); i++) {
        int fd = listenSockets_[i]->getFd();
        struct epoll_event event;
        event.events = EPOLLIN;  //  A Event what the checkpoint must check.  EPOLLIN : Something is
                                 //  ready to be READ on this FD.
        event.data.fd = fd;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &event) == -1)  // checkpoint is now ready!
        {
            throw std::runtime_error("epoll ctl add failed");
        }
    }
    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int eventsFds = epoll_wait(epollFd, events, MAX_EVENTS, -1);  // checkpoint is running.
        if (eventsFds == -1) {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("epoll wait failed");
        }
        for (int i = 0; i < eventsFds; i++) {
            int eventfd = events[i].data.fd;
            if (isListenSocket(eventfd) == true) {
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);

                int clientFd = accept(eventfd, (struct sockaddr*)&clientAddr, &clientAddrLen);
                if (clientFd != -1) {
                    struct epoll_event clientEvent;
                    clientEvent.events = EPOLLIN;
                    clientEvent.data.fd = clientFd;
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent);
                }
            } else {
                char buf[BUF_SIZE];
                int ret = read(eventfd, buf, BUF_SIZE);  // Disconnect /TCP FIN
                if (ret <= 0) {
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, eventfd, NULL);
                    close(eventfd);
                } else {
                    std::cout << "  Server Got Client's Request Received.    " << std::endl;
                    std::cout.write(buf, ret);
                    std::string response =
                        "HTTP/1.1 200 OK\r\nContent-Length: 15\r\n\r\n Hello Client!\n";
                    send(eventfd, response.c_str(), response.length(), 0);
                }
            }
        }
    }
}
