#pragma once

#include <vector>
#include <mutex>
#include <tuple>
#include <map>
#include <array>

class TCPServer {
public:
	class Callback {
	public:
		virtual ~Callback() = default;
		virtual void	onClientEnter(int fd) = 0;
		virtual void	onClientExit(int fd) = 0;
		virtual void	onReceiveMessage(int fd, void const* data, unsigned int size) = 0;
	};
	TCPServer(unsigned short port, Callback *callback);
	TCPServer(TCPServer const&) = delete;
	TCPServer(TCPServer&& other);
	TCPServer&	operator=(TCPServer const&) = delete;
	TCPServer&	operator=(TCPServer other); // Copy-swap
	~TCPServer();
	void	close(int fd);
	void	write(int fd, void *data, unsigned int size);
private:
	void	run();
	int		prepareFDSet(fd_set *read_fds, fd_set *write_fds);

	static constexpr int BACKLOG_SIZE = 10;
	struct ClientData {
		mutable std::mutex	mutex;
		std::vector<unsigned char>	buffer;
		bool	is_dirty;
	};

	int			fd;
	Callback*	callback;
	std::array<int, 2>	wakeup_pipe;
	std::map<int, ClientData> clients;
	std::thread::id	thread_id;
};
