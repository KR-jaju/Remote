#pragma once

#include <vector>
#include <mutex>
#include <tuple>
#include <map>
#include <set>
#include <array>
#include <thread>
#include <sys/types.h>
#include <cstdint>
#include <atomic>
#include <memory>
#include <condition_variable>

class TCPServer {
public:
	class Callback {
	public:
		virtual ~Callback() = default;
		virtual void	onClientEnter(int fd) = 0;
		virtual void	onClientExit(int fd) = 0;
		virtual void	onReceiveMessage(int fd, void const* data, uint32_t size) = 0;
	};
	TCPServer(uint16_t port);
	TCPServer(TCPServer const&) = delete;
	TCPServer(TCPServer&& other);
	TCPServer&	operator=(TCPServer const&) = delete;
	TCPServer&	operator=(TCPServer other); // Copy-swap
	~TCPServer();
	void	setCallback(Callback *callback);
	void	close(int fd);
	void	write(int fd, void *data, unsigned int size);
	void	start();
	void	stop();
private:
	void	main();
	int		prepareFDSet(fd_set *read_fds, fd_set *write_fds);

	static constexpr int BACKLOG_SIZE = 10;
	struct ClientData {
		std::vector<uint8_t>	buffer;
		bool	is_dirty;
	};

	std::thread	remote_thread;
	int			fd;
	Callback*	callback;
	std::array<int, 2>	wakeup_pipe;

	std::mutex	mutex; // mutex
	std::condition_variable loop_barrier;
	
	std::map<int, ClientData> clients;
	std::set<int>	write_set;
};
