#pragma once
#include <array>
#include <cstdint>

class Xoshiro256
{
private:
	std::array<uint64_t, 4> s;

	// seed 값 초기화
	uint64_t splitmix64(uint64_t& seed) {
		uint64_t result = seed;
		result += 0x9e3779b97f4a7c15;
		result = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9;
		result = (result ^ (result >> 27)) * 0x94d049bb133111eb;
		result = result ^ (result >> 31);
		seed = result;
		return result;
	}

	uint64_t Rotl(const uint64_t x, int k)
	{
		return (x << k) | (x >> (64 - k));
	}

	// 생성자 
	Xoshiro256()
	{
		uint64_t seed = static_cast<uint64_t>(time(NULL));
		for (auto& val : s) {
			val = splitmix64(seed);
		}
	}
		
	~Xoshiro256() = default;

	// 복사 생성자, 대입 연산자 삭제
	Xoshiro256(const Xoshiro256&) = delete;
	Xoshiro256& operator=(const Xoshiro256&) = delete;
public:	
	// 싱글턴 객체 반환
	static Xoshiro256& GetInstance() {
		static Xoshiro256 instance;
		return instance;
	}

	// 랜덤 값 반환
	uint64_t Next()
	{
		const uint64_t result = Rotl(s[0] + s[3], 23) + s[0];
		const uint64_t t = s[1] << 17;

		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];

		s[2] ^= t;

		s[3] = Rotl(s[3], 45);

		return result;
	}

	// 범위 사이 랜덤 값 반환
	uint64_t RandNumber(uint64_t min, uint64_t max)
	{
		return min + (Next() % (max - min + 1));
	}

	void Jump(void)
	{
		static const uint64_t jump[] = {
			0x180ec6d33cfd0abaULL,
			0xd5a61266f0c9392cULL,
			0xa9582618e03fc9aaULL,
			0x39abdc4529b1661cULL
		};

		uint64_t s0 = 0;
		uint64_t s1 = 0;
		uint64_t s2 = 0;
		uint64_t s3 = 0;

		for (auto jump_val : jump) {
			for (int b = 0; b < 64; ++b) {
				if (jump_val & (1ULL << b)) {
					s0 ^= s[0];
					s1 ^= s[1];
					s2 ^= s[2];
					s3 ^= s[3];
				}
				Next();
			}
		}

		s[0] = s0;
		s[1] = s1;
		s[2] = s2;
		s[3] = s3;
	}
};