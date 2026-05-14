#pragma once

#define OUT

#define USE_LOCK                  std::shared_mutex _rwLock;
#define READ_LOCK                 std::shared_lock<std::shared_mutex> readLock(_rwLock)
#define WRITE_LOCK                std::unique_lock<std::shared_mutex> writeLock(_rwLock)

#define CRASH(cause)				\
do {								\
	std::cerr << cause << std::endl;\
	__debugbreak();					\
	__analysis_assume(false);		\
} while(0)

#define ASSERT_CRASH(expr)		\
do {							\
	if(!(expr))					\
	{							\
		CRASH("ASSERT_CRASH");	\
		__analysis_assume(expr);\
	}							\
} while(0)
