#pragma once

class Utils
{
public:
    static char* LoadFile(const wchar_t* path)
    {
        char retBuf[1000000];
        memset(retBuf, 0, sizeof(retBuf));

        DWORD readSize;

        HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            printf("파일 안열림");
        }

        int dataSize = GetFileSize(file, NULL);

        ReadFile(file, retBuf, dataSize, &readSize, NULL);
        if (readSize != dataSize)
        {
            CloseHandle(file);
            return nullptr;
        }

        CloseHandle(file);

        return retBuf;
    }

	static wstring Convert(string str)
	{
        if (str.empty())
        {
            return std::wstring();
        }            

        const int srcLen = static_cast<int>(str.size());

        // 변환할 길이 계산
        const int retLen = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), srcLen, NULL, 0);
        if (retLen == 0)
        {
            throw std::runtime_error("UTF-8 → UTF-16 변환 실패");
        }            

        std::wstring ret(retLen, L'\0');
        if (::MultiByteToWideChar(CP_UTF8, 0, str.data(), srcLen, &ret[0], retLen) == 0)
        {
            throw std::runtime_error("UTF-8 → UTF-16 변환 실패");
        }            

        return ret;
	}
};