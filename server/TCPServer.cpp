#include "TCPServer.hpp"

#include <cstring>      // for memset
#include <sys/types.h>  // for socket
#include <sys/socket.h> // for socket functions
#include <netinet/in.h> // for sockaddr_in
// #include <winsock.h>
#include <unistd.h>
// #include <stdexcept>
#include <exception>
#include <fcntl.h>
// #include <io.h>
#include <cstdint>
#include <cstddef>
#include <sys/types.h>
#include <iostream>
#include <utility>

TCPServer::TCPServer(unsigned short port)
	: fd(-1), wakeup_pipe({-1, -1}),
	callback(callback), remote_thread()
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		throw std::runtime_error("Failed to create socket");

	int	optval = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
		throw std::runtime_error("setsockopt failed");
	} // 같은 주소에 다시 바인딩 될 수 있음

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (::bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		::close(server_fd);
		std::cerr << "error: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(server_fd, 10) == -1) {
        ::close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
	if (pipe(this->wakeup_pipe.data()) == -1) {
		::close(server_fd);
		throw std::runtime_error("Failed to open a pipe");
	}
	this->fd = server_fd;
	this->remote_thread = std::thread(&TCPServer::main, *this);
}

TCPServer::TCPServer(TCPServer&& other) {
	this->fd = other.fd;
	other.fd = -1;
}

TCPServer::~TCPServer() {
	if (this->fd != -1)
		::close(this->fd);
}

TCPServer&	TCPServer::operator=(TCPServer other) {
	std::swap(this->fd, other.fd);
	return (*this);
}

void	TCPServer::setCallback(Callback *callback) {
	std::thread::id const	thread_id = this->thread_id.load();

	if (thread_id != std::thread::id())
		throw std::runtime_error("Cannot call setCallback on a running server");
	this->callback = callback;
}

void	TCPServer::close(int fd) {
	if (this->clients.count(fd) == 0)
		throw std::runtime_error("client not found");
	this->clients.erase(fd);
	::close(fd);
}

void	TCPServer::write(int fd, void *data, unsigned int size) {
	uint8_t*const	begin = static_cast<uint8_t*>(data);
	uint8_t*const	end = begin + size;

	if (this->clients.count(fd) == 0)
		throw std::runtime_error("client not found");
	{
		std::lock_guard<std::mutex>	guard(this->clients_mutex);
		ClientData& client = this->clients.at(fd);

		client.buffer.insert(client.buffer.end(), begin, end);
		client.is_dirty = true;
	}
	if (std::this_thread::get_id() != this->remote_thread.get_id()) { //wakeup pipe
	
	}
}

// void	TCPServer::run() {
void	TCPServer::main() {
	fd_set	read_fds = {};
	fd_set	write_fds = {};
	std::thread::id thread_id;

	std::cout << "Server started" << std::endl;
	while (true) {
		int const	nfds = this->prepareFDSet(&read_fds, &write_fds);
		int const	count = select(nfds, &read_fds, &write_fds, nullptr, nullptr);

		std::cout << "Select unblocked" << std::endl;

		if (FD_ISSET(this->fd, &read_fds)) { // accept 요청
			std::cout << "Client accept request" << std::endl;

			sockaddr_in	address;
			int addrlen = sizeof(address);
			int client_fd = accept(this->fd, (sockaddr *)&address, (socklen_t *)&addrlen);
			if (client_fd == -1)
				throw std::runtime_error("accept failed. why?");
			fcntl(client_fd, F_SETFL, O_NONBLOCK);
			{
				std::lock_guard<std::mutex> guard(this->mutex);

				this->clients.emplace(client_fd, TCPServer::ClientData{std::vector<uint8_t>(), false });
			}
		}
		if (FD_ISSET(this->wakeup_pipe[0], &read_fds)) { // write 요청
			if (this->callback == nullptr)
				continue;
		}
		for (const auto &entry : this->clients) {
			int const	fd = entry.first;

			if (FD_ISSET(fd, &read_fds)) { // read 가능 검사
				if (this->callback == nullptr)
					continue;
				unsigned char message[1024];
				ssize_t size = ::read(fd, message, 1024);
				if (size == -1)
					throw std::runtime_error("failed to read");
				if (size == 0) {
					this->clients.erase(fd); // 클라이언트 목록에서 제거
					::close(fd); // fd닫기
					std::cerr << "client out";
					continue;
				}
				::write(1, message, size);
				// this->callback->onReceiveMessage(fd, )
			}
		}
	}
}

void	TCPServer::start() {
	this->loop_barrier.notify_one();
}

void	TCPServer::stop() {
	
}

int	TCPServer::prepareFDSet(fd_set *read_fds, fd_set *write_fds) {
	int	max_fd = std::max(this->fd, this->wakeup_pipe[0]);

	FD_ZERO(read_fds);
	FD_ZERO(write_fds);
	FD_SET(this->fd, read_fds); // 리스너 소켓
	FD_SET(this->wakeup_pipe[0], read_fds); // 웨이크업 파이프
	for (auto const &entry : this->clients) {
		int const fd = entry.first;

		FD_SET(fd, read_fds);
		max_fd = std::max<int>(max_fd, fd);
	}
	{
		std::lock_guard<std::mutex> guard(this->mutex);

		for (auto const &fd : this->write_set) {
			FD_SET(fd, write_fds);
		}
		this->write_set.clear();
	}
	return (max_fd + 1);
}
