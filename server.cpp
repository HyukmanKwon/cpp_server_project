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

void TcpServer::ClientHandler(int client_socket_fd){
    char temp_buffer[3000];//browser의 요청을 담는 buffer
    //1. browser에서 온 것을 read
    memset(temp_buffer, 0, sizeof(temp_buffer));
    read(client_socket_fd, temp_buffer, sizeof(temp_buffer));

    //browser에서 보낸 요청을 선택(image를 원하는지, 아니면 HTML을 원하는지)
    if(strstr(temp_buffer, "GET /leesimyeong.jpg")!=nullptr){//GET /leesimyeong.jpg요청이 있을때
        //1. browser가 leesimyeong.jpg를 요청했을 때 -> image파일 열기
        std::ifstream file("leesimyeong.jpg", std::ios::ate | std::ios::binary);//|를 통해 명령어를 조합해줌
        //binary: data로 읽음 ate: 
        if(file.is_open()){
            //file 크기 재기
            std::streamsize file_size=file.tellg();
            file.seekg(0, std::ios::beg);//seekg(offset, 기준)->std::ios::beg(파일의 맨앞)에서 0만큼 이동

            //file내용 읽어서 담기
            std::vector<char> file_buffer(file_size);
            file.read(file_buffer.data(), file_size);

            //2. image용 header만들기(Content-Type이 image/jpeg)
            std::string httep_header=
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: image/jpeg\r\n" 
            "Content-Length: "+std::to_string(file_size)+"\r\n"
            "\r\n";
            
            //3. header, body순으로 전송
            write(client_socket_fd, httep_header.c_str(), httep_header.size());
            write(client_socket_fd, file_buffer.data(), file_size);
        }
    }
    else{//html
        //1. HTML작성 설명과 이미지 태그 포함
        std::string html_body = R"(
        <html>
        <head>
            <meta charset='UTF-8'>
            <title>(환) 경상고등학교 (영)</title>
            <style>
            /* CSS design*/
                body{
                    background-color: #f4f4f4;
                    display: flex;
                    jstify-content: center;
                    align-items: center;
                    height: 100vh;
                    margin: 0;
                    font-family: 'Malgun Gothic', sans-serif;
                }
                .container{
                    background: white;
                    padding: 40px;
                    border-radius: 20px;
                    box-shadow: 0 10px 25px rgba(0,0,0,0.1);
                    text-align: center;
                }
                h1{color: #2c3e50; margin-bottom: 20px;}
                img{
                    width: 250px;
                    height: 250px;
                    border-radious: 50%; /*원형 이미지*/
                    object-fit: cover;
                    border: 5px solid #FFD700;
                    margin-bottom: 20px;
                }
                p{font-size: 1.1em; color #555;}
                .highlight{color: #e74c3c; font-weight: bold;}
            </style>
        </head>
        <body>
            <div class="container">
                <h1> 경상고등학교에 오신 것을 환영합니다! </h1>
                <img src='/leesimyeong.jpg' alt='심영이 사진'/>
                <p>이름은 <span class="highlight">심영</span>이고, 츄르를 좋아해요</p>
                <p><strong>C++를 통해 띄운 심영이입니다</p>
            </div>
        </body>
        </html>
        )";

        //2. HTML용 헤더 만들기
        std::string http_response = 
        "HTTP/1.1 200 OK\r\n"                         // HTTP/1.1로 성공
        "Content-Type: text/html; charset=UTF-8\r\n" // HTML임을 알림
        "Content-Length: " + std::to_string(html_body.size()) + "\r\n" // content의 길이를 알려줌
        "\r\n" +                                     // header와 body를 구분하는 빈줄
        html_body;                                  //html_body를 연결

        //3. 내용을 전송
        write(client_socket_fd, http_response.c_str(), http_response.size());
    }
    

    close(client_socket_fd);
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