#ifndef TESTSUPPORT_H
#define TESTSUPPORT_H

#include "tn.h"
#include "unity.h"
#include <stdbool.h>

// Get the most recent return code from the last dispatched command
// This may not be valid if the task has not yet returned from a dispatch
// of a command that blocked. isPending should be asserted true before the
// rc is checked usually.
enum TN_RCode RC(int n);
// Task structure associated with the testlet
struct TN_Task* task(int n);
// isPending is true when the testlet is waiting for another command to dispatch
// As soon is it receives a command, this is set to false, until it completes the command.
bool isPending(int n);
// The original priority that the testlet was created at. Does not necessarily reflect the
// current priority of the testlet.
int priority(int n);

// return the nth associated object
struct TN_Mutex* mutex(int n);
struct TN_Sem* semaphore(int n);
struct TN_Timer* timer(int n);
struct TN_DQueue* queue(int n);
struct TN_EventGrp* eventgroup(int n);
struct TN_FMem* pool(int n);

enum CommandType {
	_EventWait,
	_EventWaitPoll,
	_EventModify,
	_EventDelete,
	_MutexLock,
	_MutexLockPoll,
	_MutexUnlock,
	_MutexDelete,
	_PoolGet,
	_PoolGetPoll,
	_PoolRelease,
	_PoolDelete,
	_QueueReceivePoll,
	_QueueSendPoll,
	_QueueSend,
	_QueueReceive,
	_QueueDelete,
	_SemaphoreWait,
	_SemaphoreWaitPoll,
	_SemaphoreSignal,
	_SemaphoreDelete,
	_ReleaseTask,
	_TaskActivate,
	_TaskSuspend,
	_TaskSleep,
	_TaskWakeup,
	_TaskReturn,
	_TaskExit
};

typedef union {
	struct {
		uint32_t type:5;        // CommandType
		uint32_t index:3;       // Which object to act on if applicable, zero by default
		uint32_t isInterrupt:1; // Used to determine whether to try the i variant of a TNeo api if one exists or not
		uint32_t data:7;        // Used by QueueSend command to contain value to send
		uint32_t mode:8;        // receiveTarget is null or not for QueueTests, patternmode for EventGroupTests
		uint32_t hasTimeout:1;  // If applicable, timeout is set TN_TIMEOUT_INFINITE if this is false (default), otherwise the .timeout field is used
		uint32_t timeout:7;     // The timeout if applicable AND if .hasTimeout is set to true
	};
	uint32_t bits;
} Command;

void command(int which, Command command);
void interrupt(Command command);

// The last RC value received by an interrupt command
extern volatile enum TN_RCode RCInterrupt;

// The void* argument of tn_timer_start is stored in this global when the timer fires
extern uint32_t TimerCallbackValue;
void timerCallback(struct TN_Timer *timer, void *arg);
// Global value for last value received through tn_queue_receive
extern uint32_t QueueReceivedValue;
// Global value for the last value fetched through tn_fmem_get
extern void* PoolValuePointer;
// Global memory backing the single pool
extern uint32_t PoolMemory[2];

// special asserts so that we get user friendly messages
const char* rcToString(enum TN_RCode rc);  // use the following #define instead though
#define TEST_ASSERT_EQUAL_RC(expected, actual)	TEST_ASSERT_EQUAL_STRING_MESSAGE(rcToString(expected), rcToString(actual), "!RC")
const char* reasonToString(enum TN_WaitReason reason);  // use the following #define instead though
#define TEST_ASSERT_EQUAL_REASON(expected, actual)	TEST_ASSERT_EQUAL_STRING_MESSAGE(reasonToString(expected), reasonToString(actual), "!Reason")

#endif // TESTSUPPORT_H
