#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

// 클라이언트 구조체 정의
typedef struct s_clients {
  int     id;  // 클라이언트 아이디
  char    msg[1024];  // 클라이언트 메시지 버퍼
} t_clients;

t_clients clients[1024];  // 최대 1024개의 클라이언트를 담을 수 있는 배열 선언

// 파일 디스크립터 세트 선언: 읽기, 쓰기, 활성화된 파일 디스크립터
fd_set  readFds, writeFds, activeFds;

// 최대 파일 디스크립터 값과 다음 클라이언트 id 값 초기화
int         fdMax = 0, idNext = 0;
char        bufferRead[120000], bufferWrite[120000];  // 읽기와 쓰기를 위한 버퍼

// 에러 발생 시 에러 메시지를 출력하고 프로그램 종료
void    ftError(char *str) {
  if (str)
    write(2, str, strlen(str));  // 에러 메시지 출력
  else
    write(2, "Fatal error", strlen("Fatal error"));  // 기본 에러 메시지 출력
  write(2, "\n", 1);  // 줄바꿈 문자 출력
  exit(1);  // 프로그램 종료
}

// 모든 클라이언트에게 메시지 전송 함수 (특정 클라이언트 제외)
void    sendAll(int not) {
  for(int i = 0; i <= fdMax; i++)  // 모든 파일 디스크립터에 대해 반복
    if(FD_ISSET(i, &writeFds) && i != not)  // 쓰기 가능하고, 제외할 클라이언트가 아니면
      send(i, bufferWrite, strlen(bufferWrite), 0);  // 메시지 전송
}

int main(int ac, char **av) {
  int serverSockFd;  // 서버 소켓과 연결 소켓 파일 디스크립터 선언
  socklen_t  len;  // 소켓 주소 길이 변수 선언
  struct sockaddr_in  servaddr;  // 서버 주소 구조체 선언

  if (ac != 2)  // 인자의 개수 확인
    ftError("Wrong number of arguments");  // 잘못된 인자 개수 에러 출력 및 프로그램 종료
  // TCP/IP 소켓 생성
  serverSockFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSockFd < 0)
    ftError(NULL);

  // active 파일 디스크립터 세트 초기화
  FD_ZERO(&activeFds);
  // 클라이언트 구조체 배열 초기화
  bzero(&clients, sizeof(clients));
  // 최대 파일 디스크립터 값 초기화
  fdMax = serverSockFd;
  // 서버 소켓을 활성화된 파일 디스크립터 세트에 추가
  FD_SET(serverSockFd, &activeFds);

  // 서버 주소 구조체 초기화
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;  // 주소 체계 설정 (IPv4)
  servaddr.sin_addr.s_addr = htonl(2130706433);  // IP 주소 설정 (127.0.0.1)
  servaddr.sin_port = htons(atoi(av[1])); // 포트설정

  // 소켓 바인딩
  if ((bind(serverSockFd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) < 0)
    ftError(NULL);
  // 소켓 리스닝 시작 (최대 대기 클라이언트 수: 10)
  if (listen(serverSockFd, 10) < 0)  // 리스닝 실패 확인
    ftError(NULL);

  while(1) {
    readFds = activeFds;
    writeFds = activeFds;

    // select() 시스템 호출로 입출력 준비 확인
    if (select(fdMax + 1, &readFds, &writeFds, NULL, NULL) < 0) // select 실패 확인
      continue;
    // 모든 파일 디스크립터에 대해 반복
    for(int fdI = 0; fdI <= fdMax; fdI++) {
      // 새로운 클라이언트 연결 수락
      if (FD_ISSET(fdI, &readFds) && fdI == serverSockFd) {
        int connfd = accept(serverSockFd, (struct sockaddr *)&servaddr, &len);  // 연결 수락
        if (connfd < 0)  // 연결 수락 실패 확인
          continue;  // 다음 반복으로 이동
        fdMax = (connfd > fdMax) ? connfd : fdMax;  // 최대 파일 디스크립터 값 갱신
        clients[connfd].id = idNext++;  // 클라이언트 아이디 설정
        FD_SET(connfd, &activeFds);  // 활성화된 파일 디스크립터 세트에 추가
        sprintf(bufferWrite, "server: client %d just arrived\n", clients[connfd].id);  // 메시지 생성
        sendAll(connfd);  // 모든 클라이언트에게 메시지 전송
        break;
      }
      // 클라이언트로부터 데이터 수신
      if (FD_ISSET(fdI, &readFds) && fdI != serverSockFd) {
        int response_len = recv(fdI, bufferRead, 65536, 0);  // 데이터 수신
        // 수신된 데이터 없거나 에러 발생
        if (response_len <= 0) {
          sprintf(bufferWrite, "server: client %d just left\n", clients[fdI].id);  // 메시지 생성
          sendAll(fdI);  // 모든 클라이언트에게 메시지 전송
          FD_CLR(fdI, &activeFds);  // 활성화된 파일 디스크립터 세트에서 제거
          close(fdI);  // 소켓 닫기
          break;
        }
        else {
          // i 는 현재읽고 있는 bufferRead위치 j는 현재 나타내야할 msg위치
          for (int i = 0, j = strlen(clients[fdI].msg); i < response_len; i++, j++) {
            clients[fdI].msg[j] = bufferRead[i];  // 메시지 버퍼에 데이터 저장
            if (clients[fdI].msg[j] == '\n') {  // 줄바꿈 문자 확인
              clients[fdI].msg[j] = '\0';  // 문자열 종료 문자 설정
              sprintf(bufferWrite, "client %d: %s\n", clients[fdI].id, clients[fdI].msg);  // 쓰기버퍼에 보내야할 문자열 넣기
              sendAll(fdI);
              bzero(&clients[fdI].msg, strlen(clients[fdI].msg)); // 메시지 버퍼 초기화
              j = -1;  // 인덱스 초기화
            }
          }
          break;
        }
      }
    }
  }
}
