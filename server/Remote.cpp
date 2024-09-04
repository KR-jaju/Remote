#include "Remote.hpp"
#include <iostream>

Remote::Remote() {
	
}

Remote::~Remote() {

}

void    Remote::onClientEnter(int fd) {
    std::cout << "Client entered" << std::endl;
}

void    Remote::onClientExit(int fd) {
    
}

void    Remote::onReceiveMessage(int fd, void const *data, uint32_t size) {

}

