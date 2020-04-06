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

/// \brief Log object
class Log
{
public:
	/// \brief Log level
	enum Level
	{
		DEBUG,
		INFO,
		ERROR,
		COMMAND,
		RESPONSE,
	};

	~Log ();

	/// \brief Draw log
	void draw ();

	/// \brief Create log
	static SharedLog create ();

	/// \brief Bind log
	/// \param log_ Log to bind
	static void bind (SharedLog log_);

	/// \brief Add debug message to bound log
	/// \param fmt_ Message format
	__attribute__ ((format (printf, 1, 2))) static void debug (char const *fmt_, ...);
	/// \brief Add info message to bound log
	/// \param fmt_ Message format
	__attribute__ ((format (printf, 1, 2))) static void info (char const *fmt_, ...);
	/// \brief Add error message to bound log
	/// \param fmt_ Message format
	__attribute__ ((format (printf, 1, 2))) static void error (char const *fmt_, ...);
	/// \brief Add command message to bound log
	/// \param fmt_ Message format
	__attribute__ ((format (printf, 1, 2))) static void command (char const *fmt_, ...);
	/// \brief Add response message to bound log
	/// \param fmt_ Message format
	__attribute__ ((format (printf, 1, 2))) static void response (char const *fmt_, ...);

	/// \brief Add log message to bound log
	/// \param level_ Log level
	/// \param fmt_ Message format
	/// \param ap_ Message arguments
	static void log (Level level_, char const *fmt_, va_list ap_);

	/// \brief Add log message to bound log
	/// \param level_ Log level
	/// \param message_ Message to log
	static void log (Level level_, std::string_view message_);

private:
	Log ();

	/// \brief Add log message
	/// \param level_ Log level
	/// \param fmt_ Message format
	/// \param ap_ Message arguments
	void _log (Level level_, char const *fmt_, va_list ap_);

	/// \brief Log message
	struct Message
	{
		/// \brief Parameterized constructor
		/// \param level_ Log level
		/// \param message_ Log message
		Message (Level const level_, std::string message_)
		    : level (level_), message (std::move (message_))
		{
		}

		/// \brief Log level
		Level level;
		/// \brief Log message
		std::string message;
	};

	/// \brief Log messages
	std::vector<Message> m_messages;

	/// \brief Log lock
	platform::Mutex m_lock;
};
