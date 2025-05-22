#pragma once

#define CRASH(cause)						\
{											\
	int* Crash = nullptr;					\
	__analysis_assume(Crash != nullptr);	\
	*Crash = 0xDEADBEEF;					\
}											\
