
#include <iostream>
#include <string>
#include "TCPServer.hpp"
#include "Remote.hpp"

int	main(int argc, char** argv) {
	if (argc != 2) {
		std::cerr << "port must be given as argument" << std::endl;
		return (1);
	}
	short port = std::stoi(argv[1]);
	TCPServer remote_server(port);
	Remote callback;
	remote_server.setCallback(&callback);
	remote_server.start();
}
