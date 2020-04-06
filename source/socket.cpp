#include "socket.h"

#include "log.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <poll.h>

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

///////////////////////////////////////////////////////////////////////////
Socket::~Socket ()
{
	if (m_listening)
		Log::info ("Stop listening on [%s]:%u\n", m_sockName.name (), m_sockName.port ());

	if (m_connected)
		Log::info ("Closing connection to [%s]:%u\n", m_peerName.name (), m_peerName.port ());

	if (::close (m_fd) != 0)
		Log::error ("close: %s\n", std::strerror (errno));
}

Socket::Socket (int const fd_) : m_fd (fd_), m_listening (false), m_connected (false)
{
}

Socket::Socket (int const fd_, SockAddr const &sockName_, SockAddr const &peerName_)
    : m_sockName (sockName_),
      m_peerName (peerName_),
      m_fd (fd_),
      m_listening (false),
      m_connected (true)
{
}

UniqueSocket Socket::accept ()
{
	SockAddr addr;
	socklen_t addrLen = sizeof (struct sockaddr_storage);

	auto const fd = ::accept (m_fd, addr, &addrLen);
	if (fd < 0)
	{
		Log::error ("accept: %s\n", std::strerror (errno));
		return nullptr;
	}

	Log::info ("Accepted connection from [%s]:%u\n", addr.name (), addr.port ());
	return UniqueSocket (new Socket (fd, m_sockName, addr));
}

int Socket::atMark ()
{
	auto const rc = ::sockatmark (m_fd);
	if (rc < 0)
		Log::error ("sockatmark: %s\n", std::strerror (errno));

	return rc;
}

bool Socket::bind (SockAddr const &addr_)
{
	switch (static_cast<struct sockaddr_storage const &> (addr_).ss_family)
	{
	case AF_INET:
		if (::bind (m_fd, addr_, sizeof (struct sockaddr_in)) != 0)
		{
			Log::error ("bind: %s\n", std::strerror (errno));
			return false;
		}
		break;

#ifndef _3DS
	case AF_INET6:
		if (::bind (m_fd, addr_, sizeof (struct sockaddr_in6)) != 0)
		{
			Log::error ("bind: %s\n", std::strerror (errno));
			return false;
		}
		break;
#endif

	default:
		errno = EINVAL;
		Log::error ("bind: %s\n", std::strerror (errno));
		break;
	}

	if (addr_.port () == 0)
	{
		// get socket name due to request for ephemeral port
		socklen_t addrLen = sizeof (struct sockaddr_storage);
		if (::getsockname (m_fd, m_sockName, &addrLen) != 0)
			Log::error ("getsockname: %s\n", std::strerror (errno));
	}
	else
		m_sockName = addr_;

	return true;
}

bool Socket::connect (SockAddr const &addr_)
{
	if (::connect (m_fd, addr_, sizeof (struct sockaddr_storage)) != 0)
	{
		if (errno != EINPROGRESS)
			Log::error ("connect: %s\n", std::strerror (errno));
		else
		{
			m_peerName  = addr_;
			m_connected = true;
			Log::info ("Connecting to [%s]:%u\n", addr_.name (), addr_.port ());
		}
		return false;
	}

	m_peerName  = addr_;
	m_connected = true;
	Log::info ("Connected to [%s]:%u\n", addr_.name (), addr_.port ());
	return true;
}

bool Socket::listen (int const backlog_)
{
	if (::listen (m_fd, backlog_) != 0)
	{
		Log::error ("listen: %s\n", std::strerror (errno));
		return false;
	}

	m_listening = true;
	return true;
}

bool Socket::shutdown (int const how_)
{
	if (::shutdown (m_fd, how_) != 0)
	{
		Log::info ("shutdown: %s\n", std::strerror (errno));
		return false;
	}

	return true;
}

bool Socket::setLinger (bool const enable_, std::chrono::seconds const time_)
{
	struct linger linger;
	linger.l_onoff  = enable_;
	linger.l_linger = time_.count ();

	if (::setsockopt (m_fd, SOL_SOCKET, SO_LINGER, &linger, sizeof (linger)) != 0)
	{
		Log::error ("setsockopt(SO_LINGER, %s, %lus): %s\n",
		    enable_ ? "on" : "off",
		    static_cast<unsigned long> (time_.count ()),
		    std::strerror (errno));
		return false;
	}

	return true;
}

bool Socket::setNonBlocking (bool const nonBlocking_)
{
	auto flags = ::fcntl (m_fd, F_GETFL, 0);
	if (flags == -1)
	{
		Log::error ("fcntl(F_GETFL): %s\n", std::strerror (errno));
		return false;
	}

	if (nonBlocking_)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	if (::fcntl (m_fd, F_SETFL, flags) != 0)
	{
		Log::error ("fcntl(F_SETFL, %d): %s\n", flags, std::strerror (errno));
		return false;
	}

	return true;
}

bool Socket::setReuseAddress (bool const reuse_)
{
	int reuse = reuse_;
	if (::setsockopt (m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse)) != 0)
	{
		Log::error (
		    "setsockopt(SO_REUSEADDR, %s): %s\n", reuse_ ? "yes" : "no", std::strerror (errno));
		return false;
	}

	return true;
}

bool Socket::setRecvBufferSize (std::size_t const size_)
{
	int size = size_;
	if (::setsockopt (m_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof (size)) != 0)
	{
		Log::error ("setsockopt(SO_RCVBUF, %zu): %s\n", size_, std::strerror (errno));
		return false;
	}

	return true;
}

bool Socket::setSendBufferSize (std::size_t const size_)
{
	int size = size_;
	if (::setsockopt (m_fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof (size)) != 0)
	{
		Log::error ("setsockopt(SO_SNDBUF, %zu): %s\n", size_, std::strerror (errno));
		return false;
	}

	return true;
}

ssize_t Socket::read (void *const buffer_, std::size_t const size_, bool const oob_)
{
	assert (buffer_);
	assert (size_);
	auto const rc = ::recv (m_fd, buffer_, size_, oob_ ? MSG_OOB : 0);
	if (rc < 0 && errno != EWOULDBLOCK)
		Log::error ("recv: %s\n", std::strerror (errno));

	return rc;
}

ssize_t Socket::read (IOBuffer &buffer_, bool const oob_)
{
	auto const rc = read (buffer_.freeArea (), buffer_.freeSize (), oob_);
	if (rc > 0)
		buffer_.markUsed (rc);

	return rc;
}

ssize_t Socket::write (void const *const buffer_, std::size_t const size_)
{
	assert (buffer_);
	assert (size_);
	auto const rc = ::send (m_fd, buffer_, size_, 0);
	if (rc < 0 && errno != EWOULDBLOCK)
		Log::error ("send: %s\n", std::strerror (errno));

	return rc;
}

ssize_t Socket::write (IOBuffer &buffer_)
{
	auto const rc = write (buffer_.usedArea (), buffer_.usedSize ());
	if (rc > 0)
		buffer_.markFree (rc);

	return rc;
}

SockAddr const &Socket::sockName () const
{
	return m_sockName;
}

SockAddr const &Socket::peerName () const
{
	return m_peerName;
}

UniqueSocket Socket::create ()
{
	auto const fd = ::socket (AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		Log::error ("socket: %s\n", std::strerror (errno));
		return nullptr;
	}

	return UniqueSocket (new Socket (fd));
}

int Socket::poll (PollInfo *const info_,
    std::size_t const count_,
    std::chrono::milliseconds const timeout_)
{
	if (count_ == 0)
		return 0;

	auto const pfd = std::make_unique<struct pollfd[]> (count_);
	for (std::size_t i = 0; i < count_; ++i)
	{
		pfd[i].fd      = info_[i].socket.get ().m_fd;
		pfd[i].events  = info_[i].events;
		pfd[i].revents = 0;
	}

	auto const rc = ::poll (pfd.get (), count_, timeout_.count ());
	if (rc < 0)
	{
		Log::error ("poll: %s\n", std::strerror (errno));
		return rc;
	}

	for (std::size_t i = 0; i < count_; ++i)
		info_[i].revents = pfd[i].revents;

	return rc;
}
