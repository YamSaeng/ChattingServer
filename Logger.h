#pragma once

#include<mutex>
#include<filesystem>

#define LOG_DEBUG(msg) GetLogger().Debug(msg)
#define LOG_INFO(msg) GetLogger().Info(msg)
#define LOG_WARNING(msg) GetLogger().Warning(msg)
#define LOG_ERROR(msg) GetLogger().Error(msg)
#define LOG_CRITICAL(msg) GetLogger().Critical(msg)

#define LOG_DEBUGF(fmt, ...) GetLogger().Debugf(fmt, __VA_ARGS__)
#define LOG_INFOF(fmt, ...) GetLogger().Infof(fmt, __VA_ARGS__)
#define LOG_WARNINGF(fmt, ...) GetLogger().Warningf(fmt, __VA_ARGS__)
#define LOG_ERRORF(fmt, ...) GetLogger().Errorf(fmt, __VA_ARGS__)
#define LOG_CRITICALF(fmt, ...) GetLogger().Criticalf(fmt, __VA_ARGS__)

#define LOGGER

#ifdef LOGGER
#define LOGGER_DLL __declspec(dllexport)
#else
#define LOGGER_DLL __declspec(dllimport)
#endif

// �α� ����
enum class LogLevel
{
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERR = 3,
    CRITICAL = 4
};

class LOGGER_DLL Logger
{
private:
    static Logger* _instance;
    static mutex _instanceMutex;

    bool _consoleOutput;
    LogLevel _logLevel;
    wstring  _logFile;
    mutex _fileMutex;

    Logger() : _logLevel(LogLevel::INFO), _consoleOutput(true) {}

    // ���� �����ڿ� ���� ������ ���� (�̱���)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // �α� ���� ���ڿ� ��ȯ
    wstring LevelToString(LogLevel level) const;

    // �ܼ� ���� �ڵ�
    wstring GetColorCode(LogLevel level) const;

    // ���� �ð��� ���ڿ��� ��ȯ
    wstring GetCurrentTimeLog() const;

    // �α� �޽��� ������
    wstring FormatLogMessage(LogLevel level, const wstring& message, bool useColor = false) const;

    // �α� ���
    void WriteLog(LogLevel level, const wstring& message);

    template<typename... Args>
    wstring FormatString(const wstring& format, Args... args)
    {
        wchar_t buf[4096];
        swprintf(buf, 4096, format.c_str(), args...);
        return wstring(buf);
    }
public:
    // �̱��� �ν��Ͻ� ��ȯ
    static Logger& GetInstance();

    // �ΰ� ����
    void Configure(const std::wstring& logFilePath = L"",
        LogLevel level = LogLevel::INFO,
        bool enableConsoleOutput = true);

    // �α� ���� ����
    void SetLevel(LogLevel level);

    // ���� �α� ���� ��ȯ
    LogLevel GetLevel() const;

    // �α� �޼����
    void Debug(const std::wstring& message);
    void Info(const std::wstring& message);
    void Warning(const std::wstring& message);
    void Error(const std::wstring& message);
    void Critical(const std::wstring& message);

    // �����õ� �α� �޼���� (printf ��Ÿ��) - ���ø��� ����� ����
    template<typename... Args>
    void Debugf(const std::wstring& format, Args... args)
    {
        Debug(FormatString(format, args...));
    }

    template<typename... Args>
    void Infof(const std::wstring& format, Args... args)
    {
        Info(FormatString(format, args...));
    }

    template<typename... Args>
    void Warningf(const std::wstring& format, Args... args)
    {
        Warning(FormatString(format, args...));
    }

    template<typename... Args>
    void Errorf(const std::wstring& format, Args... args)
    {
        Error(FormatString(format, args...));
    }

    template<typename... Args>
    void Criticalf(const std::wstring& format, Args... args)
    {
        Critical(FormatString(format, args...));
    }
};

Logger& GetLogger()
{
    return Logger::GetInstance();
}

void ConfigureLogger(const std::wstring& _logFile,
    LogLevel level = LogLevel::INFO,
    bool _consoleOutput = true)
{
    Logger::GetInstance().Configure(_logFile, level, _consoleOutput);
}