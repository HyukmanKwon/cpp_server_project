#include "TcpServer.h"

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