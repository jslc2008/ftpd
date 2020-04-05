#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdint>

class SockAddr
{
public:
	~SockAddr ();

	SockAddr ();

	SockAddr (SockAddr const &that_);

	SockAddr (SockAddr &&that_);

	SockAddr &operator= (SockAddr const &that_);

	SockAddr &operator= (SockAddr &&that_);

	SockAddr (struct sockaddr const &addr_);

	SockAddr (struct sockaddr_in const &addr_);

#ifndef _3DS
	SockAddr (struct sockaddr_in6 const &addr_);
#endif

	SockAddr (struct sockaddr_storage const &addr_);

	operator struct sockaddr_in const & () const;

#ifndef _3DS
	operator struct sockaddr_in6 const & () const;
#endif

	operator struct sockaddr_storage const & () const;

	operator struct sockaddr * ();
	operator struct sockaddr const * () const;

	std::uint16_t port () const;
	char const *name (char *buffer_, std::size_t size_) const;
	char const *name () const;

private:
	struct sockaddr_storage m_addr = {};
};
