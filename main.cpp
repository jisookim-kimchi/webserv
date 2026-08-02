#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

int main()
{   
    const int port = 8080;
    const int protocol = IPPROTO_TCP;
    
    /*1. create socket
    @param 1 : Doman/ Address Family : AF_INET - IPv4
    @param 2: Socket Type : SOCK_STREAM - TCP
    @param 3 : protocol : IPPROTO_TCP - TCP
    @return : file descriptor for the socket
    */
    const int serverFd = socket(AF_INET, SOCK_STREAM, protocol);
    if (serverFd < 0)
        return (0);
    else
        std::cout << "socket : " << serverFd << std::endl;

    //2. assigning a socet to a specific IP address and port number.
    struct sockaddr_in sockAddr = {};
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockAddr.sin_port = htons(port);

    /*
    @param 1 : socket descriptor
    @param 2 : socket address
    @param 3 : size of socket address
    @return : 0 on success, -1 on failure
    */
    if (bind(serverFd, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) < 0)
        return (0);
    else
        std::cout << "bind : " << serverFd << std::endl;
    
    //3.listen connection
    //@param 1 : socket fd
    //@param 2 : max number of connections
    //@return : 0 on success, -1 on failure
    int ret = listen(serverFd, SOMAXCONN);
    if (ret < 0)
    {
        close(serverFd);
        return (0);
    }
    else
        std::cout << "listen : " << ret << std::endl;    

    //4.accept connection
    //@param 1 : socket fd
    //@param 2 : pointer to address for client's ip and port number
    //@param 3 : size for client's ip and port number
    //@return: fd where request connection is accepted.
    struct sockaddr_in outClientAddr = {};
    socklen_t outClientAddrLen = sizeof(outClientAddr);
    int clientFd = 0;
    clientFd = accept(serverFd, (struct sockaddr *)&outClientAddr, &outClientAddrLen);
    if (clientFd < 0)
    {
        close(serverFd);
        return (0);
    }
    else
        std::cout << "accept : " << clientFd << std::endl;
    
    char buf[4096] = {0};
    const int val = read(clientFd, buf, 4096);
    if(val < 0)
    {
        close(clientFd);
        close(serverFd);
        return (0);
    }
    else
        std::cout << "read : " << val << std::endl;

    std::string userInput;
    std::cout << "Enter something : ";
    std::getline(std::cin, userInput);
    userInput +="\r\n";
    // server response message
    std::string response = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: " + std::to_string(userInput.length()) + "\r\n"
    "\r\n"
    + userInput;

    /*
    @param 1 : socket fd
    @param 2 : pointer to buffer
    @param 3 : size of buffer
    @param 4 : flags : MSG_NOSIGNAL- do not send signal when the client is disconnected
    @return : 0 on success, -1 on failure
    */
    ret = send(clientFd, response.c_str(), response.length(), MSG_NOSIGNAL);
    if(ret < 0)
    {
        close(clientFd);
        close(serverFd);
        return (0);
    }
        
    if (close(clientFd) < 0)
        return (0);
    if (close(serverFd) < 0)
        return (0);
    return 1;
}