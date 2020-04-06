#pragma once

#include "ftpSession.h"
#include "log.h"
#include "platform.h"
#include "socket.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

class FtpServer;
using UniqueFtpServer = std::unique_ptr<FtpServer>;

/// \brief FTP server
class FtpServer
{
public:
	~FtpServer ();

	/// \brief Draw server and all of its sessions
	void draw ();

	/// \brief Create server
	/// \param port_ Port to listen on
	static UniqueFtpServer create (std::uint16_t port_);

	/// \brief Update free space
	static void updateFreeSpace ();

	/// \brief Server start time
	static std::time_t startTime ();

private:
	/// \brief Paramterized constructor
	/// \param port_ Port to listen on
	FtpServer (std::uint16_t port_);

	/// \brief Handle when start button is pressed
	void handleStartButton ();

	/// \brief Handle when stop button is pressed
	void handleStopButton ();

	/// \brief Server loop
	void loop ();

	/// \brief Thread entry point
	void threadFunc ();

	/// \brief Thread
	platform::Thread m_thread;

	/// \brief Mutex
	platform::Mutex m_lock;

	/// \brief Listen socket
	UniqueSocket m_socket;

	/// \brief ImGui window name
	std::string m_name;

	/// \brief Log
	SharedLog m_log;

	/// \brief Sessions
	std::vector<UniqueFtpSession> m_sessions;

	/// \brief Port to listen on
	std::uint16_t const m_port;

	/// \brief Whether thread should quit
	std::atomic<bool> m_quit;
};
