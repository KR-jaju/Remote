#include "Remote.hpp"
#include <io.h>

Remote::Remote(short port)
	: remote_thread(&Remote::threadMain, this),
	socket(port),
	pool_mutex()
{

}


int	Remote::allocate(unsigned int size) {
	/*
	1. pool mutex lock, allocate, pool mutex unlock
	2. client set mutex lock, send, unlock
	*/
}

/*
1. while - select로 입력을 기다림
2. 보낼 것이 있으면 보냄
3. 입력을 해석하고 실행함
4. 결과를 큐에 저장함

클라이언트는 계속 대기중이므로 항상 다시 보낼 수 있음
*/
void	Remote::threadMain() {
	fd_set	read_fds = {};
	fd_set	write_fds = {};

	while(true) {
		int count = select(Remote::MAX_CLIENTS + 2, &read_fds, &write_fds, nullptr, nullptr); // 서버 소켓과 파이프 + 모든 클라이언트

		for (const auto &entry : this->clients) {
			int const	fd = entry.first;

			if (FD_ISSET(fd, &read_fds)) { // read 가능 검사

			}
		}
		for (const auto &entry : this->clients) {
			int const	fd = entry.first;

			if (FD_ISSET(fd, &write_fds)) { // write 가능 검사
				
			}
		}
	}
}
