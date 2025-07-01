#pragma once

#include<curl/curl.h>
#include"../Logger.h"

// IP 범위와 해당 국가 코드를 저장하는 구조체
struct IPRange
{
	unsigned int start; // 범위 시작 IP 
	unsigned int end; // 범위 끝 IP
	wstring countryCode; // 국가 코드 (예: "KR", "US", "JP")

	IPRange(unsigned int s, unsigned int e, const wstring& c) : start(s), end(e), countryCode(c) {}

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
		LOG_CRITICAL(L"메모리 할당 실패");		
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
	unordered_map<wstring, int> _countryStats; // 국가별 IP 개수 통계

	// 캐시 파일 관련 상수
	const wstring CACHE_FILE = L"ip-database.txt";	
	const int UPDATE_INTERVAL_HOURS = 24;

	// RIR 정보를 저장하는 구조체
	struct RIRInfo
	{
		wstring name; // RIR 이름 (예: "APNIC", "RIPE")
		wstring url; // 데이터 다운로드 url
		wstring filePrefix; // 파일 접두사
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
		{L"APNIC", L"https://ftp.apnic.net/stats/apnic/delegated-apnic-latest", L"apnic"},
		{L"RIPE", L"https://ftp.ripe.net/ripe/stats/delegated-ripencc-latest", L"ripencc"},
		{L"ARIN", L"https://ftp.arin.net/pub/stats/arin/delegated-arin-extended-latest", L"arin"},
		{L"LACNIC", L"https://ftp.lacnic.net/pub/stats/lacnic/delegated-lacnic-latest", L"lacnic"},
		{L"AFRINIC", L"https://ftp.afrinic.net/pub/stats/afrinic/delegated-afrinic-latest", L"afrinic"}
	};

	/**
	 * 현재 로드된 데이터를 파일에 저장
	 * @param fileName 저장할 파일명
	 * @return 저장 성공 여부
	 */
	bool SaveToFile(const wstring& fileName) const
	{
		wofstream file(fileName);
		if (!file.is_open())
		{
			LOG_ERRORF(L"[%s] 파일 저장 실패", fileName.c_str());			
			return false;
		}		

		LOG_DEBUGF(L"데이터를 파일에 저장 중: %s", fileName.c_str());		

		// 헤더 정보 저장 (업데이트 시간)
		auto now = chrono::system_clock::now();
		auto time_t_val = chrono::system_clock::to_time_t(now);
		struct tm timeinfo;
		localtime_s(&timeinfo, &time_t_val);

		file << L"# IP Country Database Cache File" << endl;
		file << L"# Last Updated: " << put_time(&timeinfo, L"%Y-%m-%d %H:%M:%S") << endl;
		file << L"# Total Ranges: " << _ranges.size() << endl;
		file << L"#" << endl;

		// 각 IP 범위를 RIR 형식으로 저장
		for (const auto& range : _ranges)
		{
			// 가상의 RIR 이름과 현재 날짜를 사용하여 표준 형식으로 저장
			wstring startIP = IntToIP(range.start);
			uint32_t count = range.end - range.start + 1;

			file << L"cache|" << range.countryCode << L"|ipv4|"
				<< startIP << L"|" << count << L"|20240101|allocated" << endl;
		}

		file.close();
		LOG_DEBUGF(L"%s에 %d개 범위 저장 완료", fileName.c_str(), (int)_ranges.size());		
		return true;
	}

	/**
	 * 캐시 파일의 수정 시간을 확인
	 * @param fileName 확인할 파일명
	 * @return 파일의 마지막 수정 시간
	 */
	chrono::system_clock::time_point GetFileModificationTime(const wstring& fileName) const
	{
		struct _stat fileInfo;
		if (_wstat(fileName.c_str(), &fileInfo) == 0)
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
	bool IsCacheValid(const wstring& fileName) const
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
	unsigned IPToInt(const wstring& ip) const
	{
		wistringstream iss(ip);
		wstring octet;
		unsigned int result = 0;
		int shift = 24; // 첫 번째 옥텟은 24비트 왼쪽으로 시프트

		// 점(.)으로 구분된 각 옥텟을 처리
		while (getline(iss, octet, L'.') && shift >= 0)
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
	wstring IntToIP(unsigned int ip) const
	{
		return to_wstring((ip >> 24) & 0xFF) + L"." + // 첫 번째 옥텟
			to_wstring((ip >> 16) & 0xFF) + L"." +	// 두 번째 옥텟
			to_wstring((ip >> 8) & 0xFF) + L"." +		// 세 번째 옥텟
			to_wstring(ip & 0xFF);					// 네 번째 옥텟
	}

	/**
	 * RIR 데이터를 파싱하여 IP 범위 정보를 추출
	 * 데이터 형식: RIR|국가코드|ipv4|시작IP|개수|날짜|상태
	 * 예: apnic|KR|ipv4|1.208.0.0|4096|20110101|allocated
	 * @param data 파싱할 RIR 데이터 문자열
	 * @param rirName RIR 이름 (로그용)
	 * @return 파싱 성공 여부
	 */
	bool ParseRIRData(const wstring& data, const wstring& rirName)
	{
		vector<IPRange> newRanges;
		wistringstream stream(data);
		wstring line;
		int parsedCount = 0;

		// 데이터를 한 줄씩 처리
		while (getline(stream, line))
		{
			// 빈 줄이나 주석(#으로 시작) 건너뛰기
			if (line.empty() || line[0] == L'#') continue;

			// 파이프(|)로 구분된 필드들을 분리
			vector<wstring> tokens;
			wstringstream ss(line);
			wstring token;

			while (getline(ss, token, L'|'))
			{
				tokens.push_back(token);
			}

			// 최소 5개 필드가 있고, IPv4 레코드인지 확인
			if (tokens.size() >= 5 && tokens[2] == L"ipv4")
			{
				try
				{
					wstring countryCode = tokens[1];    // 국가 코드 (예: KR, US, JP)
					wstring startIP = tokens[3];		   // 시작 IP 주소
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

		LOG_DEBUGF(L"%s에서 %d개의 IP 범위 파싱 완료", rirName.c_str(), parsedCount);

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
			LOG_ERRORF(L"%s CURL 초기화 실패", rir.name.c_str());			
			return false;
		}

		LOG_DEBUGF(L"%s에서 데이터 다운로드 중...", rir.name.c_str());		

		string urlStr(rir.url.begin(), rir.url.end());

		// CURL 옵션 설정
		curl_easy_setopt(curl, CURLOPT_URL, urlStr.c_str());
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
			LOG_ERRORF(L"%s 다운로드 실패", rir.name.c_str());			
			free(chunk.memory);
			return false;
		}

		LOG_DEBUGF(L"%s 다운로드 완료 (%d bytes)", rir.name.c_str(), (int)chunk.size);		

		// 다운로드된 데이터를 문자열로 변환하고 파싱
		string tempData(chunk.memory);
		wstring data(tempData.begin(), tempData.end());
		free(chunk.memory);

		return ParseRIRData(data, rir.name);
	}

	/**
	 * 한국 클라이언트 처리 로직
	 * @param ip 클라이언트 IP 주소
	 */
	void ProcessKoreanClient(const wstring& ip)
	{
		LOG_INFO(L" -> 한국 클라이언트 서비스 제공");		
	}

	/**
	 * 외국 클라이언트 처리 로직
	 * @param ip 클라이언트 IP 주소
	 * @param countryCode 국가 코드
	 */
	void ProcessForeignClient(const wstring& ip, const wstring& countryCode)
	{
		LOG_INFOF(L" -> 외국 클라이언트 (%s) 접근 처리: %s",countryCode.c_str(), ip.c_str());		
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
		LOG_DEBUG(L"=== 모든 RIR에서 데이터 다운로드 시작 ===");

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
			LOG_DEBUGF(L"총 %d개의 IP 범위 로드 완료", (int)_ranges.size());

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
	bool LoadFromFile(const wstring& fileName)
	{
		wifstream file(fileName);
		if (!file.is_open())
		{
			LOG_ERRORF(L"[%s] 파일을 열 수 없습니다.", fileName.c_str());			
			return false;
		}

		// 파일 전체 내용을 문자열로 읽기
		wstring content((istreambuf_iterator<wchar_t>(file)),
			istreambuf_iterator<wchar_t>());
		file.close();

		LOG_DEBUGF(L"파일에서 데이터 로드 중: %s", fileName.c_str());
		
		return ParseRIRData(content, L"FILE");
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
		LOG_DEBUG(L"=== IP 국가 확인기 초기화 ===");

		bool loaded = false;

		// 1단계: 캐시 파일 유효성 확인
		if (IsCacheValid(CACHE_FILE))
		{
			LOG_DEBUG(L"유효한 캐시 파일 발견, 캐시에서 로드 중...");			
			
			loaded = LoadFromFile(CACHE_FILE);
			if (loaded)
			{
				// 캐시에서 로드 성공시 파일 수정 시간을 마지막 업데이트 시간으로 설정
				_lastUpdate = GetFileModificationTime(CACHE_FILE);

				LOG_DEBUG(L"캐시에서 로드 완료");				
			}
		}

		// 2단계: 캐시가 없거나 로드 실패시 온라인에서 다운로드
		if (!loaded)
		{
			LOG_DEBUG(L"온라인에서 데이터 다운로드 시도...");			
			loaded = DownloadFromAllRIRs();
		}

		// 3단계: 온라인 다운로드도 실패하면 만료된 캐시라도 사용
		if (!loaded && GetFileModificationTime(CACHE_FILE) != chrono::system_clock::from_time_t(0))
		{
			LOG_DEBUG(L"온라인 다운로드 실패, 만료된 캐시 파일 사용...");			
			loaded = LoadFromFile(CACHE_FILE);
			if (loaded)
			{
				_lastUpdate = GetFileModificationTime(CACHE_FILE);
				LOG_DEBUG(L"만료된 캐시에서 로드 완료 (업데이트 권장)");				
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
	void HandleClient(const wstring& clientIP)
	{
		wstring countryCode = IPCheck(clientIP);

		LOG_DEBUGF(L"[서버] 클라이언트 %s ->", countryCode);		

		// 국가 코드에 따른 처리 분기
		if (countryCode == L"KR")
		{
			LOG_DEBUG(L" (한국 IP - 접근 허용)");			
			ProcessKoreanClient(clientIP);
		}
		else if (countryCode == L"UNKNOWN")
		{
			LOG_DEBUG(L" (알 수 없는 IP)");			
		}
		else if (countryCode == L"INVALID")
		{
			LOG_DEBUG(L" (잘못된 IP 형식)");			
		}
		else
		{
			LOG_DEBUG(L" (외국 IP - 접근 제한)");			
			ProcessForeignClient(clientIP, countryCode);
		}
	}

	/**
	 * 여러 클라이언트를 한 번에 처리
	 * @param clientIPs 클라이언트 IP 주소 목록
	 */
	void HandleMultipleClients(const vector<wstring>& clientIPs)
	{
		LOG_DEBUG(L"=== 배치 클라이언트 처리 ===");
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
		LOG_DEBUG(L"수동 데이터 업데이트 시작...");
		bool success = DownloadFromAllRIRs();
		if (success)
		{
			LOG_DEBUG(L"데이터 업데이트 완료");
		}
		else
		{
			LOG_DEBUG(L"데이터 업데이트 실패");
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
		LOG_DEBUG(L"\n=== IP 데이터베이스 통계 ===");
		LOG_DEBUGF(L"총 IP 범위 수: %d", (int)_ranges.size());		

		// 전체 IP 개수 계산
		unsigned int totalIPs = 0;
		for (const auto& range : _ranges)
		{
			totalIPs += (range.end - range.start + 1);
		}

		LOG_DEBUGF(L"총 IP 범위 수: %d", (int)_ranges.size());
		LOG_DEBUGF(L"총 IP 개수: %d", (int)_ranges.size());		

		// 국가별 IP 개수를 내림차순으로 정렬.
		LOG_DEBUG(L"\n=== 국가별 IP 개수 (상위 10개) ===");
		
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

		// 상위 10개 국가 출력
		int count = 0;
		for (const auto& stat : sortedStats)
		{
			if (count >= 10) break;			

			LOG_DEBUGF(L"%s: %d IPs", stat.first.c_str(), stat.second);
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
			LOG_DEBUGF(L"%s - %s [%s] (%d IPs)", IntToIP(range.start), IntToIP(range.end), range.countryCode, (int)(range.end - range.start + 1));			
		}

		if (_ranges.size() > static_cast<size_t>(limit))
		{
			LOG_DEBUGF(L"... (%d개 더)", (int)(_ranges.size()) - limit);			
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
					LOG_DEBUG(L"자동 업데이트 시작...");

					// 모든 RIR에서 데이터 다시 다운로드
					if (!DownloadFromAllRIRs())
					{
						LOG_DEBUG(L"자동 업데이트 실패");
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
	wstring IPCheck(const wstring& ip) const
	{
		if (ip == L"127.0.0.1")
		{
			return L"LOOPBACK";
		}

		unsigned int ipInt = IPToInt(ip);
		if (ipInt == 0) return L"INVALID"; // 잘못된 IP 형식

		return IPCheck(ipInt);
	}

	/**
	 * 주요 기능: IP 주소(정수형)의 국가 코드를 반환
	 * 이진 검색을 사용하여 빠른 조회 성능 제공
	 * @param ipInt IP 주소 (32비트 정수)
	 * @return 국가 코드 ("KR", "US", "JP" 등) 또는 "INVALID"/"UNKNOWN"
	 */
	wstring IPCheck(unsigned int ipInt) const
	{
		if (ipInt == 0) return L"INVALID";

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

		return L"UNKNOWN"; // 해당하는 범위를 찾지 못함
	}

	/**
	 * 한국 IP 확인 (기존 코드와의 호환성 유지)
	 * @param ip IP 주소 문자열
	 * @return 한국 IP이면 true, 아니면 false
	 */
	bool IsKoreanIP(const wstring& ip) const
	{
		return IPCheck(ip) == L"KR";
	}

	/**
	 * 한국 IP 확인 (정수형 IP용)
	 * @param ipInt IP 주소 (32비트 정수)
	 * @return 한국 IP이면 true, 아니면 false
	 */
	bool IsKoreanIP(unsigned int ipInt) const
	{
		return IPCheck(ipInt) == L"KR";
	}

	/**
	 * 캐시 상태 정보 출력
	 */
	void PrintCacheStatus() const
	{
		LOG_DEBUG(L"\n=== 캐시 상태 정보 ===");		

		auto fileTime = GetFileModificationTime(CACHE_FILE);
		if (fileTime == chrono::system_clock::from_time_t(0))
		{
			LOG_DEBUG(L"캐시 파일: 없음");			
		}
		else
		{
			auto time_t_val = chrono::system_clock::to_time_t(fileTime);
			struct tm timeinfo;
			localtime_s(&timeinfo, &time_t_val);			

			LOG_DEBUGF(L"캐시 파일 %s", CACHE_FILE);			
			wcout << L"캐시 생성 시간: " << put_time(&timeinfo, L"%Y-%m-%d %H:%M:%S") << endl;
			wcout << L"캐시 유효성: " << (IsCacheValid(CACHE_FILE) ? L"유효" : L"만료") << endl;

			auto now = chrono::system_clock::now();
			auto duration = chrono::duration_cast<chrono::hours>(now - fileTime);
			LOG_DEBUGF(L"경과 시간: %d시간", duration.count());			
		}
	}

	// 시작
	void Start()
	{
		if (!Initialize())
		{
			LOG_ERROR(L"초기화 실패. 프로그램을 종료합니다.");			
			return;
		}		
	}
};