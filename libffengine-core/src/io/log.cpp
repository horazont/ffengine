/**********************************************************************
File name: log.cpp
This file is part of: SCC (working title)

LICENSE

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

FEEDBACK & QUESTIONS

For feedback and questions about SCC please e-mail one of the authors named in
the AUTHORS file.
**********************************************************************/
#include "ffengine/io/log.hpp"

#include <cstdarg>
#include <functional>
#include <iomanip>
#include <iostream>
#include <functional>

namespace std {

template<>
struct hash<::io::LogLevel>
{
    typedef typename hash<uint64_t>::result_type result_type;
    typedef ::io::LogLevel Key;

    result_type operator()(Key value) const
    {
        return m_int_hash(uint64_t(value));
    }

private:
    hash<uint64_t> m_int_hash;

};

}

namespace io {

const std::size_t AWESOME_BUFFER_SIZE = 512;

std::string awesomef(const char *message, std::va_list args)
{
    std::string buffer(AWESOME_BUFFER_SIZE, '\0');

    std::va_list tmp;
    va_copy(tmp, args);
    std::size_t formatted_len = vsnprintf(&buffer.front(),
                                          buffer.size()+1,
                                          message,
                                          tmp);
    va_end(tmp);

    if (formatted_len > buffer.size())
    {
        buffer.resize(formatted_len);
        vsnprintf(&buffer.front(), buffer.size()+1, message, args);
    } else {
        buffer.resize(formatted_len);
    }

    return buffer;
}

std::string vawesomef(const char *message, ...)
{
    std::va_list args;
    va_start(args, message);
    std::string result = awesomef(message, args);
    va_end(args);
    return result;
}

const char *level_name(LogLevel level)
{
    switch (level)
    {
    case LOG_DEBUG: return "debug";
    case LOG_INFO: return "info";
    case LOG_WARNING: return "warning";
    case LOG_ERROR: return "error";
    case LOG_EXCEPTION: return "fatal";
    case LOG_ALL:
    case LOG_NOTHING:
    default:
    {
        return "unknown";
    }
    }
}

const char *level_ansi_color(LogLevel level)
{
    static std::unordered_map<LogLevel, const char *> ansi({
        std::make_pair(LOG_DEBUG, "\033[38;5;240m"),
        std::make_pair(LOG_INFO, "\033[38;5;33m"),
        std::make_pair(LOG_WARNING, "\033[38;5;214m"),
        std::make_pair(LOG_ERROR, "\033[38;5;202m"),
        std::make_pair(LOG_EXCEPTION, "\033[1;38;5;196m")
    });

    auto iter = ansi.find(level);
    if (iter != ansi.end()) {
        return iter->second;
    }

    return "";
}


LogPipe::LogPipe(LogLevel level, const Logger &dest):
    std::ostringstream(),
    m_level(level),
    m_dest(dest)
{

}

void LogPipe::submit()
{
    m_dest.log(m_level, str());
    delete this;
}


LogSink::LogSink():
    m_level(LOG_ALL)
{

}

LogSink::~LogSink()
{

}

void LogSink::log(const LogRecord &record)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (record.level < m_level) {
            return;
        }
    }

    log_direct(record);
}

void LogSink::set_level(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}


LogTTYSink::LogTTYSink():
    LogSink()
{

}

void LogTTYSink::log_direct(const LogRecord &record)
{

    std::cout << level_ansi_color(record.level)
              << "["
              << std::setprecision(6) << std::setw(12) << std::fixed << record.rel_timestamp.count()
              << "] ["
              << record.logger_fullpath
              << "] ["
              << level_name(record.level)
              << "] \033[0m"
              << record.message
              << std::endl;
}


LogAsynchronousSink::LogAsynchronousSink(std::unique_ptr<LogSink> &&backend):
    LogSink(),
    m_backend(std::move(backend)),
    m_state_mutex(),
    m_terminated(false),
    m_log_queue(),
    m_wakeup_cv(),
    m_logger(std::bind(&LogAsynchronousSink::thread_impl, this))
{

}

LogAsynchronousSink::~LogAsynchronousSink()
{
    m_terminated = true;
    m_wakeup_cv.notify_all();
    m_logger.join();
}

void LogAsynchronousSink::log_direct(const LogRecord &record)
{
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        if (!m_synchronous) {
            m_log_queue.emplace_back(new LogRecord(record));
        } else {
            m_backend->log_direct(record);
            return;
        }
    }
    m_wakeup_cv.notify_all();
}

void LogAsynchronousSink::thread_impl()
{
    std::vector<std::unique_ptr<LogRecord> > buffer;
    std::unique_lock<std::mutex> lock(m_state_mutex);
    while (!m_terminated) {
        m_wakeup_cv.wait(lock);

        if (m_log_queue.size() == 0) {
            continue;
        }

        std::move(m_log_queue.begin(),
                  m_log_queue.end(),
                  std::back_inserter(buffer)
                  );
        m_log_queue.clear();
        lock.unlock();

        for (auto &rec: buffer)
        {
            m_backend->log_direct(*rec);
        }
        buffer.clear();

        lock.lock();
    }
}

void LogAsynchronousSink::set_synchronous(bool synchronous)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_synchronous = synchronous;
}


Logger::Logger(RootLogger *root,
               const std::string &fullpath,
               const std::string &name):
    m_fullpath(fullpath),
    m_name(name),
    m_root(root),
    m_children()
{

}

std::string Logger::get_child_fullpath(const std::string &name) const
{
    return m_fullpath + "." + name;
}

void Logger::log(LogLevel level, const std::string &message) const
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (level < m_level) {
            return;
        }
    }

    m_root->log_submit(log_clock_t::now(), level, m_fullpath, message);
}

void Logger::logf(LogLevel level, const char *format, ...) const
{
    if (level < m_level) {
        return;
    }

    LogTimestamp timestamp = log_clock_t::now();
    std::va_list args;
    va_start(args, format);
    const std::string message = awesomef(format, args);
    va_end(args);

    m_root->log_submit(timestamp, level, m_fullpath, message);
}

LogPipe &Logger::log(LogLevel level) const
{
    return *(new LogPipe(level, *this));
}

void Logger::set_level(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

Logger &Logger::get_child(const std::string &name)
{
    auto iter = m_children.find(name);
    if (iter != m_children.end()) {
        return *iter->second;
    }

    Logger *new_logger = new Logger(m_root, get_child_fullpath(name), name);
    new_logger->set_level(m_level);
    m_children.emplace(name, std::unique_ptr<Logger>(new_logger));
    return *new_logger;
}


RootLogger::RootLogger():
    Logger(this, "root", "root"),
    m_t0(log_clock_t::now())
{
    m_level = LOG_ALL;
}

std::string RootLogger::get_child_fullpath(const std::string &name) const
{
    return name;
}

LogSink *RootLogger::attach_sink(std::unique_ptr<LogSink> &&src)
{
    LogSink *result = src.get();
    m_sinks.emplace_back(std::move(src));
    return result;
}

void RootLogger::log_submit(LogTimestamp timestamp,
                            LogLevel level,
                            const std::string &logger_path,
                            const std::string &message) const
{
    LogRelativeTimestamp rel_timestamp =
            std::chrono::duration_cast<LogRelativeTimestamp>(
                timestamp - m_t0);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (level < m_level) {
        return;
    }

    LogRecord record{level, timestamp, rel_timestamp, logger_path, message};

    for (auto &sink: m_sinks) {
        sink->log(record);
    }
}

Logger &RootLogger::get_logger(const std::string &logger)
{
    if (logger.size() == 0) {
        return *this;
    }

    Logger *curr = this;
    std::string remainder = logger;
    while (true) {
        std::string::size_type dotpos = remainder.find('.');
        if (dotpos == std::string::npos) {
            return curr->get_child(remainder);
        }

        std::string this_name = remainder.substr(0, dotpos);
        curr = &curr->get_child(this_name);
        remainder = remainder.substr(dotpos+1);
    }
}


RootLogger &logging()
{
    static RootLogger logger;
    return logger;
}

std::ostream &submit(std::ostream &stream)
{
    dynamic_cast<::io::LogPipe&>(stream).submit();
    return stream;
}

}
