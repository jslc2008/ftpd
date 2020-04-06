#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdint>

/// \brief Socket address
class SockAddr
{
public:
	~SockAddr ();

	SockAddr ();

	/// \brief Copy constructor
	/// \param that_ Object to copy
	SockAddr (SockAddr const &that_);

	/// \brief Move constructor
	/// \param that_ Object to move from
	SockAddr (SockAddr &&that_);

	/// \brief Copy assignment
	/// \param that_ Object to copy
	SockAddr &operator= (SockAddr const &that_);

	/// \brief Move assignment
	/// \param that_ Object to move from
	SockAddr &operator= (SockAddr &&that_);

	/// \param Parameterized constructor
	/// \param addr_ Address
	SockAddr (struct sockaddr const &addr_);

	/// \param Parameterized constructor
	/// \param addr_ Address
	SockAddr (struct sockaddr_in const &addr_);

#ifndef _3DS
	/// \param Parameterized constructor
	/// \param addr_ Address
	SockAddr (struct sockaddr_in6 const &addr_);
#endif

	/// \param Parameterized constructor
	/// \param addr_ Address
	SockAddr (struct sockaddr_storage const &addr_);

	/// \param sockaddr_in cast operator
	operator struct sockaddr_in const & () const;

#ifndef _3DS
	/// \param sockaddr_in6 cast operator
	operator struct sockaddr_in6 const & () const;
#endif

	/// \param sockaddr_storage cast operator
	operator struct sockaddr_storage const & () const;

	/// \param sockaddr* cast operator
	operator struct sockaddr * ();
	/// \param sockaddr const* cast operator
	operator struct sockaddr const * () const;

	/// \brief Address port
	std::uint16_t port () const;

	/// \brief Address name
	/// \param buffer_
	/// \param size_ Size of buffer_
	/// \retval buffer_ success
	/// \retval nullptr failure
	char const *name (char *buffer_, std::size_t size_) const;

	/// \brief Address name
	/// \retval nullptr failure
	/// \note This function is not reentrant
	char const *name () const;

private:
	/// \brief Address storage
	struct sockaddr_storage m_addr = {};
};
