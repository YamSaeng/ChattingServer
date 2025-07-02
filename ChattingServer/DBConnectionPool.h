#pragma once
#pragma comment(lib, "mysqlcppconn.lib")

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>

#include <codecvt>

#include"../Logger.h"

/*
	MySQL 데이터베이스 단일 연결을 관리하는 클래스

	- 연결 생성, 쿼리 실행, 트랜잭션 관리 기능 포함
	- 연결 유효성 검사 및 만료 시간 관리
	- RAII 패턴을 사용하여 연결 자원 관리
	- 트랜잭션 시작, 커밋, 롤백 기능 제공
	- 연결 풀에서 사용되는 연결 객체로 설계됨
*/
class MySQLConnection
{
private:
	sql::mysql::MySQL_Driver* _driver; // MySQL 드라이버 인스턴스
	sql::Connection* _connection; // 실제 데이터베이스 연결 객체
	chrono::steady_clock::time_point _lastUsedTime; // 마지막 사용 시간 (만료 체크용)

	/*
		wstring을 UTF-8 string으로 변환
	*/
	string WStringToString(const wstring& wstr)
	{
		if (wstr.empty()) return {};

		int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
			nullptr, 0, nullptr, nullptr);
		string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
			&strTo[0], size_needed, nullptr, nullptr);
		return strTo;
	}

	/*
		UTF-8 string을 wstring으로 변환
	*/
	wstring StringToWString(const string& str)
	{
		if (str.empty()) return {};

		int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(),
			nullptr, 0);
		wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(),
			&wstrTo[0], size_needed);
		return wstrTo;
	}
public:
	/*
		기본 생성자
		연결 객체들을 초기화하고 현재 시간을 마지막 사용 시간으로 설정한다.
	*/
	MySQLConnection() : _driver(nullptr), _connection(nullptr)
	{
		_lastUsedTime = chrono::steady_clock::now();
	}

	/*
		소멸자
		연결 객체를 삭제하여 자원을 해제한다.
	*/
	~MySQLConnection()
	{
		if (_connection)
		{
			delete _connection;
		}
	}

	/*
		데이터베이스에 연결하는 메서드
		- host: 데이터베이스 호스트 주소
		- user: 사용자 이름
		- password: 비밀번호
		- database: 사용할 데이터베이스 이름
		- 성공 시 true, 실패 시 false 반환
	*/
	bool Connect(const wstring& host, const wstring& user,
		const wstring& password, const wstring& database)
	{
		try
		{
			_driver = sql::mysql::get_mysql_driver_instance();

			_connection = _driver->connect(
				WStringToString(host),
				WStringToString(user),
				WStringToString(password)
			);

			_connection->setSchema(WStringToString(database));

			LOG_DEBUGF(L"MySQL host: %s user: %s", host.c_str(), user.c_str());

			_lastUsedTime = chrono::steady_clock::now();
			return true;
		}
		catch (sql::SQLException& e)
		{
			LOG_ERRORF(L"DB Connect fail %s", StringToWString(e.what()));
			return false;
		}
	}

	/*
		SELECT 쿼리를 실행하고 결과를 콘솔에 출력하는 메서드
		- query: 실행할 SQL SELECT 쿼리
	*/
	void ExecuteSelect(const wstring& query)
	{
		try
		{
			UpdateLastUsed();
			sql::Statement* stmt = _connection->createStatement();
			sql::ResultSet* res = stmt->executeQuery(WStringToString(query));

			// 결과 메타데이터를 가져와서 컬럼 이름 출력
			sql::ResultSetMetaData* meta = res->getMetaData();
			int columnCount = meta->getColumnCount();

			for (int i = 1; i <= columnCount; ++i)
			{
				LOG_INFOF(L"%s\t", StringToWString(meta->getColumnName(i)));
			}

			wcout << endl;

			// 결과 행을 출력
			while (res->next())
			{
				for (int i = 1; i <= columnCount; ++i)
				{
					LOG_INFOF(L"%s\t", StringToWString(res->getString(i)));
				}

				wcout << endl;
			}

			delete res;
			delete stmt;
		}
		catch (sql::SQLException& e)
		{
			LOG_ERRORF(L"쿼리 실행 실패: %s", StringToWString(e.what()));
		}
	}

	/*
	   UPDATE, INSERT, DELETE 쿼리를 실행하고 영향을 받은 행 수를 출력하는 메서드
	   - query: 실행할 SQL 쿼리 (UPDATE, INSERT, DELETE 등)
	   - 성공 시 true, 실패 시 false 반환
	*/
	bool ExecuteUpdate(const wstring& query)
	{
		try
		{
			UpdateLastUsed();
			sql::Statement* stmt = _connection->createStatement();
			int affectedRows = stmt->executeUpdate(WStringToString(query));
			LOG_INFOF(L"영향받은 행 수 %d", affectedRows);
			delete stmt;
			return true;
		}
		catch (sql::SQLException& e)
		{
			LOG_ERRORF(L"쿼리 실행 실패: %s", StringToWString(e.what()));
			return false;
		}
	}

	/*
		사용자 정보를 데이터베이스에 추가하는 메서드
		- name: 사용자 이름
		- age: 사용자 나이
		- email: 사용자 이메일
	*/
	void InsertUser(const wstring& name, int age, const wstring& email)
	{
		try
		{
			UpdateLastUsed();
			string query = "INSERT INTO users (name, age, email) VALUES (?, ?, ?)";
			sql::PreparedStatement* pstmt = _connection->prepareStatement(query);
			pstmt->setString(1, WStringToString(name));
			pstmt->setInt(2, age);
			pstmt->setString(3, WStringToString(email));

			int result = pstmt->executeUpdate();

			LOG_INFOF(L"user add complete %s %d %s %d", name.c_str(), age, email.c_str(), result);
			delete pstmt;
		}
		catch (sql::SQLException& e)
		{
			LOG_ERRORF(L"쿼리 실행 실패: %s", StringToWString(e.what()));
		}
	}

	/*
		연결이 유효한지 확인하는 메서드
		- 성공적으로 연결되어 있고, 연결이 닫히지 않은 경우 true 반환
	*/
	bool IsConnected()
	{
		return _connection && !_connection->isClosed();
	}

	/*
		연결이 만료되었는지 확인하는 메서드
		- timeoutSeconds: 만료 시간 (초 단위)
		- 현재 시간과 마지막 사용 시간의 차이가 주어진 시간보다 크면 true 반환
		- 그렇지 않으면 false 반환
	*/
	bool IsExpired(int timeoutSeconds)
	{
		auto now = chrono::steady_clock::now();
		auto elapsed = chrono::duration_cast<chrono::seconds>(now - _lastUsedTime);
		return elapsed.count() > timeoutSeconds;
	}

	/*
		트랜잭션을 시작하는 메서드
		- 데이터베이스 연결이 유효한 경우 트랜잭션을 시작하고 auto-commit 모드를 false로 설정
		- 성공적으로 트랜잭션을 시작하면 true 반환 실패 시 false 반환
	*/
	bool BeginTransaction()
	{
		try
		{
			UpdateLastUsed();
			_connection->setAutoCommit(false);
			LOG_INFO(L"트랜잭션 시작");
			return true;
		}
		catch (sql::SQLException& e)
		{
			LOG_ERRORF(L"트랜잭션 시작 실패: %s", StringToWString(e.what()));
			return false;
		}
	}

	/*
		트랜잭션을 커밋하는 메서드
		- 현재 트랜잭션의 모든 변경 사항을 저장
		- auto-commit 모드를 true로 설정
		- 성공적으로 커밋하면 true 반환, 실패 시 false 반환
	*/
	bool Commit()
	{
		try
		{
			UpdateLastUsed();
			_connection->commit();
			_connection->setAutoCommit(true);
			LOG_INFO(L"트랜잭션 커밋 완료");
			return true;
		}
		catch (sql::SQLException& e)
		{
			LOG_ERRORF(L"커밋 실패: %s", StringToWString(e.what()));
			return false;
		}
	}

	/*
		트랜잭션 롤백
		현재 트랜잭션의 모든 변경사항을 취소
		성공 시 true 반환, 실패 시 false 반환
	*/
	bool Rollback()
	{
		try
		{
			UpdateLastUsed();
			_connection->rollback();
			_connection->setAutoCommit(true);
			LOG_INFO(L"트랜잭션 롤백 완료");
			return true;
		}
		catch (sql::SQLException& e)
		{
			LOG_ERRORF(L"롤백 실패: %s", StringToWString(e.what()));
			return false;
		}
	}

	/*
	   AutoCommit 모드 상태 확인
	   AutoCommit이 활성화되어 있으면 true
   */
	bool GetAutoCommit()
	{
		try
		{
			return _connection->getAutoCommit();
		}
		catch (sql::SQLException e)
		{
			LOG_ERRORF(L"AutoCommit 상태 확인 실패: %s", StringToWString(e.what()));
			return true;
		}
	}
private:
	/*
	  마지막으로 사용된 시간을 업데이트한다.
	  이 함수는 현재 시간으로 _lastUsedTime을 갱신한다.
	  실제로는 DB 연결 풀에서 사용될 때마다 호출되어야 한다.
	  예를 들어, 연결을 가져올 때마다 호출해야 함
	*/
	void UpdateLastUsed()
	{
		_lastUsedTime = chrono::steady_clock::now();
	}
};

/*
	MySQL 연결 풀을 관리하는 클래스
	여러 개의 MySQL 연결을 효율적으로 관리하여 성능을 향상시킵니다.
	연결의 생성/소멸 비용을 줄이고, 동시 접속자 수를 관리할 수 있습니다.
*/
class MySQLConnectionPool
{
private:
	bool _initialized;

	// 연결 풀 관리용 큐들
	queue<unique_ptr<MySQLConnection>> _availableConnections; // 사용 가능한 연결들
	queue<unique_ptr<MySQLConnection>> _usedConnections; // 사용 중인 연결들

	// 스레드 안전성을 위한 동기화 객체들
	mutex _poolMutex; // 풀 접근 동기화
	condition_variable _condition; // 연결 대기용 조건 변수

	// 데이터베이스 연결 정보
	wstring _host;
	wstring _user;
	wstring _password;
	wstring _database;

	// 풀 설정 값들
	int _maxPoolSize;        // 최대 연결 수
	int _minPoolSize;        // 최소 연결 수
	int _currentPoolSize;    // 현재 연결 수
	int _connectionTimeout;  // 연결 타임아웃 (초)

	// 정리 스레드 관련 변수
	bool _shutdownFlag; // 종료 플래그
	thread _cleanupThread; // 만료된 연결 정리 스레드	

	/*
		wstring을 UTF-8 string으로 변환
	*/
	string WStringToString(const wstring& wstr)
	{
		if (wstr.empty()) return {};

		int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
			nullptr, 0, nullptr, nullptr);
		string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
			&strTo[0], size_needed, nullptr, nullptr);
		return strTo;
	}

	/*
		UTF-8 string을 wstring으로 변환
	*/
	wstring StringToWString(const string& str)
	{
		if (str.empty()) return {};

		int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(),
			nullptr, 0);
		wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(),
			&wstrTo[0], size_needed);
		return wstrTo;
	}
public:
	MySQLConnectionPool()
		: _minPoolSize(0), _maxPoolSize(0), _currentPoolSize(0),
		_connectionTimeout(0), _shutdownFlag(false), _initialized(false)
	{
	}

	/*
	   연결 풀 생성자
	   host : 데이터베이스 호스트
	   user : 사용자명
	   password : 비밀번호
	   database : 데이터베이스명
	   minSize : 최소 연결 수 (기본값: 5)
	   maxSize : 최대 연결 수 (기본값: 20)
	   timeout : 연결 타임아웃 초 (기본값: 300초)
   */
	MySQLConnectionPool(const wstring& host, const wstring& user,
		const wstring& password, const wstring& database,
		int minSize = 5, int maxSize = 20, int timeout = 300)
		: _host(host), _user(user), _password(password), _database(database),
		_minPoolSize(minSize), _maxPoolSize(maxSize), _currentPoolSize(0),
		_connectionTimeout(timeout), _shutdownFlag(false)
	{

		// 최소 연결 수만큼 미리 생성
		InitializePool();

		// 정리 스레드 시작
		//_cleanupThread = thread(&MySQLConnectionPool::CleanupExpiredConnections, this);
	}

	// 초기화 메서드 추가
	bool Initialize(const wstring& host, const wstring& user,
		const wstring& password, const wstring& database,
		int minSize = 5, int maxSize = 20, int timeout = 300)
	{
		if (_initialized)
		{
			return false;
		}

		_host = host;
		_user = user;
		_password = password;
		_database = database;
		_minPoolSize = minSize;
		_maxPoolSize = maxSize;
		_connectionTimeout = timeout;
		_initialized = true;

		InitializePool();
		//_cleanupThread = thread(&MySQLConnectionPool::CleanupExpiredConnections, this);
		return true;
	}

	/*
		연결 풀 소멸자
		모든 연결을 정리하고 정리 스레드를 종료
	*/
	~MySQLConnectionPool()
	{
		_shutdownFlag = true;
		_condition.notify_all();

		if (_cleanupThread.joinable())
		{
			_cleanupThread.join();
		}

		// 모든 연결 정리
		lock_guard<mutex> lock(_poolMutex);
		while (!_availableConnections.empty())
		{
			_availableConnections.pop();
		}
		while (!_usedConnections.empty())
		{
			_usedConnections.pop();
		}
	}

	/*
		풀에서 사용 가능한 연결을 획득
		사용 가능한 연결이 없으면 새로 생성하거나 대기
		성공하면 사용 가능한 연결 객체, 실패하면 nullptr
	*/
	unique_ptr<MySQLConnection> GetConnection()
	{
		if (!_initialized) return nullptr;

		unique_lock<mutex> lock(_poolMutex);

		// 사용 가능한 연결이 없고 최대 크기에 도달하지 않았다면 새 연결 생성
		if (_availableConnections.empty() && _currentPoolSize < _maxPoolSize)
		{
			auto newConnection = CreateConnection();
			if (newConnection)
			{
				_currentPoolSize++;
				return newConnection;
			}
		}

		// 사용 가능한 연결이 있을 때까지 대기
		_condition.wait(lock, [this]
			{
				return !_availableConnections.empty() || _shutdownFlag;
			});

		if (_shutdownFlag)
		{
			return nullptr;
		}

		auto connection = move(_availableConnections.front());
		_availableConnections.pop();

		// 연결이 유효한지 확인
		if (!connection->IsConnected())
		{
			// 재연결 시도
			if (!connection->Connect(_host, _user, _password, _database))
			{
				// 재연결 실패시 새 연결 생성
				connection = CreateConnection();
				if (!connection)
				{
					return nullptr;
				}
			}
		}

		return connection;
	}

	/*
		사용 완료된 연결을 풀에 반환
		connection : 반환할 연결 객체
	*/
	void ReturnConnection(unique_ptr<MySQLConnection> connection)
	{
		if (!connection) return;

		lock_guard<mutex> lock(_poolMutex);

		if (connection->IsConnected())
		{
			_availableConnections.push(move(connection));
			_condition.notify_one(); // 대기 중인 스레드에 알림
		}
		else
		{
			// 연결이 끊어진 경우 풀 사이즈 감소
			_currentPoolSize--;
		}
	}

	/*
		현재 연결 풀의 상태 정보를 출력
	*/
	void PrintPoolStatus()
	{
		lock_guard<mutex> lock(_poolMutex);
		LOG_DEBUG(L"=== 연결 풀 상태 ===");
		LOG_DEBUGF(L"전체 연결 수: %d", _currentPoolSize);
		LOG_DEBUGF(L"사용 가능한 연결 수: %d", (int)_availableConnections.size());
		LOG_DEBUGF(L"사용 중인 연결 수: %d", _currentPoolSize - (int)_availableConnections.size());
		LOG_DEBUGF(L"최대 연결 수: ", _maxPoolSize);
		LOG_DEBUGF(L"최소 연결 수: ", _minPoolSize);
	}

private:
	/*
		연결 풀을 최소 크기로 초기화
		서버 시작 시 미리 연결들을 생성하여 초기 응답 시간을 단축
	*/
	void InitializePool()
	{
		for (int i = 0; i < _minPoolSize; ++i)
		{
			auto connection = CreateConnection();
			if (connection)
			{
				_availableConnections.push(move(connection));
				_currentPoolSize++;
			}
		}

		LOG_DEBUGF(L"DBConnection Pool Init Complete : %d", _currentPoolSize);
	}

	/*
		새로운 MySQL 연결을 생성
		성공 시 새 연결 객체, 실패 시 nullptr
	*/
	unique_ptr<MySQLConnection> CreateConnection()
	{
		auto connection = make_unique<MySQLConnection>();
		if (connection->Connect(_host, _user, _password, _database))
		{
			return connection;
		}
		return nullptr;
	}

	/*
		만료된 연결들을 주기적으로 정리하는 백그라운드 스레드
		1문마다 실행되어 타임아웃된 연결을 제거하고 최소 연결 수를 유지한다.
	*/
	void CleanupExpiredConnections()
	{
		while (!_shutdownFlag)
		{
			this_thread::sleep_for(chrono::seconds(60)); // 1분마다 정리

			lock_guard<mutex> lock(_poolMutex);

			queue<unique_ptr<MySQLConnection>> validConnections;

			// 만료되지 않은 유효한 연결들만 보존
			while (!_availableConnections.empty())
			{
				auto connection = move(_availableConnections.front());
				_availableConnections.pop();

				if (connection->IsConnected() && !connection->IsExpired(_connectionTimeout))
				{
					validConnections.push(move(connection));
				}
				else
				{
					_currentPoolSize--;
				}
			}

			_availableConnections = move(validConnections);

			// 최소 연결 수 유지
			while (_currentPoolSize < _minPoolSize)
			{
				auto newConnection = CreateConnection();
				if (newConnection)
				{
					_availableConnections.push(move(newConnection));
					_currentPoolSize++;
				}
				else
				{
					break;
				}
			}
		}
	}
};

/*
	RAII 패턴을 적용한 연결 래퍼 클래스

	연결 풀에서 가져온 연결을 자동으로 관리한다.
	객체가 소멸될 때 자동으로 연결을 풀에 반환하여
	연결 누수를 방지한다.
*/
class PooledConnection
{
private:
	unique_ptr<MySQLConnection> _connection; // 관리하는 연결 객체
	MySQLConnectionPool* _pool; // 연결이 속한 풀의 포인터
public:
	/*
		생성자
		conn : 관리할 연결 객체
		p : 연결이 속한 풀
	*/
	PooledConnection(unique_ptr<MySQLConnection> conn, MySQLConnectionPool* p)
		: _connection(move(conn)), _pool(p)
	{
	}

	/*
		소멸자 - 연결을 자동으로 풀에 반환
	*/
	~PooledConnection()
	{
		if (_connection && _pool)
		{
			_pool->ReturnConnection(move(_connection));
		}
	}

	/*
		포인터 연산자 오버로딩
		MySQLConnection의 메서드에 직접 접근 가능
	*/
	MySQLConnection* operator->()
	{
		return _connection.get();
	}

	/*
		참조 연산자 오버로딩
		객체에 직접 접근 가능
	*/
	MySQLConnection& operator*()
	{
		return *_connection;
	}

	/*
		연결 유효성 확인
		연결이 유효하면 true 반환
	*/
	bool IsValid() const
	{
		return _connection != nullptr;
	}
};

/*
	자동 트랜잭션 관리를 위한 RAII 클래스

	트랜잭션을 자동으로 시작하고, 명시적으로 커밋하지 않으면
	소멸 시 자동으로 롤백하여 데이터 일관성을 보장한다.
*/
class Transaction
{
private:
	MySQLConnection* _connection; // 트랜잭션을 수행할 연결
	bool _committed; // 커밋 여부
	bool _rolledBack; // 롤백 여부

public:
	/*
		생성자
		자동으로 트랜잭션을 시작
		conn : 트랜잭션을 수행할 연결
	*/
	Transaction(MySQLConnection* conn) : _connection(conn), _committed(false), _rolledBack(false)
	{
		if (_connection)
		{
			_connection->BeginTransaction();
		}
	}

	/*
		소멸자
		명시적으로 커밋하지 않았다면 자동으로 롤백
	*/
	~Transaction()
	{
		if (_connection && !_committed && !_rolledBack)
		{
			// 명시적으로 커밋하지 않았다면 자동 롤백
			_connection->Rollback();
		}
	}

	/*
		트랜잭션 커밋
		성공 시 true
	*/
	bool Commit()
	{
		if (_connection && !_committed && !_rolledBack)
		{
			_committed = _connection->Commit();
			return _committed;
		}
		return false;
	}

	/*
		트랜잭션 롤백
		성공 시 true
	*/
	bool Rollback()
	{
		if (_connection && !_committed && !_rolledBack)
		{
			_rolledBack = _connection->Rollback();
			return _rolledBack;
		}
		return false;
	}

	/*
		커밋 상태 확인
		커밋되었으면 true
	*/
	bool IsCommitted() const
	{
		return _committed;
	}

	/*
		롤백 상태 확인
		롤백되었으면 true
	*/
	bool IsRolledBack() const
	{
		return _rolledBack;
	}
};