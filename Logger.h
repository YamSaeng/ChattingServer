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

// 로그 레벨
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

    // 복사 생성자와 대입 연산자 삭제 (싱글턴)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 로그 레벨 문자열 변환
    wstring LevelToString(LogLevel level) const;

    // 콘솔 색상 코드
    wstring GetColorCode(LogLevel level) const;

    // 현재 시간을 문자열로 반환
    wstring GetCurrentTimeLog() const;

    // 로그 메시지 포맷팅
    wstring FormatLogMessage(LogLevel level, const wstring& message, bool useColor = false) const;

    // 로그 출력
    void WriteLog(LogLevel level, const wstring& message);

    template<typename... Args>
    wstring FormatString(const wstring& format, Args... args)
    {
        wchar_t buf[4096];
        swprintf(buf, 4096, format.c_str(), args...);
        return wstring(buf);
    }
public:
    // 싱글톤 인스턴스 반환
    static Logger& GetInstance();

    // 로거 설정
    void Configure(const std::wstring& logFilePath = L"",
        LogLevel level = LogLevel::INFO,
        bool enableConsoleOutput = true);

    // 로그 레벨 설정
    void SetLevel(LogLevel level);

    // 현재 로그 레벨 반환
    LogLevel GetLevel() const;

    // 로그 메서드들
    void Debug(const std::wstring& message);
    void Info(const std::wstring& message);
    void Warning(const std::wstring& message);
    void Error(const std::wstring& message);
    void Critical(const std::wstring& message);

    // 포맷팅된 로그 메서드들 (printf 스타일) - 템플릿은 헤더에 구현
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