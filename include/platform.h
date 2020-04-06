#pragma once

#ifdef _3DS
#include <3ds.h>
#endif

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

namespace platform
{
/// \brief Initialize platform
bool init ();

/// \brief Platform loop
bool loop ();

/// \brief Platform render
void render ();

/// \brief Deinitialize platform
void exit ();

#ifdef _3DS
/// \brief Steady clock
struct steady_clock
{
	/// \brief Type representing number of ticks
	using rep = std::uint64_t;

	/// \brief Type representing ratio of clock period in seconds
	using period = std::ratio<1, SYSCLOCK_ARM11>;

	/// \brief Duration type
	using duration = std::chrono::duration<rep, period>;

	/// \brief Timestamp type
	using time_point = std::chrono::time_point<steady_clock>;

	/// \brief Whether clock is steady
	constexpr static bool is_steady = true;

	/// \brief Current timestamp
	static time_point now () noexcept
	{
		return time_point (duration (svcGetSystemTick ()));
	}
};
#else
/// \brief Steady clock
using steady_clock = std::chrono::steady_clock;
#endif

/// \brief Platform thread
class Thread
{
public:
	~Thread ();
	Thread ();

	/// \brief Parameterized constructor
	/// \param func_ Thread entrypoint
	Thread (std::function<void ()> func_);

	Thread (Thread const &that_) = delete;

	/// \brief Move constructor
	/// \param that_ Object to move from
	Thread (Thread &&that_);

	Thread &operator= (Thread const &that_) = delete;

	/// \brief Move assignment
	/// \param that_ Object to move from
	Thread &operator= (Thread &&that_);

	/// \brief Join thread
	void join ();

	/// \brief Suspend current thread
	/// \param timeout_ Minimum time to sleep
	static void sleep (std::chrono::milliseconds timeout_);

private:
	class privateData_t;

	/// \brief pimpl
	std::unique_ptr<privateData_t> m_d;
};

/// \brief Platform mutex
class Mutex
{
public:
	~Mutex ();
	Mutex ();

	/// \brief Lock mutex
	void lock ();

	/// \brief Unlock mutex
	void unlock ();

private:
	class privateData_t;

	/// \brief pimpl
	std::unique_ptr<privateData_t> m_d;
};
}
