#include "platform.h"

#include "imgui_deko3d.h"
#include "imgui_nx.h"

#include "imgui.h"

#include <switch.h>

#include <mutex>
#include <thread>

bool platform::init ()
{
	if (!imgui::nx::init ())
		return false;

	imgui::deko3d::init ();

	return true;
}

bool platform::loop ()
{
	if (!appletMainLoop ())
		return false;

	hidScanInput ();

	auto const keysDown = hidKeysDown (CONTROLLER_P1_AUTO);
	if (keysDown & KEY_PLUS)
		return false;

	imgui::nx::newFrame ();
	imgui::deko3d::newFrame ();
	ImGui::NewFrame ();

	imgui::deko3d::test ();

	return true;
}

void platform::render ()
{
	ImGui::Render ();

	imgui::deko3d::render ();
}

void platform::exit ()
{
	imgui::nx::exit ();
	imgui::deko3d::exit ();
}

///////////////////////////////////////////////////////////////////////////
/// \brief Platform thread pimpl
class platform::Thread::privateData_t
{
public:
	privateData_t () = default;

	/// \brief Parameterized constructor
	/// \param func_ Thread entry point
	privateData_t (std::function<void ()> func_) : thread (func_)
	{
	}

	/// \brief Underlying thread
	std::thread thread;
};

///////////////////////////////////////////////////////////////////////////
platform::Thread::~Thread () = default;

platform::Thread::Thread () : m_d (new privateData_t ())
{
}

platform::Thread::Thread (std::function<void ()> func_) : m_d (new privateData_t (func_))
{
}

platform::Thread::Thread (Thread &&that_) : m_d (new privateData_t ())
{
	std::swap (m_d, that_.m_d);
}

platform::Thread &platform::Thread::operator= (Thread &&that_)
{
	std::swap (m_d, that_.m_d);
	return *this;
}

void platform::Thread::join ()
{
	m_d->thread.join ();
}

void platform::Thread::sleep (std::chrono::milliseconds const timeout_)
{
	std::this_thread::sleep_for (timeout_);
}

///////////////////////////////////////////////////////////////////////////
#define USE_STD_MUTEX 1

/// \brief Platform mutex pimpl
class platform::Mutex::privateData_t
{
public:
#if USE_STD_MUTEX
	/// \brief Underlying mutex
	std::mutex mutex;
#else
	/// \brief Underlying mutex
	::Mutex mutex;
#endif
};

///////////////////////////////////////////////////////////////////////////
platform::Mutex::~Mutex () = default;

platform::Mutex::Mutex () : m_d (new privateData_t ())
{
#if !USE_STD_MUTEX
	mutexInit (&m_d->mutex);
#endif
}

void platform::Mutex::lock ()
{
#if USE_STD_MUTEX
	m_d->mutex.lock ();
#else
	mutexLock (&m_d->mutex);
#endif
}

void platform::Mutex::unlock ()
{
#if USE_STD_MUTEX
	m_d->mutex.unlock ();
#else
	mutexUnlock (&m_d->mutex);
#endif
}
