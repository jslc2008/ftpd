#pragma once

#include <dirent.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace fs
{
/// \brief Print size in human-readable format (KiB, MiB, etc)
/// \param size_ Size to print
std::string printSize (std::uint64_t size_);

/// \brief File I/O object
class File
{
public:
	~File ();

	File ();

	File (File const &that_) = delete;

	/// \brief Move constructor
	/// \param that_ Object to move from
	File (File &&that_);

	File &operator= (File const &that_) = delete;

	/// \brief Move assignment
	/// \param that_ Object to move from
	File &operator= (File &&that_);

	/// \brief bool cast operator
	explicit operator bool () const;

	/// \brief std::FILE* cast operator
	operator std::FILE * () const;

	/// \brief Set buffer size
	/// \param size_ Buffer size
	void setBufferSize (std::size_t size_);

	/// \brief Open file
	/// \param path_ Path to open
	/// \param mode_ Access mode (\sa std::fopen)
	bool open (char const *path_, char const *mode_ = "rb");

	/// \brief Close file
	void close ();

	/// \brief Seek to file position
	/// \param pos_ File position
	/// \param origin_ Reference position (\sa std::fseek)
	ssize_t seek (std::size_t pos_, int origin_);

	/// \brief Read data
	/// \param data_ Output buffer
	/// \param size_ Size to read
	/// \note Can return partial reads
	ssize_t read (void *data_, std::size_t size_);

	/// \brief Read data
	/// \param data_ Output buffer
	/// \param size_ Size to read
	/// \note Fails on partial reads and errors
	bool readAll (void *data_, std::size_t size_);

	/// \brief Write data
	/// \param data_ Input data
	/// \param size_ Size to write
	/// \note Can return partial writes
	ssize_t write (void const *data_, std::size_t size_);

	/// \brief Write data
	/// \param data_ Input data
	/// \param size_ Size to write
	/// \note Fails on partials writes and errors
	bool writeAll (void const *data_, std::size_t size_);

	/// \brief Read data
	/// \tparam T Type to read
	template <typename T>
	T read ()
	{
		T data;
		if (!readAll (&data, sizeof (data)))
			std::abort ();

		return data;
	}

	/// \brief Write data
	/// \tparam T type to write
	/// \param data_ Data to write
	template <typename T>
	void write (T const &data_)
	{
		if (!writeAll (&data_, sizeof (data_)))
			std::abort ();
	}

private:
	/// \brief Underlying std::FILE*
	std::unique_ptr<std::FILE, int (*) (std::FILE *)> m_fp{nullptr, nullptr};

	/// \brief Buffer
	std::unique_ptr<char[]> m_buffer;

	/// \brief Buffer size
	std::size_t m_bufferSize = 0;
};

/// Directory object
class Dir
{
public:
	~Dir ();

	Dir ();

	Dir (Dir const &that_) = delete;

	/// \brief Move constructor
	/// \param that_ Object to move from
	Dir (Dir &&that_);

	Dir &operator= (Dir const &that_) = delete;

	/// \brief Move assignment
	/// \param that_ Object to move from
	Dir &operator= (Dir &&that_);

	/// \brief bool cast operator
	explicit operator bool () const;

	/// \brief DIR* cast operator
	operator DIR * () const;

	/// \brief Open directory
	/// \param path_ Path to open
	bool open (char const *const path_);

	/// \brief Close directory
	void close ();

	/// \brief Read a directory entry
	/// \note Returns nullptr on end-of-directory or error; check errno
	struct dirent *read ();

private:
	/// \brief Underlying DIR*
	std::unique_ptr<DIR, int (*) (DIR *)> m_dp{nullptr, nullptr};
};
}
