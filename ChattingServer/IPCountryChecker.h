#pragma once
#include"pch.h"
#include<curl/curl.h>
#include<iomanip>

// IP 범위와 해당 국가 코드를 저장하는 구조체
struct IPRange
{
	unsigned int start; // 범위 시작 IP 
	unsigned int end; // 범위 끝 IP
	string countryCode; // 국가 코드 (예: "KR", "US", "JP")

	IPRange(unsigned int s, unsigned int e, const string& c) : start(s), end(e), countryCode(c) {}

	// 주어진 IP가 범위에 포함되는지 확인
	// ip : 확인할 IP 주소
	// return: 범위에 포함되면 true, 아니면 false
	bool contains(unsigned int ip) const
	{
		return ip >= start && ip <= end;
	}
};

// curl로 다운로드한 데이터를 저장하는 구조체
struct MemoryStruct
{
	char* memory; // 다운로드된 데이터를 저장할 장소
	size_t size;  // 저장된 데이터 크기
};

/**
 * CURL 콜백 함수: 다운로드된 데이터를 메모리에 저장
 * @param contents 다운로드된 데이터 포인터
 * @param size 각 데이터 블록의 크기
 * @param nmemb 데이터 블록의 개수
 * @param userp 사용자 데이터 (MemoryStruct 포인터)
 * @return 처리된 바이트 수
 */
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)userp;

	// 기존 메모리를 확장해서 새 데이터를 추가한다.
	char* ptr = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr) {
		printf("메모리 할당 실패!\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0; // 문자열 종료 문자 추가

	return realsize;
}

// IP 주소의 국가를 확인
// 전 세계 5개 RIR에서 데이터를 수집하여
// IP 주소가 어느 나라에 속하는지 확인한다.
class IPCountryChecker
{
private:
	vector<IPRange> _ranges; // 모든 IP 범위
	chrono::system_clock::time_point _lastUpdate; // 마지막 업데이트 시간
	unordered_map<string, int> _countryStats; // 국가별 IP 개수 통계

	// 캐시 파일 관련 상수
	const string CACHE_FILE = "ip-database.txt";	
	const int UPDATE_INTERVAL_HOURS = 24;

	// RIR 정보를 저장하는 구조체
	struct RIRInfo
	{
		string name; // RIR 이름 (예: "APNIC", "RIPE")
		string url; // 데이터 다운로드 url
		string filePrefix; // 파일 접두사
	};

	/**
	 * 전 세계 5개 RIR 정보 목록
	 * - APNIC: 아시아-태평양 지역
	 * - RIPE: 유럽, 중동, 중앙아시아
	 * - ARIN: 북미
	 * - LACNIC: 라틴아메리카, 카리브해
	 * - AFRINIC: 아프리카
	 */
	vector<RIRInfo> _rirList = {
		{"APNIC", "https://ftp.apnic.net/stats/apnic/delegated-apnic-latest", "apnic"},
		{"RIPE", "https://ftp.ripe.net/ripe/stats/delegated-ripencc-latest", "ripencc"},
		{"ARIN", "https://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest", "arin"},
		{"LACNIC", "https://ftp.lacnic.net/pub/stats/lacnic/delegated-lacnic-latest", "lacnic"},
		{"AFRINIC", "https://ftp.afrinic.net/pub/stats/afrinic/delegated-afrinic-latest", "afrinic"}
	};

	/**
	 * 현재 로드된 데이터를 파일에 저장
	 * @param fileName 저장할 파일명
	 * @return 저장 성공 여부
	 */
	bool SaveToFile(const string& fileName) const
	{
		ofstream file(fileName);
		if (!file.is_open())
		{
			cout << "[" << fileName << "] 파일 저장 실패" << endl;
			return false;
		}

		cout << "데이터를 파일에 저장 중: " << fileName << endl;

		// 헤더 정보 저장 (업데이트 시간)
		auto now = chrono::system_clock::now();
		auto time_t_val = chrono::system_clock::to_time_t(now);
		struct tm timeinfo;
		localtime_s(&timeinfo, &time_t_val);
		file << "# IP Country Database Cache File" << endl;
		file << "# Last Updated: " << put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << endl;
		file << "# Total Ranges: " << _ranges.size() << endl;
		file << "#" << endl;

		// 각 IP 범위를 RIR 형식으로 저장
		for (const auto& range : _ranges)
		{
			// 가상의 RIR 이름과 현재 날짜를 사용하여 표준 형식으로 저장
			string startIP = IntToIP(range.start);
			uint32_t count = range.end - range.start + 1;

			file << "cache|" << range.countryCode << "|ipv4|"
				<< startIP << "|" << count << "|20240101|allocated" << endl;
		}

		file.close();
		cout << fileName << "에 " << _ranges.size() << "개 범위 저장 완료" << endl;
		return true;
	}

	/**
	 * 캐시 파일의 수정 시간을 확인
	 * @param fileName 확인할 파일명
	 * @return 파일의 마지막 수정 시간
	 */
	chrono::system_clock::time_point GetFileModificationTime(const string& fileName) const
	{
		struct stat fileInfo;
		if (stat(fileName.c_str(), &fileInfo) == 0)
		{
			return chrono::system_clock::from_time_t(fileInfo.st_mtime);
		}
		// 파일이 없으면 epoch 시간(1970-01-01 00:00:00) 반환
		return chrono::system_clock::from_time_t(0);
	}

	/**
	 * 캐시 파일이 유효한지 확인 (24시간 이내)
	 * @param fileName 확인할 파일명
	 * @return 캐시가 유효하면 true
	 */
	bool IsCacheValid(const string& fileName) const
	{
		auto fileTime = GetFileModificationTime(fileName);
		// epoch 시간이면 파일이 없음
		if (fileTime == chrono::system_clock::from_time_t(0))
		{
			return false; // 파일이 없음
		}

		auto now = chrono::system_clock::now();
		auto duration = chrono::duration_cast<chrono::hours>(now - fileTime);
		return duration.count() < UPDATE_INTERVAL_HOURS; // 24시간 미만이면 유효
	}

	/**
	 * IP 주소 문자열을 32비트 정수로 변환
	 * 예: "192.168.1.1" -> 3232235777
	 * @param ip IP 주소 문자열 (예: "192.168.1.1")
	 * @return 32비트 정수로 변환된 IP 주소, 실패시 0
	 */
	unsigned IPToInt(const string& ip) const
	{
		istringstream iss(ip);
		string octet;
		unsigned int result = 0;
		int shift = 24; // 첫 번째 옥텟은 24비트 왼쪽으로 시프트

		// 점(.)으로 구분된 각 옥텟을 처리
		while (getline(iss, octet, '.') && shift >= 0)
		{
			try
			{
				int value = stoi(octet);
				if (value < 0 || value > 255) return 0; // 유효하지 않은 옥텟값
				result |= (static_cast<uint32_t>(value) << shift);
				shift -= 8; // 다음 옥텟을 위해 8비트씩 감소
			}
			catch (...) {
				return 0; // 숫자 변환 실패
			}
		}

		return result;
	}

	/**
	 * 32비트 정수를 IP 주소 문자열로 변환
	 * 예: 3232235777 -> "192.168.1.1"
	 * @param ip 32비트 정수 IP 주소
	 * @return IP 주소 문자열
	 */
	string IntToIP(unsigned int ip) const
	{
		return to_string((ip >> 24) & 0xFF) + "." + // 첫 번째 옥텟
			to_string((ip >> 16) & 0xFF) + "." +	// 두 번째 옥텟
			to_string((ip >> 8) & 0xFF) + "." +		// 세 번째 옥텟
			to_string(ip & 0xFF);					// 네 번째 옥텟
	}

	/**
	 * RIR 데이터를 파싱하여 IP 범위 정보를 추출
	 * 데이터 형식: RIR|국가코드|ipv4|시작IP|개수|날짜|상태
	 * 예: apnic|KR|ipv4|1.208.0.0|4096|20110101|allocated
	 * @param data 파싱할 RIR 데이터 문자열
	 * @param rirName RIR 이름 (로그용)
	 * @return 파싱 성공 여부
	 */
	bool ParseRIRData(const string& data, const string& rirName)
	{
		vector<IPRange> newRanges;
		istringstream stream(data);
		string line;
		int parsedCount = 0;

		// 데이터를 한 줄씩 처리
		while (getline(stream, line))
		{
			// 빈 줄이나 주석(#으로 시작) 건너뛰기
			if (line.empty() || line[0] == '#') continue;

			// 파이프(|)로 구분된 필드들을 분리
			vector<string> tokens;
			stringstream ss(line);
			string token;

			while (getline(ss, token, '|'))
			{
				tokens.push_back(token);
			}

			// 최소 5개 필드가 있고, IPv4 레코드인지 확인
			if (tokens.size() >= 5 && tokens[2] == "ipv4")
			{
				try
				{
					string countryCode = tokens[1];    // 국가 코드 (예: KR, US, JP)
					string startIP = tokens[3];		   // 시작 IP 주소
					uint32_t count = stoul(tokens[4]); // IP 개수

					// 유효한 국가 코드(2글자)와 IP 개수 확인
					if (countryCode.length() == 2 && count > 0)
					{
						uint32_t start = IPToInt(startIP);
						if (start != 0)
						{
							uint32_t end = start + count - 1; // 끝 IP 계산
							newRanges.emplace_back(start, end, countryCode);
							parsedCount++;

							// 국가별 통계 업데이트
							_countryStats[countryCode] += count;
						}
					}
				}
				catch (...)
				{
					continue;  // 파싱 오류 시 해당 라인 건너뛰기
				}
			}
		}

		cout << rirName << "에서 " << parsedCount << "개의 IP 범위 파싱 완료" << endl;

		// 새로운 범위를 기존 범위 벡터에 추가
		_ranges.insert(_ranges.end(), newRanges.begin(), newRanges.end());

		return parsedCount > 0;
	}

	/**
	 * 단일 RIR에서 데이터를 다운로드
	 * @param rir 다운로드할 RIR 정보
	 * @return 다운로드 및 파싱 성공 여부
	 */
	bool DownloadFromRIR(const RIRInfo& rir)
	{
		CURL* curl;
		CURLcode res;

		// 다운로드된 데이터를 저장할 메모리 구조체 초기화
		MemoryStruct chunk;
		chunk.memory = (char*)malloc(1);
		chunk.size = 0;

		// CURL 초기화
		curl = curl_easy_init();
		if (!curl)
		{
			free(chunk.memory);
			cout << rir.name << " CURL 초기화 실패" << endl;
			return false;
		}

		cout << rir.name << "에서 데이터 다운로드 중..." << endl;

		// CURL 옵션 설정
		curl_easy_setopt(curl, CURLOPT_URL, rir.url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "IP-Country-Checker/1.0");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

		// 다운로드 실행
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		// 다운로드 결과 확인
		if (res != CURLE_OK)
		{
			cout << rir.name << " 다운로드 실패: " << curl_easy_strerror(res) << endl;
			free(chunk.memory);
			return false;
		}

		cout << rir.name << " 다운로드 완료 (" << chunk.size << " bytes)" << endl;

		// 다운로드된 데이터를 문자열로 변환하고 파싱
		string data(chunk.memory);
		free(chunk.memory);

		return ParseRIRData(data, rir.name);
	}

	/**
	 * 한국 클라이언트 처리 로직
	 * @param ip 클라이언트 IP 주소
	 */
	void ProcessKoreanClient(const string& ip)
	{
		cout << "  -> 한국 클라이언트 서비스 제공: " << ip << endl;
	}

	/**
	 * 외국 클라이언트 처리 로직
	 * @param ip 클라이언트 IP 주소
	 * @param countryCode 국가 코드
	 */
	void ProcessForeignClient(const string& ip, const string& countryCode)
	{
		cout << "  -> 외국 클라이언트 (" << countryCode << ") 접근 처리: " << ip << endl;
	}

public:
	IPCountryChecker()
	{
		// curl 라이브러리 초기화
		curl_global_init(CURL_GLOBAL_DEFAULT);
	}

	~IPCountryChecker()
	{
		// curl 라이브러리 정리
		curl_global_cleanup();
	}
private:
	/**
	 * 모든 RIR에서 데이터를 다운로드하고 파싱
	 * 전 세계 5개 RIR에서 순차적으로 데이터를 다운로드합니다.
	 * @return 하나 이상의 RIR에서 성공적으로 다운로드했으면 true
	 */
	bool DownloadFromAllRIRs()
	{
		cout << "=== 모든 RIR에서 데이터 다운로드 시작 ===" << endl;

		// 기존 데이터 초기화
		_ranges.clear();
		_countryStats.clear();

		bool anySuccess = false;

		// 각 RIR에서 순차적으로 다운로드
		for (const auto& rir : _rirList)
		{
			if (DownloadFromRIR(rir))
			{
				anySuccess = true;
			}

			// 서버 부하 방지를 위해 각 다운로드 사이에 0.5초 대기
			this_thread::sleep_for(chrono::milliseconds(500));
		}

		if (anySuccess)
		{
			// IP 범위를 시작 주소 기준으로 정렬 (이진 검색 최적화)
			sort(_ranges.begin(), _ranges.end(),
				[](const IPRange& a, const IPRange& b)
				{
					return a.start < b.start;
				});

			_lastUpdate = chrono::system_clock::now();
			cout << "총 " << _ranges.size() << "개의 IP 범위 로드 완료" << endl;

			// 새로 추가: 성공적으로 다운로드하면 캐시 파일에 저장
			SaveToFile(CACHE_FILE);
		}

		return anySuccess;
	}

	/**
	 * 파일에서 IP 데이터를 로드 (백업용)
	 * 네트워크 다운로드가 실패했을 때 사용
	 * @param fileName 로드할 파일명
	 * @return 로드 성공 여부
	 */
	bool LoadFromFile(const string& fileName)
	{
		ifstream file(fileName);
		if (!file.is_open())
		{
			cout << "[" << fileName << "] 파일을 열 수 없습니다." << endl;
			return false;
		}

		// 파일 전체 내용을 문자열로 읽기
		string content((istreambuf_iterator<char>(file)),
			istreambuf_iterator<char>());
		file.close();

		cout << "파일에서 데이터 로드 중: " << fileName << endl;
		return ParseRIRData(content, "FILE");
	}

	/**
	 * IP 국가 확인기 초기화
	 * 1. 캐시 파일이 유효하면 캐시에서 로드
	 * 2. 캐시가 없거나 만료되면 온라인에서 다운로드
	 * 3. 온라인 다운로드 실패시 만료된 캐시라도 사용
	 * @return 초기화 성공 여부
	 */
	bool Initialize()
	{
		cout << "=== IP 국가 확인기 초기화 ===" << endl;

		bool loaded = false;

		// 1단계: 캐시 파일 유효성 확인
		if (IsCacheValid(CACHE_FILE))
		{
			cout << "유효한 캐시 파일 발견, 캐시에서 로드 중..." << endl;
			loaded = LoadFromFile(CACHE_FILE);
			if (loaded)
			{
				// 캐시에서 로드 성공시 파일 수정 시간을 마지막 업데이트 시간으로 설정
				_lastUpdate = GetFileModificationTime(CACHE_FILE);
				cout << "캐시에서 로드 완료" << endl;
			}
		}

		// 2단계: 캐시가 없거나 로드 실패시 온라인에서 다운로드
		if (!loaded)
		{
			cout << "온라인에서 데이터 다운로드 시도..." << endl;
			loaded = DownloadFromAllRIRs();
		}

		// 3단계: 온라인 다운로드도 실패하면 만료된 캐시라도 사용
		if (!loaded && GetFileModificationTime(CACHE_FILE) != chrono::system_clock::from_time_t(0))
		{
			cout << "온라인 다운로드 실패, 만료된 캐시 파일 사용..." << endl;
			loaded = LoadFromFile(CACHE_FILE);
			if (loaded)
			{
				_lastUpdate = GetFileModificationTime(CACHE_FILE);
				cout << "만료된 캐시에서 로드 완료 (업데이트 권장)" << endl;
			}
		}

		// 4단계: 로드 성공시 통계 출력
		if (loaded)
		{
			PrintStatistics();
		}

		return loaded;
	}	

	/**
	 * 클라이언트 처리 (접속한 클라이언트의 국가에 따른 처리)
	 * @param clientIP 클라이언트 IP 주소
	 */
	void HandleClient(const string& clientIP)
	{
		string countryCode = IPCheck(clientIP);

		cout << "[서버] 클라이언트 " << clientIP << " -> " << countryCode;

		// 국가 코드에 따른 처리 분기
		if (countryCode == "KR")
		{
			cout << " (한국 IP - 접근 허용)" << endl;
			ProcessKoreanClient(clientIP);
		}
		else if (countryCode == "UNKNOWN")
		{
			cout << " (알 수 없는 IP)" << endl;
		}
		else if (countryCode == "INVALID")
		{
			cout << " (잘못된 IP 형식)" << endl;
		}
		else
		{
			cout << " (외국 IP - 접근 제한)" << endl;
			ProcessForeignClient(clientIP, countryCode);
		}
	}

	/**
	 * 여러 클라이언트를 한 번에 처리
	 * @param clientIPs 클라이언트 IP 주소 목록
	 */
	void HandleMultipleClients(const vector<string>& clientIPs)
	{
		cout << "\n=== 배치 클라이언트 처리 ===" << endl;
		for (const auto& ip : clientIPs)
		{
			HandleClient(ip);
		}
	}

	/**
	 * 로드된 IP 범위 개수 반환
	 * @return IP 범위 개수
	 */
	size_t GetRangeCount() const
	{
		return _ranges.size();
	}

	/**
	 * 마지막 업데이트 시간 반환
	 * @return 마지막 업데이트 시간
	 */
	chrono::system_clock::time_point GetLastUpdateTime() const
	{
		return _lastUpdate;
	}

	/**
	 * 데이터 업데이트가 필요한지 확인 (수정된 버전)
	 * 캐시 파일 기준으로 24시간 경과 여부 확인
	 * @return 업데이트가 필요하면 true
	 */
	bool NeedsUpdate() const
	{
		return !IsCacheValid(CACHE_FILE);
	}

	/**
	 * 수동으로 데이터 업데이트 실행
	 * @return 업데이트 성공 여부
	 */
	bool UpdateData()
	{
		cout << "수동 데이터 업데이트 시작..." << endl;
		bool success = DownloadFromAllRIRs();
		if (success)
		{
			cout << "데이터 업데이트 완료" << endl;
		}
		else
		{
			cout << "데이터 업데이트 실패" << endl;
		}
		return success;
	}

	/**
	 * 데이터베이스 통계 정보 출력
	 * - 총 IP 범위 수
	 * - 총 IP 개수
	 * - 국가별 IP 개수 (상위 10개국)
	 */
	void PrintStatistics() const
	{
		cout << "\n=== IP 데이터베이스 통계 ===" << endl;
		cout << "총 IP 범위 수: " << _ranges.size() << endl;

		// 전체 IP 개수 계산
		unsigned int totalIPs = 0;
		for (const auto& range : _ranges)
		{
			totalIPs += (range.end - range.start + 1);
		}
		cout << "총 IP 개수: " << totalIPs << endl;

		// 국가별 IP 개수를 내림차순으로 정렬
		cout << "\n=== 국가별 IP 개수 (상위 10개) ===" << endl;
		vector<pair<string, int>> sortedStats(_countryStats.begin(), _countryStats.end());
		sort(sortedStats.begin(), sortedStats.end(),
			[](const pair<string, int>& a, const pair<string, int>& b)
			{
				return a.second > b.second;  // IP 개수 기준 내림차순
			});

		// 상위 10개 국가 출력
		int count = 0;
		for (const auto& stat : sortedStats)
		{
			if (count >= 10) break;
			cout << stat.first << ": " << stat.second << " IPs" << endl;
			count++;
		}
	}

	/**
	 * IP 범위 목록 출력 (디버깅용)
	 * @param limit 출력할 최대 범위 수 (기본값: 10)
	 */
	void PrintRanges(int limit = 10) const
	{
		cout << "\n=== IP 범위 목록 (최대 " << limit << "개) ===" << endl;
		for (size_t i = 0; i < _ranges.size() && i < static_cast<size_t>(limit); ++i)
		{
			const auto& range = _ranges[i];
			cout << IntToIP(range.start) << " - " << IntToIP(range.end)
				<< " [" << range.countryCode << "] "
				<< "(" << (range.end - range.start + 1) << " IPs)" << endl;
		}
		if (_ranges.size() > static_cast<size_t>(limit))
		{
			cout << "... (" << (_ranges.size() - limit) << "개 더)" << endl;
		}
	}

	/**
	 * 자동 업데이트 백그라운드 작업 시작
	 * 별도 스레드에서 주기적으로 데이터를 업데이트합니다.
	 * @param intervalHours 업데이트 간격 (시간 단위, 기본값: 24시간)
	 */
	void StartAutoUpdate(int intervalHours = 24)
	{
		thread([this, intervalHours]()
			{
				while (true)
				{
					// 지정된 시간만큼 대기
					this_thread::sleep_for(chrono::hours(intervalHours));
					cout << "자동 업데이트 시작..." << endl;

					// 모든 RIR에서 데이터 다시 다운로드
					if (!DownloadFromAllRIRs())
					{
						cout << "자동 업데이트 실패" << endl;
					}
				}
			}).detach();  // 스레드를 분리하여 백그라운드에서 실행
	}
public:

	/**
	 * 주요 기능: IP 주소의 국가 코드를 반환
	 * @param ip IP 주소 문자열 (예: "8.8.8.8")
	 * @return 국가 코드 ("KR", "US", "JP" 등) 또는 "INVALID"/"UNKNOWN"
	 */
	string IPCheck(const string& ip) const
	{
		if (ip == "127.0.0.1")
		{
			return "LOOPBACK";
		}

		unsigned int ipInt = IPToInt(ip);
		if (ipInt == 0) return "INVALID"; // 잘못된 IP 형식

		return IPCheck(ipInt);
	}

	/**
	 * 주요 기능: IP 주소(정수형)의 국가 코드를 반환
	 * 이진 검색을 사용하여 빠른 조회 성능 제공
	 * @param ipInt IP 주소 (32비트 정수)
	 * @return 국가 코드 ("KR", "US", "JP" 등) 또는 "INVALID"/"UNKNOWN"
	 */
	string IPCheck(unsigned int ipInt) const
	{
		if (ipInt == 0) return "INVALID";

		// 이진 검색으로 해당 IP를 포함하는 범위 찾기
		auto it = lower_bound(_ranges.begin(), _ranges.end(), ipInt,
			[](const IPRange& range, unsigned int ip)
			{
				return range.end < ip; // 범위의 끝이 IP보다 작으면 true
			});

		// 찾은 범위가 실제로 IP를 포함하는지 확인
		if (it != _ranges.end() && it->contains(ipInt))
		{
			return it->countryCode;
		}

		return "UNKNOWN"; // 해당하는 범위를 찾지 못함
	}

	/**
	 * 한국 IP 확인 (기존 코드와의 호환성 유지)
	 * @param ip IP 주소 문자열
	 * @return 한국 IP이면 true, 아니면 false
	 */
	bool IsKoreanIP(const string& ip) const
	{
		return IPCheck(ip) == "KR";
	}

	/**
	 * 한국 IP 확인 (정수형 IP용)
	 * @param ipInt IP 주소 (32비트 정수)
	 * @return 한국 IP이면 true, 아니면 false
	 */
	bool IsKoreanIP(unsigned int ipInt) const
	{
		return IPCheck(ipInt) == "KR";
	}

	/**
	 * 캐시 상태 정보 출력
	 */
	void PrintCacheStatus() const
	{
		cout << "\n=== 캐시 상태 정보 ===" << endl;

		auto fileTime = GetFileModificationTime(CACHE_FILE);
		if (fileTime == chrono::system_clock::from_time_t(0))
		{
			cout << "캐시 파일: 없음" << endl;
		}
		else
		{
			auto time_t_val = chrono::system_clock::to_time_t(fileTime);
			struct tm timeinfo;
			localtime_s(&timeinfo, &time_t_val);
			cout << "캐시 파일: " << CACHE_FILE << endl;
			cout << "캐시 생성 시간: " << put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") << endl;
			cout << "캐시 유효성: " << (IsCacheValid(CACHE_FILE) ? "유효" : "만료") << endl;

			auto now = chrono::system_clock::now();
			auto duration = chrono::duration_cast<chrono::hours>(now - fileTime);
			cout << "경과 시간: " << duration.count() << "시간" << endl;
		}
	}

	// 시작
	void Start()
	{
		if (!Initialize())
		{
			cout << "초기화 실패. 프로그램을 종료합니다." << endl;
			return;
		}		
	}
};