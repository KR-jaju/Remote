#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <variant>
#include <queue>
#include <map>
#include <array>
#include "active.hpp"
#include "TCPServer.hpp"

/*
Remote for server
*/

class Remote : public TCPServer::Callback {
public:
	Remote();
	~Remote();
	void	onClientEnter(int fd);
	void	onClientExit(int fd);
	void	onReceiveMessage(int fd, void const* data, uint32_t size);
};