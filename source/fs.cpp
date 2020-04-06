#include "fs.h"

#include <cinttypes>
#include <cstdio>

std::string fs::printSize (std::uint64_t const size_)
{
	constexpr std::uint64_t const KiB = 1024;
	constexpr std::uint64_t const MiB = 1024 * KiB;
	constexpr std::uint64_t const GiB = 1024 * MiB;
	constexpr std::uint64_t const TiB = 1024 * GiB;
	constexpr std::uint64_t const PiB = 1024 * TiB;
	constexpr std::uint64_t const EiB = 1024 * PiB;

	char buffer[64] = {};

	for (auto const &[name, bin] : {
	         // clang-format off
	         std::make_pair ("EiB", EiB),
	         std::make_pair ("PiB", PiB),
	         std::make_pair ("TiB", TiB),
	         std::make_pair ("GiB", GiB),
	         std::make_pair ("MiB", MiB),
	         std::make_pair ("KiB", KiB)
	         // clang-format on
	     })
	{
		// get the integral portion of the number
		auto const whole = size_ / bin;
		if (size_ >= 100 * bin)
		{
			// >= 100, print xxxXiB
			std::sprintf (buffer, "%" PRIu64 "%s", whole, name);
			return buffer;
		}

		// get the fractional portion of the number
		auto const frac = size_ - whole * bin;
		if (size_ >= 10 * bin)
		{
			// >= 10, print xx.xXiB
			std::sprintf (buffer, "%" PRIu64 ".%" PRIu64 "%s", whole, frac * 10 / bin, name);
			return buffer;
		}

		if (size_ >= 1000 * (bin / KiB))
		{
			// >= 1000 of lesser bin, print x.xxXiB
			std::sprintf (buffer, "%" PRIu64 ".%02" PRIu64 "%s", whole, frac * 100 / bin, name);
			return buffer;
		}
	}

	// < 1KiB, just print the number
	std::sprintf (buffer, "%" PRIu64 "B", size_);
	return buffer;
}

///////////////////////////////////////////////////////////////////////////
fs::File::~File () = default;

fs::File::File () = default;

fs::File::File (File &&that_) = default;

fs::File &fs::File::operator= (File &&that_) = default;

fs::File::operator bool () const
{
	return static_cast<bool> (m_fp);
}

fs::File::operator FILE * () const
{
	return m_fp.get ();
}

void fs::File::setBufferSize (std::size_t const size_)
{
	if (m_bufferSize != size_)
	{
		m_buffer     = std::make_unique<char[]> (size_);
		m_bufferSize = size_;
	}

	if (m_fp)
		std::setvbuf (m_fp.get (), m_buffer.get (), _IOFBF, m_bufferSize);
}

bool fs::File::open (char const *const path_, char const *const mode_)
{
	auto const fp = std::fopen (path_, mode_);
	if (!fp)
		return false;

	m_fp = std::unique_ptr<std::FILE, int (*) (std::FILE *)> (fp, &std::fclose);

	if (m_buffer)
		std::setvbuf (m_fp.get (), m_buffer.get (), _IOFBF, m_bufferSize);

	return true;
}

void fs::File::close ()
{
	m_fp.reset ();
}

ssize_t fs::File::seek (std::size_t const pos_, int const origin_)
{
	return std::fseek (m_fp.get (), pos_, origin_);
}

ssize_t fs::File::read (void *const data_, std::size_t const size_)
{
	return std::fread (data_, 1, size_, m_fp.get ());
}

bool fs::File::readAll (void *const data_, std::size_t const size_)
{
	auto p            = static_cast<char *> (data_);
	std::size_t bytes = 0;

	while (bytes < size_)
	{
		auto const rc = read (p, size_ - bytes);
		if (rc <= 0)
			return false;

		p += rc;
		bytes += rc;
	}

	return true;
}

ssize_t fs::File::write (void const *const data_, std::size_t const size_)
{
	return std::fwrite (data_, 1, size_, m_fp.get ());
}

bool fs::File::writeAll (void const *const data_, std::size_t const size_)
{
	auto p            = static_cast<char const *> (data_);
	std::size_t bytes = 0;

	while (bytes < size_)
	{
		auto const rc = write (p, size_ - bytes);
		if (rc <= 0)
			return false;

		p += rc;
		bytes += rc;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////
fs::Dir::~Dir () = default;

fs::Dir::Dir () = default;

fs::Dir::Dir (Dir &&that_) = default;

fs::Dir &fs::Dir::operator= (Dir &&that_) = default;

fs::Dir::operator bool () const
{
	return static_cast<bool> (m_dp);
}

fs::Dir::operator DIR * () const
{
	return m_dp.get ();
}

bool fs::Dir::open (char const *const path_)
{
	auto const dp = ::opendir (path_);
	if (!dp)
		return false;

	m_dp = std::unique_ptr<DIR, int (*) (DIR *)> (dp, &::closedir);
	return true;
}

void fs::Dir::close ()
{
	m_dp.reset ();
}

struct dirent *fs::Dir::read ()
{
	errno = 0;
	return ::readdir (m_dp.get ());
}
