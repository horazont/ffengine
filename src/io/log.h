#ifndef SCC_IO_LOG_H
#define SCC_IO_LOG_H

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>


namespace io {

enum LogLevel {
    LOG_ALL = 0x00,
    LOG_DEBUG = 0x01,
    LOG_INFO = 0x02,
    LOG_WARNING = 0x04,
    LOG_ERROR = 0x08,
    LOG_EXCEPTION = 0x10,
    LOG_NOTHING = 0xff
};

typedef std::chrono::steady_clock log_clock_t;

typedef log_clock_t::time_point LogTimestamp;
typedef std::chrono::duration<float, std::ratio<1> > LogRelativeTimestamp;


class Logger;


struct LogRecord
{
    LogLevel level;
    LogTimestamp abs_timestamp;
    LogRelativeTimestamp rel_timestamp;
    std::string logger_fullpath;
    std::string message;
};


class LogPipe: public std::ostringstream
{
public:
    LogPipe(LogLevel level, const Logger &dest);

private:
    LogLevel m_level;
    const Logger &m_dest;

public:
    void submit();

};


class LogSink
{
public:
    LogSink();
    virtual ~LogSink();

private:
    mutable std::mutex m_mutex;
    LogLevel m_level;

public:
    virtual void log_direct(const LogRecord &record) = 0;
    void log(const LogRecord &record);

public:
    inline LogLevel level() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_level;
    }

    void set_level(LogLevel level);

};


class LogTTYSink: public LogSink
{
public:
    LogTTYSink();

public:
    void log_direct(const LogRecord &record);

};


class LogAsynchronousSink: public LogSink
{
public:
    LogAsynchronousSink(std::unique_ptr<LogSink> &&backend);
    ~LogAsynchronousSink() override;

private:
    std::unique_ptr<LogSink> m_backend;
    std::mutex m_state_mutex;
    bool m_terminated;
    std::vector<std::unique_ptr<LogRecord> > m_log_queue;
    std::condition_variable m_wakeup_cv;
    std::thread m_logger;

public:
    void log_direct(const LogRecord &record) override;

    void thread_impl();

};


class RootLogger;
class Logger;


class Logger
{
protected:
    Logger(RootLogger *root,
           const std::string &fullpath,
           const std::string &name);

public:
    Logger(const Logger &ref) = delete;
    Logger &operator=(const Logger &ref) = delete;
    Logger(Logger &&src) = delete;
    Logger &operator=(Logger &&src) = delete;

protected:
    const std::string m_fullpath;
    const std::string m_name;
    mutable std::mutex m_mutex;
    RootLogger *const m_root;
    std::unordered_map<std::string, std::unique_ptr<Logger> > m_children;
    LogLevel m_level;

protected:
    virtual std::string get_child_fullpath(const std::string &childname) const;

public:
    void log(LogLevel level, const std::string &message) const;
    void logf(LogLevel level, const std::string &format, ...) const;
    LogPipe &log(LogLevel level) const;

    Logger &get_child(const std::string &name);

public:
    inline const std::string fullpath() const
    {
        return m_fullpath;
    }

    inline const std::string name() const
    {
        return m_name;
    }

    inline LogLevel level() const
    {
        return m_level;
    }

    void set_level(LogLevel level);

};


class RootLogger: public Logger
{
public:
    RootLogger();

private:
    const log_clock_t::time_point m_t0;
    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<LogSink> > m_sinks;

protected:
    std::string get_child_fullpath(const std::string &childname) const override;
    void log_submit(LogTimestamp timestamp,
                    LogLevel level,
                    const std::string &logger_path,
                    const std::string &message) const;

public:
    LogSink *attach_sink(std::unique_ptr<LogSink> &&src);

    template <typename T, typename... args_ts>
    T *attach_sink(args_ts&&... args)
    {
        T *obj = new T(std::forward<args_ts>(args)...);
        attach_sink(std::unique_ptr<LogSink>(obj));
        return obj;
    }

    inline std::tuple<std::unique_lock<std::mutex>, std::vector<std::unique_ptr<LogSink> >&> sinks()
    {
        return std::tuple<std::unique_lock<std::mutex>, std::vector<std::unique_ptr<LogSink> >&>(
                    std::unique_lock<std::mutex>(m_mutex),
                    m_sinks);
    }

    Logger &get_logger(const std::string &path);

    friend class Logger;
};

RootLogger &logging();

std::ostream &submit(std::ostream &stream);

}

#endif
