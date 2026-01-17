#ifndef TCPSERVER_H //혹은 #pragma once
#define TCPSERVER_H

#include <iostream>
#include <sys/socket.h> // 소켓 함수들 (socket, bind, listen...)
#include <netinet/in.h> // 주소 구조체 (sockaddr_in)
#include <arpa/inet.h>  //inet_ntoa 함수(IP주소 변환)
#include <unistd.h>     // close 함수
#include <cstring>      // memset
#include <thread>       //thread 사용
#include <vector>       //vector로 여러 client 관리
#include <mutex>        //vector 관리를 위한 mutex
#include <algorithm>
#include <map>     //std::map 이용
#include <fstream> //file 입출력

class TcpServer
{
private:
    int server_socket_fd;                                       // main socket number
    int port;                                                   // port number
    std::map<int, std::string> client_sockets;                  // socket을 담는 map
    std::mutex client_mutex;                                    // client_sockets의 관리자
    void ClientHandler(int client_socket_fd);                   // thread를 통해 여러 client병렬처리
    void BroadCast(int current_socket, std::string client_msg); // client의 msg를 전달해주는 함수
    std::string ReadFile(const std::string &file_name);
public:
    TcpServer(int port_num);
    ~TcpServer();
    void run();
};


#endif