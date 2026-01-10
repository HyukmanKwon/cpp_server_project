#include <iostream>
#include <sys/socket.h> // 소켓 함수들 (socket, bind, listen...)
#include <netinet/in.h> // 주소 구조체 (sockaddr_in)
#include <unistd.h>     // close 함수
#include <cstring>      // memset

int main() {
    //1. socket 만들기
    int server_socket_fd=socket(AF_INET, SOCK_STREAM, 0);//int socket(int 도메인, int socket 타입, int protocol)
    // 여기서 만약 server_fd가 -1이면 실패한 거니까 에러 처리를 해주는 게 좋아.
    if(server_socket_fd==-1){
        std::cerr<<"socket 생성 실패, 에러코드: "<<errno<<std::endl;
        return -1;
    }

    // 2. 주소 구조체(sockaddr_in)를 설정하고 bind()를 호출해봐.
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));//구조체 0으로 전부 초기화
    //memset(배열의 시작주소, 넣을 값, 크기);
    server_addr.sin_family=AF_INET;//IPv4 주소 체계를 사용
    server_addr.sin_addr.s_addr=INADDR_ANY;//내 컴퓨터의 모든 IP에서 접속 허용
    server_addr.sin_port=htons(8080);//port num 8080

    // 그 다음 bind(server_fd, (struct sockaddr*)&address, sizeof(address)) 호출!
    int temp=bind(server_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));//bind(int socket var, const struct socket_server, socklen_t addrlen);
    // 이것도 실패하면 -1을 반환하니까 에러 처리를 꼭 해줘.
    if(temp==-1){
        std::cerr<<"binding 실패, 에러 코드: "<<errno<<std::endl;//보통 port가 이미 사용중일 경우
        return -1;
    }
    
    std::cout << "1, 2단계 성공! 소켓 만들고 바인딩까지 완료." << std::endl;
    return 0;
}
