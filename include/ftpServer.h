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

class FtpServer
{
public:
	~FtpServer ();

	void draw ();

	static UniqueFtpServer create (std::uint16_t port_);

	static void updateFreeSpace ();

	static std::time_t startTime ();

private:
	FtpServer (std::uint16_t port_);

	void handleStartButton ();
	void handleStopButton ();

	void loop ();
	void threadFunc ();

	platform::Thread m_thread;
	platform::Mutex m_lock;

	UniqueSocket m_socket;

	std::string m_name;

	SharedLog m_log;

	std::vector<UniqueFtpSession> m_sessions;

	std::uint16_t m_port;

	std::atomic<bool> m_quit;
};
