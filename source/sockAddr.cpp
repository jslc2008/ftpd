#include "sockAddr.h"

#include <arpa/inet.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

///////////////////////////////////////////////////////////////////////////
SockAddr::~SockAddr () = default;

SockAddr::SockAddr () = default;

SockAddr::SockAddr (SockAddr const &that_) = default;

SockAddr::SockAddr (SockAddr &&that_) = default;

SockAddr &SockAddr::operator= (SockAddr const &that_) = default;

SockAddr &SockAddr::operator= (SockAddr &&that_) = default;

SockAddr::SockAddr (struct sockaddr const &addr_)
{
	switch (addr_.sa_family)
	{
	case AF_INET:
		std::memcpy (&m_addr, &addr_, sizeof (struct sockaddr_in));
		break;

#ifndef _3DS
	case AF_INET6:
		std::memcpy (&m_addr, &addr_, sizeof (struct sockaddr_in6));
		break;
#endif

	default:
		std::abort ();
		break;
	}
}

SockAddr::SockAddr (struct sockaddr_in const &addr_)
    : SockAddr (reinterpret_cast<struct sockaddr const &> (addr_))
{
	assert (m_addr.ss_family == AF_INET);
}

#ifndef _3DS
SockAddr::SockAddr (struct sockaddr_in6 const &addr_)
    : SockAddr (reinterpret_cast<struct sockaddr const &> (addr_))
{
	assert (m_addr.ss_family == AF_INET6);
}
#endif

SockAddr::SockAddr (struct sockaddr_storage const &addr_)
    : SockAddr (reinterpret_cast<struct sockaddr const &> (addr_))
{
}

SockAddr::operator struct sockaddr_in const & () const
{
	assert (m_addr.ss_family == AF_INET);
	return reinterpret_cast<struct sockaddr_in const &> (m_addr);
}

#ifndef _3DS
SockAddr::operator struct sockaddr_in6 const & () const
{
	assert (m_addr.ss_family == AF_INET6);
	return reinterpret_cast<struct sockaddr_in6 const &> (m_addr);
}
#endif

SockAddr::operator struct sockaddr_storage const & () const
{
	return m_addr;
}

SockAddr::operator struct sockaddr * ()
{
	return reinterpret_cast<struct sockaddr *> (&m_addr);
}

SockAddr::operator struct sockaddr const * () const
{
	return reinterpret_cast<struct sockaddr const *> (&m_addr);
}

std::uint16_t SockAddr::port () const
{
	switch (m_addr.ss_family)
	{
	case AF_INET:
		return ntohs (reinterpret_cast<struct sockaddr_in const *> (&m_addr)->sin_port);

#ifndef _3DS
	case AF_INET6:
		return ntohs (reinterpret_cast<struct sockaddr_in6 const *> (&m_addr)->sin6_port);
#endif

	default:
		std::abort ();
		break;
	}
}

char const *SockAddr::name (char *buffer_, std::size_t size_) const
{
	switch (m_addr.ss_family)
	{
	case AF_INET:
		return inet_ntop (AF_INET,
		    &reinterpret_cast<struct sockaddr_in const *> (&m_addr)->sin_addr,
		    buffer_,
		    size_);

#ifndef _3DS
	case AF_INET6:
		return inet_ntop (AF_INET6,
		    &reinterpret_cast<struct sockaddr_in6 const *> (&m_addr)->sin6_addr,
		    buffer_,
		    size_);
#endif

	default:
		std::abort ();
		break;
	}
}

char const *SockAddr::name () const
{
#if defined(_3DS)
	thread_local static char buffer[INET_ADDRSTRLEN];
#else
	thread_local static char buffer[INET6_ADDRSTRLEN];
#endif

	return name (buffer, sizeof (buffer));
}
