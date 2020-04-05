#pragma once

#include "platform.h"

#include <cstdarg>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class Log;
using SharedLog = std::shared_ptr<Log>;
using WeakLog   = std::weak_ptr<Log>;

class Log
{
public:
	enum Level
	{
		DEBUG,
		INFO,
		ERROR,
		COMMAND,
		RESPONSE,
	};

	~Log ();

	void draw ();

	static SharedLog create ();
	static void bind (SharedLog log_);

	__attribute__ ((format (printf, 1, 2))) static void debug (char const *fmt_, ...);
	__attribute__ ((format (printf, 1, 2))) static void info (char const *fmt_, ...);
	__attribute__ ((format (printf, 1, 2))) static void error (char const *fmt_, ...);
	__attribute__ ((format (printf, 1, 2))) static void command (char const *fmt_, ...);
	__attribute__ ((format (printf, 1, 2))) static void response (char const *fmt_, ...);

	static void log (Level level_, char const *fmt_, va_list ap_);
	static void log (Level level_, std::string_view message_);

private:
	Log ();

	void _log (Level level_, char const *fmt_, va_list ap_);

	struct Message
	{
		Message (Level const level_, std::string message_)
		    : level (level_), message (std::move (message_))
		{
		}

		Level level;
		std::string message;
	};

	std::vector<Message> m_messages;
	platform::Mutex m_lock;
};
