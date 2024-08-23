#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <variant>
#include <queue>
#include <map>
#include <array>
#include "TCPSocket.hpp"
#include "active.hpp"

#include <winsock.h>

/*
Remote for server
*/

class Remote {
public:
	Remote(short port);
	~Remote();



	int		allocate(unsigned int size);
	void	setDirty(unsigned int from, unsigned int to);
private:
	static constexpr int	MAX_CLIENTS = 20;
	//main thread
	// void	
	//remote thread
	void	threadMain();

	using ClientData = std::tuple<std::vector<unsigned char>, std::mutex>;


	std::thread remote_thread;
	TCPSocket socket;

	std::mutex pool_mutex;

	std::map<int, ClientData> clients;
	fd_set client_fds;
	int	client_fds_max;
};