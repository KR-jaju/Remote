#include "TCPServer.hpp"

#include <cstring>      // for memset
#include <sys/types.h>  // for socket
// #include <sys/socket.h> // for socket functions
// #include <netinet/in.h> // for sockaddr_in
#include <winsock.h>
#include <stdexcept>
#include <fcntl.h>
#include <io.h>
#include <cstdint>
#include <cstddef>

TCPServer::TCPServer(unsigned short port, Callback *callback)
	: fd(-1), wakeup_pipe({-1, -1}),
	callback(callback), thread_id(std::this_thread::get_id())
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		throw std::runtime_error("Failed to create socket");

	char optval[] = { 1 };
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, optval, sizeof(optval)); // 같은 주소에 다시 바인딩 될 수 있음

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		close(server_fd);
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(server_fd, 10) == -1) {
        close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }
	if (pipe(this->wakeup_pipe) == -1) {
		close(server_fd);
		throw std::runtime_error("Failed to open a pipe");
	}
	this->fd = server_fd;
	this->run();
}

TCPServer::TCPServer(TCPServer&& other) {
	this->fd = other.fd;
	other.fd = -1;
}

TCPServer::~TCPServer() {
	if (this->fd != -1)
		close(this->fd);
}

TCPServer&	TCPServer::operator=(TCPServer other) {
	std::swap(this->fd, other.fd);
	return (*this);
}

void	TCPServer::close(int fd) {
	if (this->clients.count(fd) == 0)
		throw std::runtime_error("client not found");
	this->clients.erase(fd);
	close(fd);
}

void	TCPServer::write(int fd, void *data, unsigned int size) {
	unsigned char*const	begin = static_cast<unsigned char*>(data);
	unsigned char*const	end = begin + size;

	if (this->clients.count(fd) == 0)
		throw std::runtime_error("client not found");
	ClientData& client = this->clients.at(fd);

	client.mutex.lock();
	client.buffer.insert(client.buffer.end(), begin, end);
	client.is_dirty = true;
	client.mutex.unlock();
	if (std::this_thread::get_id() != this->thread_id) { //WAKEUP PIPE

	}
}

void	TCPServer::run() {
	fd_set	read_fds = {};
	fd_set	write_fds = {};

	while (true) {
		int const	nfds = prepareFDSet(&read_fds, &write_fds);
		int const	count = select(nfds, &read_fds, &write_fds, nullptr, nullptr);

		for (const auto &entry : this->clients) {
			int const	fd = entry.first;

			if (FD_ISSET(fd, &read_fds)) { // read 가능 검사
				if (fd == this->fd) { // accept 요청

				} else if (fd == this->wakeup_pipe[0]) { // write 요청

				} else {
					if (this->callback != nullptr)
						continue;
					unsigned char message[1024];
					std::ssize_t size = read(fd, message, 1024);
					if (size == -1)
						throw std::runtime_error("failed to read");
					// this->callback->onReceiveMessage(fd, )
				}
			}
		}
	}
}

int	TCPServer::prepareFDSet(fd_set *read_fds, fd_set *write_fds) {
	int	max_fd = -1;

	FD_ZERO(read_fds);
	FD_ZERO(write_fds);
	FD_SET(this->fd, read_fds);
	FD_SET(this->wakeup_pipe[0], read_fds); // 파이프를 읽는다
	for (auto const &entry : this->clients) {
		int const fd = entry.first;
		ClientData const	&client = entry.second;
		std::lock_guard<std::mutex>	lock_guard(client.mutex);

		max_fd = std::max<int>(max_fd, fd);
		FD_SET(fd, read_fds);
		if (client.is_dirty)
			FD_SET(fd, write_fds);
	}
	return (max_fd + 1);
}
