#pragma once

#include <cstddef>
#include <memory>

class IOBuffer
{
public:
	~IOBuffer ();

	IOBuffer (std::size_t size_);

	char *freeArea () const;
	std::size_t freeSize () const;
	void markFree (std::size_t size_);

	char *usedArea () const;
	std::size_t usedSize () const;
	void markUsed (std::size_t size_);

	bool empty () const;
	std::size_t capacity () const;
	void clear ();
	void coalesce ();

private:
	std::unique_ptr<char[]> m_buffer;
	std::size_t const m_size;
	std::size_t m_start = 0;
	std::size_t m_end   = 0;
};
