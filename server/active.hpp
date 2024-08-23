#pragma once

class Remote;

template <typename T>
class active {
public:
	// active(Remote *remote) : remote(remote)
	T const	&operator*() const { return (*this->data); }
	T const	*operator->() const { return (this->data); }
	void	setData(T const& src) {
		*this->data = src;
		// this->remote.set
	}
private:
	Remote 			*remote;
	unsigned int	begin;
};

