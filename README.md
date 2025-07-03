# 고성능 채팅 서버 (High-Performance Chat Server)

📋 개요
ChattingServer는 IOCP(I/O Completion Port)를 활용한 고성능 비동기 네트워크 채팅 서버입니다. 대용량 동시 접속을 지원하며, 효율적인 메모리 관리와 멀티스레드 아키텍처를 통해 최적화된 성능을 제공합니다.

## 🚀 주요 특징

### 네트워크 아키텍처
- **IOCP (I/O Completion Port)** 기반 비동기 네트워크 처리
- **멀티스레드 워커 풀**: CPU 코어 수에 맞춰 자동으로 워커 스레드 생성
- **링 버퍼**: 효율적인 수신 버퍼 관리

### 성능 및 확장성
- **동시 접속자 지원**: `SERVER_SESSION_MAX` 설정으로 최대 동시 접속자 수 제어
- **패킷 풀링**: 메모리 할당/해제 오버헤드 최소화
- **TPS 모니터링**: 실시간 Accept/Send/Recv TPS 추적
- **배치 전송**: 최대 `SESSION_SEND_PACKET_MAX`개 패킷 일괄 전송

### 보안 및 안정성
- **IP 국가 코드 검증**: 한국(KR) 및 로컬호스트만 접속 허용
- **패킷 검증**: 커스텀 패킷 코드(52) 검증으로 비정상 패킷 차단
- **세션 관리**: Reference counting과 CAS 연산으로 안전한 세션 해제
- **패킷 크기 제한**: 비정상적으로 큰 패킷 차단

### 멀티스레드 처리
- **Job Queue 시스템**: 패킷 처리를 별도 스레드에서 비동기 처리
- **업데이트 스레드 풀**: 5개의 업데이트 스레드로 패킷 처리 분산
- **이벤트 기반 스케줄링**: 패킷 도착 시에만 처리 스레드 활성화
- **Lock-Free 구조**: 성능 최적화를 위한 Interlocked 연산 활용

## 🏗️ 아키텍처 구성

```
[클라이언트] → [Accept Thread] → [IOCP Worker Threads] → [Job Queue] → [Update Threads]
```

1. **Accept Thread**: 클라이언트 연결 수락
2. **IOCP Worker Threads**: 네트워크 I/O 처리 (CPU 코어 수만큼)
3. **Job Queue**: 패킷 처리 작업 대기열
4. **Update Threads**: 실제 패킷 로직 처리 (5개 스레드)

## 📊 성능 모니터링

- **Accept TPS**: 초당 연결 수락 횟수
- **Send/Recv Packet TPS**: 초당 송수신 패킷 수
- **Update TPS**: 초당 패킷 처리 횟수
- **세션 카운트**: 현재 활성 세션 수

## 🔧 주요 기능

### 채팅 브로드캐스팅
- 한 클라이언트가 보낸 메시지를 모든 연결된 클라이언트에게 전송
- 메시지 길이 가변 지원
- 패킷 타입 기반 프로토콜 처리

### 세션 관리
- 고유 세션 ID 생성 (상위 32비트: 증가값, 하위 32비트: 인덱스)
- 안전한 세션 검색 및 해제
- 메모리 재사용을 위한 세션 인덱스 풀

### 패킷 처리
- 커스텀 패킷 인코딩/디코딩
- 헤더 검증 및 무결성 체크
- 패킷 객체 풀링으로 성능 최적화

## 🛠️ 기술 스택

- **언어**: C++
- **플랫폼**: Windows
- **네트워크**: Winsock2, IOCP
- **스레딩**: Windows Threading API
- **메모리 관리**: 커스텀 오브젝트 풀

## 📋 요구사항

- Windows 10/11 또는 Windows Server
- Visual Studio 2019 이상
- Windows SDK

## ⚙️ 설정 가능한 옵션

- `SERVER_SESSION_MAX`: 최대 동시 접속자 수
- `SESSION_SEND_PACKET_MAX`: 배치 전송 최대 패킷 수
- `PACKET_BUFFER_DEFAULT_SIZE`: 패킷 버퍼 기본 크기
- 업데이트 스레드 수 (기본 5개)

## 📈 성능 특징

- **비동기 I/O**: 블로킹 없는 네트워크 처리
- **메모리 풀링**: 동적 할당 오버헤드 제거
- **CPU 스케일링**: 멀티코어 활용 최적화
