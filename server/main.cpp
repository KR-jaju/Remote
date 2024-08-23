
#include <iostream>
#include <string>
#include "Remote.hpp"

int	main(int argc, char** argv) {
	if (argc != 2)
		return (1); // port must be given as argument
	short port = std::stoi(argv[1]);
	// TCPSocket chat_socket(12);// socket for chat
	Remote remote(port);

	while (true) { // main loop
		// 1. check for updates (recv)
		// 2. manage remote variables
		// 3. update game logics
		// 4. return result (send)
	}
}
