#include "unity.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

UnityType Unity;
jmp_buf UnityAbort;

__attribute__((weak)) void setUp(void) {}
__attribute__((weak)) void tearDown(void) {}

void UnityBegin(const char *filename)
{
	Unity.TestFile = filename;
	Unity.TestsRun = 0;
	Unity.TestsPassed = 0;
	Unity.TestsFailed = 0;
	Unity.TestsIgnored = 0;
	printf("\n-----------------------\n");
	printf("%s\n", filename);
	printf("-----------------------\n");
}

int UnityEnd(void)
{
	printf("-----------------------\n");
	printf("%d Tests %d Passed %d Failed %d Ignored\n",
		Unity.TestsRun, Unity.TestsPassed, Unity.TestsFailed, Unity.TestsIgnored);
	printf("-----------------------\n");
	return Unity.TestsFailed;
}

void UnityPrint(const char *str)
{
	printf("%s", str);
}

void UnityPrintNumber(int number)
{
	printf("%d", number);
}

void UnityFail(const char *msg, int line)
{
	printf("%s:%d:FAIL: %s\n", Unity.TestFile, line, msg);
	Unity.TestsFailed++;
	longjmp(UnityAbort, 1);
}

void UnityIgnore(const char *msg, int line)
{
	printf("%s:%d:IGNORE: %s\n", Unity.TestFile, line, msg);
	Unity.TestsIgnored++;
}

void UnityConcludeTest(void)
{
	if (!Unity.TestsFailed || Unity.TestsRun == Unity.TestsPassed + Unity.TestsIgnored) {
		printf("%s:PASSED\n", Unity.CurrentTest);
		Unity.TestsPassed++;
	}
	tearDown();
	Unity.TestsRun++;
}
