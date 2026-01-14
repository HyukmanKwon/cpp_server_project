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
#include <map> //std::map 이용

class TcpServer
{
private:
    int server_socket_fd;                                       // main socket number
    int port;                                                   // port number
    std::map<int, std::string> client_sockets;                  // socket을 담는 map
    std::mutex client_mutex;                                    // client_sockets의 관리자
    void ClientHandler(int client_socket_fd);                   // thread를 통해 여러 client병렬처리
    void BroadCast(int current_socket, std::string client_msg); // client의 msg를 전달해주는 함수
public:
    TcpServer(int port_num)
    {
        // 1. socket 만들기
        server_socket_fd = socket(AF_INET, SOCK_STREAM, 0); // int socket(int 도메인, int socket 타입, int protocol)
        if (server_socket_fd == -1)
        { // error 처리
            std::string error_msg = "socket 생성 실패" + std::string(strerror(errno));
            throw std::runtime_error(error_msg); // 오류를 catch문으로 throw해줌
        }
        int opt = 1;
        setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        // 2. 주소 구조체(sockaddr_in) 설정
        struct sockaddr_in server_addr;               // struct sockaddr_in (Socket Address for Internet)-<netinet/in.h> 헤더에 정의, c언어시절에 struct라고 쓴것
        memset(&server_addr, 0, sizeof(server_addr)); // 구조체 0으로 전부 초기화, memset(배열의 시작주소, 넣을 값, 크기);
        server_addr.sin_family = AF_INET;             // IPv4 주소 체계를 사용
        server_addr.sin_addr.s_addr = INADDR_ANY;     // 내 컴퓨터의 모든 IP에서 접속 허용
        server_addr.sin_port = htons(port_num);       // port num 8080
        // network는 항상 big endian. 그러므로 host computer의 숫자 순서를 host to network short(short형 숫자)=htons
        //  그 다음 bind(server_fd, (struct sockaddr*)&address, sizeof(address)) 호출!
        int server_bind = bind(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)); // bind(int socket var, const struct socket_server, socklen_t addrlen);
        // bind 함수는 sockaddr_in에 정보를 담고, sockaddr로 Casting(형변환)을 해서 넘겨줌
        if (server_bind == -1)
        {                                                                          // error 처리
            std::string error_msg = "binding 실패" + std::string(strerror(errno)); // 보통 port가 이미 사용중일 경우
            throw std::runtime_error(error_msg);                                   // 오류를 catch문으로 throw해줌
        }

        // 3. listen만들기
        int server_listen = listen(server_socket_fd, 8); // listen(int socket, 최대 클라이언트 수);
        if (server_listen == -1)
        {
            std::string error_msg = "listening 실패" + std::string(strerror(errno));
            throw std::runtime_error(error_msg); // 오류를 catch문으로 throw해줌
        }
    }
    ~TcpServer()
    { // 소멸자
        close(server_socket_fd);
    }
    void run();
};

void TcpServer::ClientHandler(int client_socket_fd)
{
    std::string my_nickname;
    {
        char temp_buffer[1024];
        ssize_t nickname_len;
        while (true)
        {
            memset(temp_buffer, 0, sizeof(temp_buffer));                               // temp_buffer초기화
            write(client_socket_fd, "write your name: ", sizeof("write your name: ")); // 닉네임 입력을 요청해줌
            nickname_len = read(client_socket_fd, temp_buffer, sizeof(temp_buffer));   // nickname 받아오기
            bool duplication_nickname = false;
            if (nickname_len <= 0)
            { // 연결 끊김 처리
                close(client_socket_fd);
                return;
            }
            if (nickname_len > 0 && temp_buffer[nickname_len - 1] == '\n')
                temp_buffer[nickname_len - 1] = '\0'; // 개행문자를 null로 대체
            if (nickname_len > 1 && temp_buffer[nickname_len - 2] == '\r')
                temp_buffer[nickname_len - 2] = '\0';
            if (strlen(temp_buffer) == 0)
                continue;
            client_mutex.lock(); // lock 걸기
            for (auto i = client_sockets.begin(); i != client_sockets.end(); i++)
            { // duplication_nickname으로 닉네임의 중복을 체크함
                if (std::string(i->second) == std::string(temp_buffer))
                    duplication_nickname = true;
            }
            client_mutex.unlock(); // lock 해제
            if (duplication_nickname == false)
            {
                my_nickname = std::string(temp_buffer);
                break;
            }

            else
                write(client_socket_fd, "This nickname already exists. Please enter again\n", sizeof("This nickname already exists. Please enter again"));
        }
        client_mutex.lock(); // lock 걸어버림
        client_sockets[client_socket_fd] = std::string(temp_buffer);
        client_mutex.unlock(); // lock 해제
        BroadCast(client_socket_fd, client_sockets[client_socket_fd] + " has entered!\r\n");
    }

    while (true) // 채팅 loop
    {
        // 5-1. read() 구현
        char temp_buffer[1024];
        ssize_t server_read = read(client_socket_fd, temp_buffer, sizeof(temp_buffer));
        // ssize_t read(int client_socket_fd, 받아올 버퍼의 포인터, size_t 받아올 크기);
        if (server_read <= 0)                               // 0==종료, -1==error
        {                                                   // client 연결종료(EOF)
            std::lock_guard<std::mutex> lock(client_mutex); // lock걸기
            client_sockets.erase(client_socket_fd);         // client 삭제
            if (server_read == -1)
            {
                // reading 실패
                std::cerr << "reading 실패, 에러 코드: " << errno << std::endl;
                break; // read에서의 에러->보통 연결에 문제가 생긴 경우->break를 통해 close()호출
            }
            break; // while loop를 탈출해서 client 종료
        }
        if (server_read < 1024)
            temp_buffer[server_read] = '\0';
        if (server_read > 0 && temp_buffer[server_read - 1] == '\n')//개행문자삭제
        {
            temp_buffer[server_read - 1] = '\0'; 
        }
        // 5-2.write() 구현
        std::string guest_msg = my_nickname + ": " + temp_buffer + "\r";
        BroadCast(client_socket_fd, guest_msg);
        // ssize_t write(int client_socket_fd, const void *(보낼 문자열), size_t (보낼 문자열의 길이));
    }
    close(client_socket_fd); // client_socket_fd를 close
    return;
}

void TcpServer::BroadCast(int current_socket, std::string client_msg)
{
    std::lock_guard<std::mutex> lock(client_mutex); // mutex lock
    int vecter_size = client_sockets.size();        // std::lock_guard<std::mutex> lock(client_mutex);
    for (auto i = client_sockets.begin(); i != client_sockets.end(); i++)
    {
        if (i->first == current_socket) // 본인의 경우에는 제외+ i->first를 사용하면 key값을 반환함.
            continue;
        write(i->first, client_msg.c_str(), client_msg.size()); // 모든 client에 대하여 write해주기
        //&client_msg를 하면 string class의 주소를 줘버림, client_msg.c_str()를 해야 진짜 주소를 줌
    }
    return;
}

void TcpServer::run()
{
    while (true)
    {
        // 4. accept-client가 올때까지 program이 멈춰서 기다림
        struct sockaddr_in client_addr;             // client의 주소 정보를 담을 sockaddr_in 변수
        socklen_t client_len = sizeof(client_addr); // client의 크기를 담을 socklen_t type의 변수
        int client_socket_fd = accept(this->server_socket_fd, (struct sockaddr *)&client_addr, &client_len);
        // accept(server_socket, client_addr의 casting된 주소, client_len의 주소);
        // 이의 반환값은 new socket fd를 되돌려줌->즉, 이제 client와 소통할 때는 server_socket_fd가 아닌 이 new socket fd를 통해야 함.
        if (client_socket_fd == -1)
        {
            std::cerr << "accepting 실패, 에러 코드: " << errno << std::endl;
            continue;
        }
        else
        {
            std::cout << "IP: " << inet_ntoa(client_addr.sin_addr) << std::endl; // 접속한 client의 ip를 출력
        }
        std::thread temp_client(&TcpServer::ClientHandler, this, client_socket_fd); // std::thread var(실행할 함수, this, 매개변수);
        //->여기서는 class의 ClientHandler함수를, this 이 객체에서, client_socket_fd를 매개변수로 해서 thread 만들어!
        temp_client.detach(); // thread 분리
        // temp_client.join()->이 thread가 끝날 때까지 main func이 기다림.(멈춰있음)
        // temp_client.detach()->main func은 바로 다음으로 넘어가고, thread는 백그라운드에서 스스로 돌아감(독립적으로 움직임)
    }
}

int main()
{
    try
    {
        TcpServer tcp_server(8080); // port_num을 8080으로 넘겨주며 tcp_server class 생성
        tcp_server.run();           // 내부 함수인 tcp_server::run()으로 server실행
    }
    catch (const std::exception &e)
    {                                       // 오류를 받아옴, e.what()에 아까 throw할 때 적은 메시지가 들어있음
        std::cerr << e.what() << std::endl; // 에러코드 출력
    }
    return 0;
}