#pragma once

#include<curl/curl.h>
#include"../Logger.h"

// IP ������ �ش� ���� �ڵ带 �����ϴ� ����ü
struct IPRange
{
	unsigned int start; // ���� ���� IP 
	unsigned int end; // ���� �� IP
	wstring countryCode; // ���� �ڵ� (��: "KR", "US", "JP")

	IPRange(unsigned int s, unsigned int e, const wstring& c) : start(s), end(e), countryCode(c) {}

	// �־��� IP�� ������ ���ԵǴ��� Ȯ��
	// ip : Ȯ���� IP �ּ�
	// return: ������ ���ԵǸ� true, �ƴϸ� false
	bool contains(unsigned int ip) const
	{
		return ip >= start && ip <= end;
	}
};

// curl�� �ٿ�ε��� �����͸� �����ϴ� ����ü
struct MemoryStruct
{
	char* memory; // �ٿ�ε�� �����͸� ������ ���
	size_t size;  // ����� ������ ũ��
};

/**
 * CURL �ݹ� �Լ�: �ٿ�ε�� �����͸� �޸𸮿� ����
 * @param contents �ٿ�ε�� ������ ������
 * @param size �� ������ ����� ũ��
 * @param nmemb ������ ����� ����
 * @param userp ����� ������ (MemoryStruct ������)
 * @return ó���� ����Ʈ ��
 */
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)userp;

	// ���� �޸𸮸� Ȯ���ؼ� �� �����͸� �߰��Ѵ�.
	char* ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr) {
		LOG_CRITICAL(L"�޸� �Ҵ� ����");		
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0; // ���ڿ� ���� ���� �߰�

	return realsize;
}

// IP �ּ��� ������ Ȯ��
// �� ���� 5�� RIR���� �����͸� �����Ͽ�
// IP �ּҰ� ��� ���� ���ϴ��� Ȯ���Ѵ�.
class IPCountryChecker
{
private:
	vector<IPRange> _ranges; // ��� IP ����
	chrono::system_clock::time_point _lastUpdate; // ������ ������Ʈ �ð�
	unordered_map<wstring, int> _countryStats; // ������ IP ���� ���

	// ĳ�� ���� ���� ���
	const wstring CACHE_FILE = L"ip-database.txt";	
	const int UPDATE_INTERVAL_HOURS = 24;

	// RIR ������ �����ϴ� ����ü
	struct RIRInfo
	{
		wstring name; // RIR �̸� (��: "APNIC", "RIPE")
		wstring url; // ������ �ٿ�ε� url
		wstring filePrefix; // ���� ���λ�
	};

	/**
	 * �� ���� 5�� RIR ���� ���
	 * - APNIC: �ƽþ�-����� ����
	 * - RIPE: ����, �ߵ�, �߾Ӿƽþ�
	 * - ARIN: �Ϲ�
	 * - LACNIC: ��ƾ�Ƹ޸�ī, ī������
	 * - AFRINIC: ������ī
	 */
	vector<RIRInfo> _rirList = {
		{L"APNIC", L"https://ftp.apnic.net/stats/apnic/delegated-apnic-latest", L"apnic"},
		{L"RIPE", L"https://ftp.ripe.net/ripe/stats/delegated-ripencc-latest", L"ripencc"},
		{L"ARIN", L"https://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest", L"arin"},
		{L"LACNIC", L"https://ftp.lacnic.net/pub/stats/lacnic/delegated-lacnic-latest", L"lacnic"},
		{L"AFRINIC", L"https://ftp.afrinic.net/pub/stats/afrinic/delegated-afrinic-latest", L"afrinic"}
	};

	/**
	 * ���� �ε�� �����͸� ���Ͽ� ����
	 * @param fileName ������ ���ϸ�
	 * @return ���� ���� ����
	 */
	bool SaveToFile(const wstring& fileName) const
	{
		wofstream file(fileName);
		if (!file.is_open())
		{
			LOG_ERRORF(L"[%s] ���� ���� ����", fileName.c_str());			
			return false;
		}		

		LOG_DEBUGF(L"�����͸� ���Ͽ� ���� ��: %s", fileName.c_str());		

		// ��� ���� ���� (������Ʈ �ð�)
		auto now = chrono::system_clock::now();
		auto time_t_val = chrono::system_clock::to_time_t(now);
		struct tm timeinfo;
		localtime_s(&timeinfo, &time_t_val);

		file << L"# IP Country Database Cache File" << endl;
		file << L"# Last Updated: " << put_time(&timeinfo, L"%Y-%m-%d %H:%M:%S") << endl;
		file << L"# Total Ranges: " << _ranges.size() << endl;
		file << L"#" << endl;

		// �� IP ������ RIR �������� ����
		for (const auto& range : _ranges)
		{
			// ������ RIR �̸��� ���� ��¥�� ����Ͽ� ǥ�� �������� ����
			wstring startIP = IntToIP(range.start);
			uint32_t count = range.end - range.start + 1;

			file << L"cache|" << range.countryCode << L"|ipv4|"
				<< startIP << L"|" << count << L"|20240101|allocated" << endl;
		}

		file.close();
		LOG_DEBUGF(L"%s�� %d�� ���� ���� �Ϸ�", fileName.c_str(), (int)_ranges.size());		
		return true;
	}

	/**
	 * ĳ�� ������ ���� �ð��� Ȯ��
	 * @param fileName Ȯ���� ���ϸ�
	 * @return ������ ������ ���� �ð�
	 */
	chrono::system_clock::time_point GetFileModificationTime(const wstring& fileName) const
	{
		struct _stat fileInfo;
		if (_wstat(fileName.c_str(), &fileInfo) == 0)
		{
			return chrono::system_clock::from_time_t(fileInfo.st_mtime);
		}

		// ������ ������ epoch �ð�(1970-01-01 00:00:00) ��ȯ
		return chrono::system_clock::from_time_t(0);
	}

	/**
	 * ĳ�� ������ ��ȿ���� Ȯ�� (24�ð� �̳�)
	 * @param fileName Ȯ���� ���ϸ�
	 * @return ĳ�ð� ��ȿ�ϸ� true
	 */
	bool IsCacheValid(const wstring& fileName) const
	{
		auto fileTime = GetFileModificationTime(fileName);
		// epoch �ð��̸� ������ ����
		if (fileTime == chrono::system_clock::from_time_t(0))
		{
			return false; // ������ ����
		}

		auto now = chrono::system_clock::now();
		auto duration = chrono::duration_cast<chrono::hours>(now - fileTime);
		return duration.count() < UPDATE_INTERVAL_HOURS; // 24�ð� �̸��̸� ��ȿ
	}

	/**
	 * IP �ּ� ���ڿ��� 32��Ʈ ������ ��ȯ
	 * ��: "192.168.1.1" -> 3232235777
	 * @param ip IP �ּ� ���ڿ� (��: "192.168.1.1")
	 * @return 32��Ʈ ������ ��ȯ�� IP �ּ�, ���н� 0
	 */
	unsigned IPToInt(const wstring& ip) const
	{
		wistringstream iss(ip);
		wstring octet;
		unsigned int result = 0;
		int shift = 24; // ù ��° ������ 24��Ʈ �������� ����Ʈ

		// ��(.)���� ���е� �� ������ ó��
		while (getline(iss, octet, L'.') && shift >= 0)
		{
			try
			{
				int value = stoi(octet);
				if (value < 0 || value > 255) return 0; // ��ȿ���� ���� ���ݰ�
				result |= (static_cast<uint32_t>(value) << shift);
				shift -= 8; // ���� ������ ���� 8��Ʈ�� ����
			}
			catch (...) {
				return 0; // ���� ��ȯ ����
			}
		}

		return result;
	}

	/**
	 * 32��Ʈ ������ IP �ּ� ���ڿ��� ��ȯ
	 * ��: 3232235777 -> "192.168.1.1"
	 * @param ip 32��Ʈ ���� IP �ּ�
	 * @return IP �ּ� ���ڿ�
	 */
	wstring IntToIP(unsigned int ip) const
	{
		return to_wstring((ip >> 24) & 0xFF) + L"." + // ù ��° ����
			to_wstring((ip >> 16) & 0xFF) + L"." +	// �� ��° ����
			to_wstring((ip >> 8) & 0xFF) + L"." +		// �� ��° ����
			to_wstring(ip & 0xFF);					// �� ��° ����
	}

	/**
	 * RIR �����͸� �Ľ��Ͽ� IP ���� ������ ����
	 * ������ ����: RIR|�����ڵ�|ipv4|����IP|����|��¥|����
	 * ��: apnic|KR|ipv4|1.208.0.0|4096|20110101|allocated
	 * @param data �Ľ��� RIR ������ ���ڿ�
	 * @param rirName RIR �̸� (�α׿�)
	 * @return �Ľ� ���� ����
	 */
	bool ParseRIRData(const wstring& data, const wstring& rirName)
	{
		vector<IPRange> newRanges;
		wistringstream stream(data);
		wstring line;
		int parsedCount = 0;

		// �����͸� �� �پ� ó��
		while (getline(stream, line))
		{
			// �� ���̳� �ּ�(#���� ����) �ǳʶٱ�
			if (line.empty() || line[0] == L'#') continue;

			// ������(|)�� ���е� �ʵ���� �и�
			vector<wstring> tokens;
			wstringstream ss(line);
			wstring token;

			while (getline(ss, token, L'|'))
			{
				tokens.push_back(token);
			}

			// �ּ� 5�� �ʵ尡 �ְ�, IPv4 ���ڵ����� Ȯ��
			if (tokens.size() >= 5 && tokens[2] == L"ipv4")
			{
				try
				{
					wstring countryCode = tokens[1];    // ���� �ڵ� (��: KR, US, JP)
					wstring startIP = tokens[3];		   // ���� IP �ּ�
					uint32_t count = stoul(tokens[4]); // IP ����

					// ��ȿ�� ���� �ڵ�(2����)�� IP ���� Ȯ��
					if (countryCode.length() == 2 && count > 0)
					{
						uint32_t start = IPToInt(startIP);
						if (start != 0)
						{
							uint32_t end = start + count - 1; // �� IP ���
							newRanges.emplace_back(start, end, countryCode);
							parsedCount++;

							// ������ ��� ������Ʈ
							_countryStats[countryCode] += count;
						}
					}
				}
				catch (...)
				{
					continue;  // �Ľ� ���� �� �ش� ���� �ǳʶٱ�
				}
			}
		}

		LOG_DEBUGF(L"%s���� %d���� IP ���� �Ľ� �Ϸ�", rirName.c_str(), parsedCount);

		// ���ο� ������ ���� ���� ���Ϳ� �߰�
		_ranges.insert(_ranges.end(), newRanges.begin(), newRanges.end());

		return parsedCount > 0;
	}

	/**
	 * ���� RIR���� �����͸� �ٿ�ε�
	 * @param rir �ٿ�ε��� RIR ����
	 * @return �ٿ�ε� �� �Ľ� ���� ����
	 */
	bool DownloadFromRIR(const RIRInfo& rir)
	{
		CURL* curl;
		CURLcode res;

		// �ٿ�ε�� �����͸� ������ �޸� ����ü �ʱ�ȭ
		MemoryStruct chunk;
		chunk.memory = (char*)malloc(1);
		chunk.size = 0;

		// CURL �ʱ�ȭ
		curl = curl_easy_init();
		if (!curl)
		{
			free(chunk.memory);	
			LOG_ERRORF(L"%s CURL �ʱ�ȭ ����", rir.name.c_str());			
			return false;
		}

		LOG_DEBUGF(L"%s���� ������ �ٿ�ε� ��...", rir.name.c_str());		

		string urlStr(rir.url.begin(), rir.url.end());

		// CURL �ɼ� ����
		curl_easy_setopt(curl, CURLOPT_URL, urlStr.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "IP-Country-Checker/1.0");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

		// �ٿ�ε� ����
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		// �ٿ�ε� ��� Ȯ��
		if (res != CURLE_OK)
		{
			LOG_ERRORF(L"%s �ٿ�ε� ����", rir.name.c_str());			
			free(chunk.memory);
			return false;
		}

		LOG_DEBUGF(L"%s �ٿ�ε� �Ϸ� (%d bytes)", rir.name.c_str(), (int)chunk.size);		

		// �ٿ�ε�� �����͸� ���ڿ��� ��ȯ�ϰ� �Ľ�
		string tempData(chunk.memory);
		wstring data(tempData.begin(), tempData.end());
		free(chunk.memory);

		return ParseRIRData(data, rir.name);
	}

	/**
	 * �ѱ� Ŭ���̾�Ʈ ó�� ����
	 * @param ip Ŭ���̾�Ʈ IP �ּ�
	 */
	void ProcessKoreanClient(const wstring& ip)
	{
		LOG_INFO(L" -> �ѱ� Ŭ���̾�Ʈ ���� ����");		
	}

	/**
	 * �ܱ� Ŭ���̾�Ʈ ó�� ����
	 * @param ip Ŭ���̾�Ʈ IP �ּ�
	 * @param countryCode ���� �ڵ�
	 */
	void ProcessForeignClient(const wstring& ip, const wstring& countryCode)
	{
		LOG_INFOF(L" -> �ܱ� Ŭ���̾�Ʈ (%s) ���� ó��: %s",countryCode.c_str(), ip.c_str());		
	}

public:
	IPCountryChecker()
	{
		// curl ���̺귯�� �ʱ�ȭ
		curl_global_init(CURL_GLOBAL_DEFAULT);
	}

	~IPCountryChecker()
	{
		// curl ���̺귯�� ����
		curl_global_cleanup();
	}
private:
	/**
	 * ��� RIR���� �����͸� �ٿ�ε��ϰ� �Ľ�
	 * �� ���� 5�� RIR���� ���������� �����͸� �ٿ�ε��մϴ�.
	 * @return �ϳ� �̻��� RIR���� ���������� �ٿ�ε������� true
	 */
	bool DownloadFromAllRIRs()
	{
		LOG_DEBUG(L"=== ��� RIR���� ������ �ٿ�ε� ���� ===");

		// ���� ������ �ʱ�ȭ
		_ranges.clear();
		_countryStats.clear();

		bool anySuccess = false;

		// �� RIR���� ���������� �ٿ�ε�
		for (const auto& rir : _rirList)
		{
			if (DownloadFromRIR(rir))
			{
				anySuccess = true;
			}

			// ���� ���� ������ ���� �� �ٿ�ε� ���̿� 0.5�� ���
			this_thread::sleep_for(chrono::milliseconds(500));
		}

		if (anySuccess)
		{
			// IP ������ ���� �ּ� �������� ���� (���� �˻� ����ȭ)
			sort(_ranges.begin(), _ranges.end(),
				[](const IPRange& a, const IPRange& b)
				{
					return a.start < b.start;
				});

			_lastUpdate = chrono::system_clock::now();
			LOG_DEBUGF(L"�� %d���� IP ���� �ε� �Ϸ�", (int)_ranges.size());

			// ���� �߰�: ���������� �ٿ�ε��ϸ� ĳ�� ���Ͽ� ����
			SaveToFile(CACHE_FILE);
		}

		return anySuccess;
	}

	/**
	 * ���Ͽ��� IP �����͸� �ε� (�����)
	 * ��Ʈ��ũ �ٿ�ε尡 �������� �� ���
	 * @param fileName �ε��� ���ϸ�
	 * @return �ε� ���� ����
	 */
	bool LoadFromFile(const wstring& fileName)
	{
		wifstream file(fileName);
		if (!file.is_open())
		{
			LOG_ERRORF(L"[%s] ������ �� �� �����ϴ�.", fileName.c_str());			
			return false;
		}

		// ���� ��ü ������ ���ڿ��� �б�
		wstring content((istreambuf_iterator<wchar_t>(file)),
			istreambuf_iterator<wchar_t>());
		file.close();

		LOG_DEBUGF(L"���Ͽ��� ������ �ε� ��: %s", fileName.c_str());
		
		return ParseRIRData(content, L"FILE");
	}

	/**
	 * IP ���� Ȯ�α� �ʱ�ȭ
	 * 1. ĳ�� ������ ��ȿ�ϸ� ĳ�ÿ��� �ε�
	 * 2. ĳ�ð� ���ų� ����Ǹ� �¶��ο��� �ٿ�ε�
	 * 3. �¶��� �ٿ�ε� ���н� ����� ĳ�ö� ���
	 * @return �ʱ�ȭ ���� ����
	 */
	bool Initialize()
	{
		LOG_DEBUG(L"=== IP ���� Ȯ�α� �ʱ�ȭ ===");

		bool loaded = false;

		// 1�ܰ�: ĳ�� ���� ��ȿ�� Ȯ��
		if (IsCacheValid(CACHE_FILE))
		{
			LOG_DEBUG(L"��ȿ�� ĳ�� ���� �߰�, ĳ�ÿ��� �ε� ��...");			
			
			loaded = LoadFromFile(CACHE_FILE);
			if (loaded)
			{
				// ĳ�ÿ��� �ε� ������ ���� ���� �ð��� ������ ������Ʈ �ð����� ����
				_lastUpdate = GetFileModificationTime(CACHE_FILE);

				LOG_DEBUG(L"ĳ�ÿ��� �ε� �Ϸ�");				
			}
		}

		// 2�ܰ�: ĳ�ð� ���ų� �ε� ���н� �¶��ο��� �ٿ�ε�
		if (!loaded)
		{
			LOG_DEBUG(L"�¶��ο��� ������ �ٿ�ε� �õ�...");			
			loaded = DownloadFromAllRIRs();
		}

		// 3�ܰ�: �¶��� �ٿ�ε嵵 �����ϸ� ����� ĳ�ö� ���
		if (!loaded && GetFileModificationTime(CACHE_FILE) != chrono::system_clock::from_time_t(0))
		{
			LOG_DEBUG(L"�¶��� �ٿ�ε� ����, ����� ĳ�� ���� ���...");			
			loaded = LoadFromFile(CACHE_FILE);
			if (loaded)
			{
				_lastUpdate = GetFileModificationTime(CACHE_FILE);
				LOG_DEBUG(L"����� ĳ�ÿ��� �ε� �Ϸ� (������Ʈ ����)");				
			}
		}

		// 4�ܰ�: �ε� ������ ��� ���
		if (loaded)
		{
			PrintStatistics();
		}

		return loaded;
	}	

	/**
	 * Ŭ���̾�Ʈ ó�� (������ Ŭ���̾�Ʈ�� ������ ���� ó��)
	 * @param clientIP Ŭ���̾�Ʈ IP �ּ�
	 */
	void HandleClient(const wstring& clientIP)
	{
		wstring countryCode = IPCheck(clientIP);

		LOG_DEBUGF(L"[����] Ŭ���̾�Ʈ %s ->", countryCode);		

		// ���� �ڵ忡 ���� ó�� �б�
		if (countryCode == L"KR")
		{
			LOG_DEBUG(L" (�ѱ� IP - ���� ���)");			
			ProcessKoreanClient(clientIP);
		}
		else if (countryCode == L"UNKNOWN")
		{
			LOG_DEBUG(L" (�� �� ���� IP)");			
		}
		else if (countryCode == L"INVALID")
		{
			LOG_DEBUG(L" (�߸��� IP ����)");			
		}
		else
		{
			LOG_DEBUG(L" (�ܱ� IP - ���� ����)");			
			ProcessForeignClient(clientIP, countryCode);
		}
	}

	/**
	 * ���� Ŭ���̾�Ʈ�� �� ���� ó��
	 * @param clientIPs Ŭ���̾�Ʈ IP �ּ� ���
	 */
	void HandleMultipleClients(const vector<wstring>& clientIPs)
	{
		LOG_DEBUG(L"=== ��ġ Ŭ���̾�Ʈ ó�� ===");
		for (const auto& ip : clientIPs)
		{
			HandleClient(ip);
		}
	}

	/**
	 * �ε�� IP ���� ���� ��ȯ
	 * @return IP ���� ����
	 */
	size_t GetRangeCount() const
	{
		return _ranges.size();
	}

	/**
	 * ������ ������Ʈ �ð� ��ȯ
	 * @return ������ ������Ʈ �ð�
	 */
	chrono::system_clock::time_point GetLastUpdateTime() const
	{
		return _lastUpdate;
	}

	/**
	 * ������ ������Ʈ�� �ʿ����� Ȯ�� (������ ����)
	 * ĳ�� ���� �������� 24�ð� ��� ���� Ȯ��
	 * @return ������Ʈ�� �ʿ��ϸ� true
	 */
	bool NeedsUpdate() const
	{
		return !IsCacheValid(CACHE_FILE);
	}

	/**
	 * �������� ������ ������Ʈ ����
	 * @return ������Ʈ ���� ����
	 */
	bool UpdateData()
	{
		LOG_DEBUG(L"���� ������ ������Ʈ ����...");
		bool success = DownloadFromAllRIRs();
		if (success)
		{
			LOG_DEBUG(L"������ ������Ʈ �Ϸ�");
		}
		else
		{
			LOG_DEBUG(L"������ ������Ʈ ����");
		}
		return success;
	}

	/**
	 * �����ͺ��̽� ��� ���� ���
	 * - �� IP ���� ��
	 * - �� IP ����
	 * - ������ IP ���� (���� 10����)
	 */
	void PrintStatistics() const
	{
		LOG_DEBUG(L"\n=== IP �����ͺ��̽� ��� ===");
		LOG_DEBUGF(L"�� IP ���� ��: %d", (int)_ranges.size());		

		// ��ü IP ���� ���
		unsigned int totalIPs = 0;
		for (const auto& range : _ranges)
		{
			totalIPs += (range.end - range.start + 1);
		}

		LOG_DEBUGF(L"�� IP ���� ��: %d", (int)_ranges.size());
		LOG_DEBUGF(L"�� IP ����: %d", (int)_ranges.size());		

		// ������ IP ������ ������������ ����.
		LOG_DEBUG(L"\n=== ������ IP ���� (���� 10��) ===");
		
		vector<pair<wstring, int>> sortedStats;
		for (const auto& stat : _countryStats)
		{
			sortedStats.push_back(stat);
		}

		sort(sortedStats.begin(), sortedStats.end(),
			[](const pair<wstring, int>& a, const pair<wstring, int>& b)
			{
				return a.second > b.second;  
			});

		// ���� 10�� ���� ���
		int count = 0;
		for (const auto& stat : sortedStats)
		{
			if (count >= 10) break;			

			LOG_DEBUGF(L"%s: %d IPs", stat.first.c_str(), stat.second);
			count++;
		}
	}

	/**
	 * IP ���� ��� ��� (������)
	 * @param limit ����� �ִ� ���� �� (�⺻��: 10)
	 */
	void PrintRanges(int limit = 10) const
	{
		cout << "\n=== IP ���� ��� (�ִ� " << limit << "��) ===" << endl;
		for (size_t i = 0; i < _ranges.size() && i < static_cast<size_t>(limit); ++i)
		{
			const auto& range = _ranges[i];
			LOG_DEBUGF(L"%s - %s [%s] (%d IPs)", IntToIP(range.start), IntToIP(range.end), range.countryCode, (int)(range.end - range.start + 1));			
		}

		if (_ranges.size() > static_cast<size_t>(limit))
		{
			LOG_DEBUGF(L"... (%d�� ��)", (int)(_ranges.size()) - limit);			
		}
	}

	/**
	 * �ڵ� ������Ʈ ��׶��� �۾� ����
	 * ���� �����忡�� �ֱ������� �����͸� ������Ʈ�մϴ�.
	 * @param intervalHours ������Ʈ ���� (�ð� ����, �⺻��: 24�ð�)
	 */
	void StartAutoUpdate(int intervalHours = 24)
	{
		thread([this, intervalHours]()
			{
				while (true)
				{
					// ������ �ð���ŭ ���
					this_thread::sleep_for(chrono::hours(intervalHours));
					LOG_DEBUG(L"�ڵ� ������Ʈ ����...");

					// ��� RIR���� ������ �ٽ� �ٿ�ε�
					if (!DownloadFromAllRIRs())
					{
						LOG_DEBUG(L"�ڵ� ������Ʈ ����");
					}
				}
			}).detach();  // �����带 �и��Ͽ� ��׶��忡�� ����
	}
public:

	/**
	 * �ֿ� ���: IP �ּ��� ���� �ڵ带 ��ȯ
	 * @param ip IP �ּ� ���ڿ� (��: "8.8.8.8")
	 * @return ���� �ڵ� ("KR", "US", "JP" ��) �Ǵ� "INVALID"/"UNKNOWN"
	 */
	wstring IPCheck(const wstring& ip) const
	{
		if (ip == L"127.0.0.1")
		{
			return L"LOOPBACK";
		}

		unsigned int ipInt = IPToInt(ip);
		if (ipInt == 0) return L"INVALID"; // �߸��� IP ����

		return IPCheck(ipInt);
	}

	/**
	 * �ֿ� ���: IP �ּ�(������)�� ���� �ڵ带 ��ȯ
	 * ���� �˻��� ����Ͽ� ���� ��ȸ ���� ����
	 * @param ipInt IP �ּ� (32��Ʈ ����)
	 * @return ���� �ڵ� ("KR", "US", "JP" ��) �Ǵ� "INVALID"/"UNKNOWN"
	 */
	wstring IPCheck(unsigned int ipInt) const
	{
		if (ipInt == 0) return L"INVALID";

		// ���� �˻����� �ش� IP�� �����ϴ� ���� ã��
		auto it = lower_bound(_ranges.begin(), _ranges.end(), ipInt,
			[](const IPRange& range, unsigned int ip)
			{
				return range.end < ip; // ������ ���� IP���� ������ true
			});

		// ã�� ������ ������ IP�� �����ϴ��� Ȯ��
		if (it != _ranges.end() && it->contains(ipInt))
		{
			return it->countryCode;
		}

		return L"UNKNOWN"; // �ش��ϴ� ������ ã�� ����
	}

	/**
	 * �ѱ� IP Ȯ�� (���� �ڵ���� ȣȯ�� ����)
	 * @param ip IP �ּ� ���ڿ�
	 * @return �ѱ� IP�̸� true, �ƴϸ� false
	 */
	bool IsKoreanIP(const wstring& ip) const
	{
		return IPCheck(ip) == L"KR";
	}

	/**
	 * �ѱ� IP Ȯ�� (������ IP��)
	 * @param ipInt IP �ּ� (32��Ʈ ����)
	 * @return �ѱ� IP�̸� true, �ƴϸ� false
	 */
	bool IsKoreanIP(unsigned int ipInt) const
	{
		return IPCheck(ipInt) == L"KR";
	}

	/**
	 * ĳ�� ���� ���� ���
	 */
	void PrintCacheStatus() const
	{
		LOG_DEBUG(L"\n=== ĳ�� ���� ���� ===");		

		auto fileTime = GetFileModificationTime(CACHE_FILE);
		if (fileTime == chrono::system_clock::from_time_t(0))
		{
			LOG_DEBUG(L"ĳ�� ����: ����");			
		}
		else
		{
			auto time_t_val = chrono::system_clock::to_time_t(fileTime);
			struct tm timeinfo;
			localtime_s(&timeinfo, &time_t_val);			

			LOG_DEBUGF(L"ĳ�� ���� %s", CACHE_FILE);			
			wcout << L"ĳ�� ���� �ð�: " << put_time(&timeinfo, L"%Y-%m-%d %H:%M:%S") << endl;
			wcout << L"ĳ�� ��ȿ��: " << (IsCacheValid(CACHE_FILE) ? L"��ȿ" : L"����") << endl;

			auto now = chrono::system_clock::now();
			auto duration = chrono::duration_cast<chrono::hours>(now - fileTime);
			LOG_DEBUGF(L"��� �ð�: %d�ð�", duration.count());			
		}
	}

	// ����
	void Start()
	{
		if (!Initialize())
		{
			LOG_ERROR(L"�ʱ�ȭ ����. ���α׷��� �����մϴ�.");			
			return;
		}		
	}
};