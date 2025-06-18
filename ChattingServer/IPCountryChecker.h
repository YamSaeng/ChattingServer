#pragma once
#include"pch.h"
#include<curl/curl.h>
#include<iomanip>

// IP ������ �ش� ���� �ڵ带 �����ϴ� ����ü
struct IPRange
{
	unsigned int start; // ���� ���� IP 
	unsigned int end; // ���� �� IP
	string countryCode; // ���� �ڵ� (��: "KR", "US", "JP")

	IPRange(unsigned int s, unsigned int e, const string& c) : start(s), end(e), countryCode(c) {}

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
		printf("�޸� �Ҵ� ����!\n");
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
	unordered_map<string, int> _countryStats; // ������ IP ���� ���

	// ĳ�� ���� ���� ���
	const string CACHE_FILE = "ip-database.txt";	
	const int UPDATE_INTERVAL_HOURS = 24;

	// RIR ������ �����ϴ� ����ü
	struct RIRInfo
	{
		string name; // RIR �̸� (��: "APNIC", "RIPE")
		string url; // ������ �ٿ�ε� url
		string filePrefix; // ���� ���λ�
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
		{"APNIC", "https://ftp.apnic.net/stats/apnic/delegated-apnic-latest", "apnic"},
		{"RIPE", "https://ftp.ripe.net/ripe/stats/delegated-ripencc-latest", "ripencc"},
		{"ARIN", "https://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest", "arin"},
		{"LACNIC", "https://ftp.lacnic.net/pub/stats/lacnic/delegated-lacnic-latest", "lacnic"},
		{"AFRINIC", "https://ftp.afrinic.net/pub/stats/afrinic/delegated-afrinic-latest", "afrinic"}
	};

	/**
	 * ���� �ε�� �����͸� ���Ͽ� ����
	 * @param fileName ������ ���ϸ�
	 * @return ���� ���� ����
	 */
	bool SaveToFile(const string& fileName) const
	{
		ofstream file(fileName);
		if (!file.is_open())
		{
			cout << "[" << fileName << "] ���� ���� ����" << endl;
			return false;
		}

		cout << "�����͸� ���Ͽ� ���� ��: " << fileName << endl;

		// ��� ���� ���� (������Ʈ �ð�)
		auto now = chrono::system_clock::now();
		auto time_t_val = chrono::system_clock::to_time_t(now);
		struct tm timeinfo;
		localtime_s(&timeinfo, &time_t_val);
		file << "# IP Country Database Cache File" << endl;
		file << "# Last Updated: " << put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << endl;
		file << "# Total Ranges: " << _ranges.size() << endl;
		file << "#" << endl;

		// �� IP ������ RIR �������� ����
		for (const auto& range : _ranges)
		{
			// ������ RIR �̸��� ���� ��¥�� ����Ͽ� ǥ�� �������� ����
			string startIP = IntToIP(range.start);
			uint32_t count = range.end - range.start + 1;

			file << "cache|" << range.countryCode << "|ipv4|"
				<< startIP << "|" << count << "|20240101|allocated" << endl;
		}

		file.close();
		cout << fileName << "�� " << _ranges.size() << "�� ���� ���� �Ϸ�" << endl;
		return true;
	}

	/**
	 * ĳ�� ������ ���� �ð��� Ȯ��
	 * @param fileName Ȯ���� ���ϸ�
	 * @return ������ ������ ���� �ð�
	 */
	chrono::system_clock::time_point GetFileModificationTime(const string& fileName) const
	{
		struct stat fileInfo;
		if (stat(fileName.c_str(), &fileInfo) == 0)
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
	bool IsCacheValid(const string& fileName) const
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
	unsigned IPToInt(const string& ip) const
	{
		istringstream iss(ip);
		string octet;
		unsigned int result = 0;
		int shift = 24; // ù ��° ������ 24��Ʈ �������� ����Ʈ

		// ��(.)���� ���е� �� ������ ó��
		while (getline(iss, octet, '.') && shift >= 0)
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
	string IntToIP(unsigned int ip) const
	{
		return to_string((ip >> 24) & 0xFF) + "." + // ù ��° ����
			to_string((ip >> 16) & 0xFF) + "." +	// �� ��° ����
			to_string((ip >> 8) & 0xFF) + "." +		// �� ��° ����
			to_string(ip & 0xFF);					// �� ��° ����
	}

	/**
	 * RIR �����͸� �Ľ��Ͽ� IP ���� ������ ����
	 * ������ ����: RIR|�����ڵ�|ipv4|����IP|����|��¥|����
	 * ��: apnic|KR|ipv4|1.208.0.0|4096|20110101|allocated
	 * @param data �Ľ��� RIR ������ ���ڿ�
	 * @param rirName RIR �̸� (�α׿�)
	 * @return �Ľ� ���� ����
	 */
	bool ParseRIRData(const string& data, const string& rirName)
	{
		vector<IPRange> newRanges;
		istringstream stream(data);
		string line;
		int parsedCount = 0;

		// �����͸� �� �پ� ó��
		while (getline(stream, line))
		{
			// �� ���̳� �ּ�(#���� ����) �ǳʶٱ�
			if (line.empty() || line[0] == '#') continue;

			// ������(|)�� ���е� �ʵ���� �и�
			vector<string> tokens;
			stringstream ss(line);
			string token;

			while (getline(ss, token, '|'))
			{
				tokens.push_back(token);
			}

			// �ּ� 5�� �ʵ尡 �ְ�, IPv4 ���ڵ����� Ȯ��
			if (tokens.size() >= 5 && tokens[2] == "ipv4")
			{
				try
				{
					string countryCode = tokens[1];    // ���� �ڵ� (��: KR, US, JP)
					string startIP = tokens[3];		   // ���� IP �ּ�
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

		cout << rirName << "���� " << parsedCount << "���� IP ���� �Ľ� �Ϸ�" << endl;

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
			cout << rir.name << " CURL �ʱ�ȭ ����" << endl;
			return false;
		}

		cout << rir.name << "���� ������ �ٿ�ε� ��..." << endl;

		// CURL �ɼ� ����
		curl_easy_setopt(curl, CURLOPT_URL, rir.url.c_str());
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
			cout << rir.name << " �ٿ�ε� ����: " << curl_easy_strerror(res) << endl;
			free(chunk.memory);
			return false;
		}

		cout << rir.name << " �ٿ�ε� �Ϸ� (" << chunk.size << " bytes)" << endl;

		// �ٿ�ε�� �����͸� ���ڿ��� ��ȯ�ϰ� �Ľ�
		string data(chunk.memory);
		free(chunk.memory);

		return ParseRIRData(data, rir.name);
	}

	/**
	 * �ѱ� Ŭ���̾�Ʈ ó�� ����
	 * @param ip Ŭ���̾�Ʈ IP �ּ�
	 */
	void ProcessKoreanClient(const string& ip)
	{
		cout << "  -> �ѱ� Ŭ���̾�Ʈ ���� ����: " << ip << endl;
	}

	/**
	 * �ܱ� Ŭ���̾�Ʈ ó�� ����
	 * @param ip Ŭ���̾�Ʈ IP �ּ�
	 * @param countryCode ���� �ڵ�
	 */
	void ProcessForeignClient(const string& ip, const string& countryCode)
	{
		cout << "  -> �ܱ� Ŭ���̾�Ʈ (" << countryCode << ") ���� ó��: " << ip << endl;
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
		cout << "=== ��� RIR���� ������ �ٿ�ε� ���� ===" << endl;

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
			cout << "�� " << _ranges.size() << "���� IP ���� �ε� �Ϸ�" << endl;

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
	bool LoadFromFile(const string& fileName)
	{
		ifstream file(fileName);
		if (!file.is_open())
		{
			cout << "[" << fileName << "] ������ �� �� �����ϴ�." << endl;
			return false;
		}

		// ���� ��ü ������ ���ڿ��� �б�
		string content((istreambuf_iterator<char>(file)),
			istreambuf_iterator<char>());
		file.close();

		cout << "���Ͽ��� ������ �ε� ��: " << fileName << endl;
		return ParseRIRData(content, "FILE");
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
		cout << "=== IP ���� Ȯ�α� �ʱ�ȭ ===" << endl;

		bool loaded = false;

		// 1�ܰ�: ĳ�� ���� ��ȿ�� Ȯ��
		if (IsCacheValid(CACHE_FILE))
		{
			cout << "��ȿ�� ĳ�� ���� �߰�, ĳ�ÿ��� �ε� ��..." << endl;
			loaded = LoadFromFile(CACHE_FILE);
			if (loaded)
			{
				// ĳ�ÿ��� �ε� ������ ���� ���� �ð��� ������ ������Ʈ �ð����� ����
				_lastUpdate = GetFileModificationTime(CACHE_FILE);
				cout << "ĳ�ÿ��� �ε� �Ϸ�" << endl;
			}
		}

		// 2�ܰ�: ĳ�ð� ���ų� �ε� ���н� �¶��ο��� �ٿ�ε�
		if (!loaded)
		{
			cout << "�¶��ο��� ������ �ٿ�ε� �õ�..." << endl;
			loaded = DownloadFromAllRIRs();
		}

		// 3�ܰ�: �¶��� �ٿ�ε嵵 �����ϸ� ����� ĳ�ö� ���
		if (!loaded && GetFileModificationTime(CACHE_FILE) != chrono::system_clock::from_time_t(0))
		{
			cout << "�¶��� �ٿ�ε� ����, ����� ĳ�� ���� ���..." << endl;
			loaded = LoadFromFile(CACHE_FILE);
			if (loaded)
			{
				_lastUpdate = GetFileModificationTime(CACHE_FILE);
				cout << "����� ĳ�ÿ��� �ε� �Ϸ� (������Ʈ ����)" << endl;
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
	void HandleClient(const string& clientIP)
	{
		string countryCode = IPCheck(clientIP);

		cout << "[����] Ŭ���̾�Ʈ " << clientIP << " -> " << countryCode;

		// ���� �ڵ忡 ���� ó�� �б�
		if (countryCode == "KR")
		{
			cout << " (�ѱ� IP - ���� ���)" << endl;
			ProcessKoreanClient(clientIP);
		}
		else if (countryCode == "UNKNOWN")
		{
			cout << " (�� �� ���� IP)" << endl;
		}
		else if (countryCode == "INVALID")
		{
			cout << " (�߸��� IP ����)" << endl;
		}
		else
		{
			cout << " (�ܱ� IP - ���� ����)" << endl;
			ProcessForeignClient(clientIP, countryCode);
		}
	}

	/**
	 * ���� Ŭ���̾�Ʈ�� �� ���� ó��
	 * @param clientIPs Ŭ���̾�Ʈ IP �ּ� ���
	 */
	void HandleMultipleClients(const vector<string>& clientIPs)
	{
		cout << "\n=== ��ġ Ŭ���̾�Ʈ ó�� ===" << endl;
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
		cout << "���� ������ ������Ʈ ����..." << endl;
		bool success = DownloadFromAllRIRs();
		if (success)
		{
			cout << "������ ������Ʈ �Ϸ�" << endl;
		}
		else
		{
			cout << "������ ������Ʈ ����" << endl;
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
		cout << "\n=== IP �����ͺ��̽� ��� ===" << endl;
		cout << "�� IP ���� ��: " << _ranges.size() << endl;

		// ��ü IP ���� ���
		unsigned int totalIPs = 0;
		for (const auto& range : _ranges)
		{
			totalIPs += (range.end - range.start + 1);
		}
		cout << "�� IP ����: " << totalIPs << endl;

		// ������ IP ������ ������������ ����
		cout << "\n=== ������ IP ���� (���� 10��) ===" << endl;
		vector<pair<string, int>> sortedStats(_countryStats.begin(), _countryStats.end());
		sort(sortedStats.begin(), sortedStats.end(),
			[](const pair<string, int>& a, const pair<string, int>& b)
			{
				return a.second > b.second;  // IP ���� ���� ��������
			});

		// ���� 10�� ���� ���
		int count = 0;
		for (const auto& stat : sortedStats)
		{
			if (count >= 10) break;
			cout << stat.first << ": " << stat.second << " IPs" << endl;
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
			cout << IntToIP(range.start) << " - " << IntToIP(range.end)
				<< " [" << range.countryCode << "] "
				<< "(" << (range.end - range.start + 1) << " IPs)" << endl;
		}
		if (_ranges.size() > static_cast<size_t>(limit))
		{
			cout << "... (" << (_ranges.size() - limit) << "�� ��)" << endl;
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
					cout << "�ڵ� ������Ʈ ����..." << endl;

					// ��� RIR���� ������ �ٽ� �ٿ�ε�
					if (!DownloadFromAllRIRs())
					{
						cout << "�ڵ� ������Ʈ ����" << endl;
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
	string IPCheck(const string& ip) const
	{
		if (ip == "127.0.0.1")
		{
			return "LOOPBACK";
		}

		unsigned int ipInt = IPToInt(ip);
		if (ipInt == 0) return "INVALID"; // �߸��� IP ����

		return IPCheck(ipInt);
	}

	/**
	 * �ֿ� ���: IP �ּ�(������)�� ���� �ڵ带 ��ȯ
	 * ���� �˻��� ����Ͽ� ���� ��ȸ ���� ����
	 * @param ipInt IP �ּ� (32��Ʈ ����)
	 * @return ���� �ڵ� ("KR", "US", "JP" ��) �Ǵ� "INVALID"/"UNKNOWN"
	 */
	string IPCheck(unsigned int ipInt) const
	{
		if (ipInt == 0) return "INVALID";

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

		return "UNKNOWN"; // �ش��ϴ� ������ ã�� ����
	}

	/**
	 * �ѱ� IP Ȯ�� (���� �ڵ���� ȣȯ�� ����)
	 * @param ip IP �ּ� ���ڿ�
	 * @return �ѱ� IP�̸� true, �ƴϸ� false
	 */
	bool IsKoreanIP(const string& ip) const
	{
		return IPCheck(ip) == "KR";
	}

	/**
	 * �ѱ� IP Ȯ�� (������ IP��)
	 * @param ipInt IP �ּ� (32��Ʈ ����)
	 * @return �ѱ� IP�̸� true, �ƴϸ� false
	 */
	bool IsKoreanIP(unsigned int ipInt) const
	{
		return IPCheck(ipInt) == "KR";
	}

	/**
	 * ĳ�� ���� ���� ���
	 */
	void PrintCacheStatus() const
	{
		cout << "\n=== ĳ�� ���� ���� ===" << endl;

		auto fileTime = GetFileModificationTime(CACHE_FILE);
		if (fileTime == chrono::system_clock::from_time_t(0))
		{
			cout << "ĳ�� ����: ����" << endl;
		}
		else
		{
			auto time_t_val = chrono::system_clock::to_time_t(fileTime);
			struct tm timeinfo;
			localtime_s(&timeinfo, &time_t_val);
			cout << "ĳ�� ����: " << CACHE_FILE << endl;
			cout << "ĳ�� ���� �ð�: " << put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << endl;
			cout << "ĳ�� ��ȿ��: " << (IsCacheValid(CACHE_FILE) ? "��ȿ" : "����") << endl;

			auto now = chrono::system_clock::now();
			auto duration = chrono::duration_cast<chrono::hours>(now - fileTime);
			cout << "��� �ð�: " << duration.count() << "�ð�" << endl;
		}
	}

	// ����
	void Start()
	{
		if (!Initialize())
		{
			cout << "�ʱ�ȭ ����. ���α׷��� �����մϴ�." << endl;
			return;
		}		
	}
};