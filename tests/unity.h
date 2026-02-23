#ifndef UNITY_H
#define UNITY_H

#include <setjmp.h>

void setUp(void);
void tearDown(void);

void UnityBegin(const char *filename);
int UnityEnd(void);
void UnityConcludeTest(void);

void UnityPrint(const char *str);
void UnityPrintNumber(int number);
void UnityFail(const char *msg, int line);
void UnityIgnore(const char *msg, int line);

#define TEST_PROTECT() (setjmp(UnityAbort) == 0)

#define TEST_ASSERT(condition) \
	do { \
		if (!(condition)) { \
			UnityFail(#condition, __LINE__); \
			return; \
		} \
	} while (0)

#define TEST_ASSERT_TRUE(condition) TEST_ASSERT(condition)
#define TEST_ASSERT_FALSE(condition) TEST_ASSERT(!(condition))

#define TEST_ASSERT_EQUAL_INT(expected, actual) \
	do { \
		if ((expected) != (actual)) { \
			UnityFail("Expected " #expected " but got " #actual, __LINE__); \
			return; \
		} \
	} while (0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) \
	do { \
		if (strcmp((expected), (actual)) != 0) { \
			UnityFail("String mismatch", __LINE__); \
			return; \
		} \
	} while (0)

#define TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, len) \
	do { \
		if (strncmp((expected), (actual), (len)) != 0) { \
			UnityFail("String (len) mismatch", __LINE__); \
			return; \
		} \
	} while (0)

#define TEST_ASSERT_EQUAL_FLOAT(expected, actual) \
	do { \
		float _exp = (expected); \
		float _act = (actual); \
		if (_exp != _act && (_exp - _act > 0.0001f || _act - _exp > 0.0001f)) { \
			UnityFail("Float mismatch", __LINE__); \
			return; \
		} \
	} while (0)

#define TEST_ASSERT_EQUAL_DOUBLE(expected, actual) \
	do { \
		double _exp = (expected); \
		double _act = (actual); \
		if (_exp != _act && (_exp - _act > 0.0001 || _act - _exp > 0.0001)) { \
			UnityFail("Double mismatch", __LINE__); \
			return; \
		} \
	} while (0)

#define TEST_ASSERT_EQUAL_PTR(expected, actual) \
	do { \
		if ((expected) != (actual)) { \
			UnityFail("Pointer mismatch", __LINE__); \
			return; \
		} \
	} while (0)

#define TEST_ASSERT_NULL(ptr) TEST_ASSERT((ptr) == NULL)
#define TEST_ASSERT_NOT_NULL(ptr) TEST_ASSERT((ptr) != NULL)

#define TEST_ASSERT_EQUAL_SIZE(expected, actual) \
	do { \
		if ((size_t)(expected) != (size_t)(actual)) { \
			UnityFail("Size mismatch", __LINE__); \
			return; \
		} \
	} while (0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len) \
	do { \
		if (memcmp((expected), (actual), (len)) != 0) { \
			UnityFail("Memory mismatch", __LINE__); \
			return; \
		} \
	} while (0)

#define RUN_TEST(func) \
	do { \
		Unity.CurrentTest = #func; \
		Unity.TestFile = __FILE__; \
		Unity.CurrentTestLine = __LINE__; \
		if (TEST_PROTECT()) { \
			setUp(); \
			func(); \
		} \
		UnityConcludeTest(); \
	} while (0)

extern jmp_buf UnityAbort;

typedef struct {
	const char *TestFile;
	const char *CurrentTest;
	int CurrentTestLine;
	int TestsRun;
	int TestsPassed;
	int TestsFailed;
	int TestsIgnored;
} UnityType;

extern UnityType Unity;

#endif
