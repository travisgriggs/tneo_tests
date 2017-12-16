#include "unity.h"

void testEventGroup(void);
void testSemaphore(void);
void testPool(void);
void testQueue(void);
void testTask(void);
void testMutex(void);
void testTimer(void);

void allTestsRun(void *_) {
	UnityBegin("");
	testEventGroup();
	testSemaphore();
	testPool();
	testQueue();
	testTask();
	testMutex();
	testTimer();
	UnityEnd();
}
