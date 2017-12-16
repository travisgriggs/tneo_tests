#include "testSupport.h"

enum {
	_Inherit1 = 0,
	_Inherit2 = 1,
	_Ceiling1 = 2,
	_Ceiling2 = 3
};

static void testCreateInheritOK(void) {
	struct TN_Mutex mutex;
	enum TN_RCode rc = tn_mutex_create(&mutex, TN_MUTEX_PROT_INHERIT, -1);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testCreateCeilingNegative(void) {
	struct TN_Mutex mutex;
	enum TN_RCode rc = tn_mutex_create(&mutex, TN_MUTEX_PROT_CEILING, -1);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testCreateCeilingZero(void) {
	struct TN_Mutex mutex;
	enum TN_RCode rc = tn_mutex_create(&mutex, TN_MUTEX_PROT_CEILING, 0);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testCreateCeilingOK(void) {
	struct TN_Mutex mutex;
	enum TN_RCode rc = tn_mutex_create(&mutex, TN_MUTEX_PROT_CEILING, 10);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testCreateCeilingAtMax(void) {
	struct TN_Mutex mutex;
	enum TN_RCode rc = tn_mutex_create(&mutex, TN_MUTEX_PROT_CEILING, TN_PRIORITIES_CNT - 2);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testCreateCeilingAboveMax(void) {
	struct TN_Mutex mutex;
	enum TN_RCode rc = tn_mutex_create(&mutex, TN_MUTEX_PROT_CEILING, TN_PRIORITIES_CNT - 1);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testDelete(void) {
	enum TN_RCode rc = tn_mutex_delete(mutex(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testDoubleDelete(void) {
	tn_mutex_delete(mutex(0));
	enum TN_RCode rc = tn_mutex_delete(mutex(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_INVALID_OBJ, rc);
}

static void testRecursiveInherit(void) {
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	command(1, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL(3, mutex(_Inherit1)->cnt);
	command(0, (Command){.type=_MutexUnlock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL(2, mutex(_Inherit1)->cnt);
	command(0, (Command){.type=_MutexUnlock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL(1, mutex(_Inherit1)->cnt);
	command(0, (Command){.type=_MutexUnlock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL(1, mutex(_Inherit1)->cnt);
}

static void testRecursiveCeiling(void) {
	command(0, (Command){.type=_MutexLock, .index=_Ceiling1});
	command(0, (Command){.type=_MutexLock, .index=_Ceiling1});
	command(0, (Command){.type=_MutexLock, .index=_Ceiling1});
	command(1, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL(3, mutex(_Ceiling1)->cnt);
	command(0, (Command){.type=_MutexUnlock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL(2, mutex(_Ceiling1)->cnt);
	command(0, (Command){.type=_MutexUnlock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL(1, mutex(_Ceiling1)->cnt);
	command(0, (Command){.type=_MutexUnlock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL(1, mutex(_Ceiling1)->cnt);
}

static void testWrongUnlockHolderCeiling(void) {
	command(0, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(1, (Command){.type=_MutexUnlock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_ILLEGAL_USE, RC(1));
}

static void testWrongDeleteContex(void) {
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(1, (Command){.type=_MutexDelete, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_ILLEGAL_USE, RC(1));
}

static void testWrongUnlockHolderInherit(void) {
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(1, (Command){.type=_MutexUnlock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_ILLEGAL_USE, RC(1));
}

static bool DeadlockOccured = false;
void deadlockDetected(TN_BOOL active, struct TN_Mutex *mutex, struct TN_Task *task) {
	DeadlockOccured = true;
}

static void testDeadlockCeiling(void) {
	DeadlockOccured = false;
	command(0, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(1, (Command){.type=_MutexLock, .index=_Ceiling2});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	command(0, (Command){.type=_MutexLock, .index=_Ceiling2});
	TEST_ASSERT_FALSE(DeadlockOccured);
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(0)->task_wait_reason);
	command(1, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_TRUE(DeadlockOccured);
}

static void testDeadlockInherit(void) {
	DeadlockOccured = false;
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(1, (Command){.type=_MutexLock, .index=_Inherit2});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	command(0, (Command){.type=_MutexLock, .index=_Inherit2});
	TEST_ASSERT_FALSE(DeadlockOccured);
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(0)->task_wait_reason);
	command(1, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_TRUE(DeadlockOccured);
}

static void testCeilingLockFromTemporaryPriorityInheritance(void) {
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	command(4, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_EQUAL(task(4)->priority, task(0)->priority);
	command(0, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL(task(4)->priority, task(0)->priority);
}

static void testCeilingRaisesPriority(void) {
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	command(0, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_EQUAL(mutex(_Ceiling1)->ceil_priority, task(0)->priority);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
	command(1, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority); // shouldn't raise yet
	command(0, (Command){.type=_MutexUnlock, .index=_Ceiling1});
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL(mutex(_Ceiling1)->ceil_priority, task(1)->priority);
}

static void testCeilingLockFromSamePriority(void) {
	TEST_ASSERT_EQUAL(mutex(_Ceiling1)->ceil_priority, task(2)->priority);
	command(2, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
}

static void testCeilingLockFromHigherPriority(void) {
	command(3, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_TRUE(isPending(3));
	TEST_ASSERT_EQUAL_RC(TN_RC_ILLEGAL_USE, RC(3));
}

static void testCeilingFallsToPreviousMutex(void) {
	TEST_ASSERT_LESS_THAN(task(0)->priority, mutex(_Ceiling1)->ceil_priority);
	TEST_ASSERT_LESS_THAN(mutex(_Ceiling1)->ceil_priority, mutex(_Ceiling2)->ceil_priority);
	command(0, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_EQUAL(mutex(_Ceiling1)->ceil_priority, task(0)->priority);
	command(0, (Command){.type=_MutexLock, .index=_Ceiling2});
	TEST_ASSERT_EQUAL(mutex(_Ceiling2)->ceil_priority, task(0)->priority);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(0, (Command){.type=_MutexUnlock, .index=_Ceiling2});
	TEST_ASSERT_EQUAL(mutex(_Ceiling1)->ceil_priority, task(0)->priority);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
}

static void testCeilingStaysAtHigherPreviousMutex(void) {
	TEST_ASSERT_LESS_THAN(task(0)->priority, mutex(_Ceiling1)->ceil_priority);
	TEST_ASSERT_LESS_THAN(mutex(_Ceiling1)->ceil_priority, mutex(_Ceiling2)->ceil_priority);
	command(0, (Command){.type=_MutexLock, .index=_Ceiling2});
	TEST_ASSERT_EQUAL(mutex(_Ceiling2)->ceil_priority, task(0)->priority);
	command(0, (Command){.type=_MutexLock, .index=_Ceiling1});
	TEST_ASSERT_EQUAL(mutex(_Ceiling2)->ceil_priority, task(0)->priority);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(0, (Command){.type=_MutexUnlock, .index=_Ceiling2});
	TEST_ASSERT_EQUAL(mutex(_Ceiling1)->ceil_priority, task(0)->priority);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
}

static void testDuelingInheritPriorityEscalation(void) {
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	command(0, (Command){.type=_MutexLock, .index=_Inherit2});
	command(2, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_EQUAL(task(2)->priority, task(0)->priority);
	command(3, (Command){.type=_MutexLock, .index=_Inherit2});
	TEST_ASSERT_EQUAL(task(3)->priority, task(0)->priority);
	command(1, (Command){.type=_MutexLock, .index=_Inherit2});
	TEST_ASSERT_EQUAL(task(3)->priority, task(0)->priority);
	command(4, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_EQUAL(task(4)->priority, task(0)->priority);
}

static void testRecursingInheritPriorityEscalation(void) {
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	command(2, (Command){.type=_MutexLock, .index=_Inherit2});
	command(2, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_EQUAL(task(2)->priority, task(0)->priority);
	TEST_ASSERT_FALSE(isPending(2));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(2)->task_wait_reason);
	command(4, (Command){.type=_MutexLock, .index=_Inherit2});
	TEST_ASSERT_EQUAL(task(4)->priority, task(0)->priority);
	TEST_ASSERT_EQUAL(task(4)->priority, task(0)->priority);
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(4)->task_wait_reason);
}

static void testUnlockOrderInherit(void) {
	command(2, (Command){.type=_MutexLock, .index=_Inherit1});
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	command(4, (Command){.type=_MutexLock, .index=_Inherit1});
	command(1, (Command){.type=_MutexLock, .index=_Inherit1});
	command(3, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(0)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(1)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(3)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(4)->task_wait_reason);
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(4), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
	command(2, (Command){.type=_MutexUnlock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(1)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(3)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(4)->task_wait_reason);
	TEST_ASSERT_EQUAL(priority(4), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
	command(0, (Command){.type=_MutexUnlock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(4));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(4));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(1)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(3)->task_wait_reason);
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
	command(4, (Command){.type=_MutexUnlock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(4));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(4));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(3)->task_wait_reason);
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
	command(1, (Command){.type=_MutexUnlock, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_TRUE(isPending(3));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(3));
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
}

static void testUnlockOrderCeiling(void) {
	TEST_ASSERT_EQUAL(priority(3), mutex(_Ceiling2)->ceil_priority);
	command(2, (Command){.type=_MutexLock, .index=_Ceiling2});
	command(0, (Command){.type=_MutexLock, .index=_Ceiling2});
	command(3, (Command){.type=_MutexLock, .index=_Ceiling2});
	command(1, (Command){.type=_MutexLock, .index=_Ceiling2});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(0)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(1)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(3)->task_wait_reason);
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
	command(2, (Command){.type=_MutexUnlock, .index=_Ceiling2});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(1)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(3)->task_wait_reason);
	TEST_ASSERT_EQUAL(priority(3), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
	command(0, (Command){.type=_MutexUnlock, .index=_Ceiling2});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(3));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(3));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
	command(3, (Command){.type=_MutexUnlock, .index=_Ceiling2});
	TEST_ASSERT_TRUE(isPending(3));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(3));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
}

static void testLockPoll(void) {
	command(2, (Command){.type=_MutexLockPoll, .index=_Inherit2});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_EQUAL_PTR(task(2), mutex(_Inherit2)->holder);
	command(1, (Command){.type=_MutexLockPoll, .index=_Inherit2});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(1));
}

static void testLockTimeout(void) {
	command(2, (Command){.type=_MutexLock, .index=_Ceiling1, .hasTimeout=true, .timeout=50});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_EQUAL_PTR(task(2), mutex(_Ceiling1)->holder);
	command(1, (Command){.type=_MutexLock, .index=_Ceiling1, .hasTimeout=true, .timeout=50});
	tn_task_sleep(40);
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(1)->task_wait_reason);
	tn_task_sleep(20);
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(1));
}

static void testInheritanceDropsAfterTimeout(void) {
	command(1, (Command){.type=_MutexLock, .index=_Inherit1});
	command(3, (Command){.type=_MutexLock, .index=_Inherit1, .hasTimeout=true, .timeout=50});
	tn_task_sleep(40);
	TEST_ASSERT_EQUAL(priority(3), task(1)->priority);
	tn_task_sleep(20);
	TEST_ASSERT_TRUE(isPending(3));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(3));
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
}

static void testDeleteFromInterruptContex(void) {
	interrupt((Command){.type=_MutexDelete});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testLockFromInterruptContex(void) {
	interrupt((Command){.type=_MutexLock});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testUnlockFromInterruptContex(void) {
	interrupt((Command){.type=_MutexUnlock});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testLockPollingFromInterruptContex(void) {
	interrupt((Command){.type=_MutexLockPoll});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testDeleteReleasesTasks(void) {
	command(2, (Command){.type=_MutexLock, .index=_Inherit1});
	command(4, (Command){.type=_MutexLock, .index=_Inherit1});
	command(0, (Command){.type=_MutexLock, .index=_Inherit1});
	command(1, (Command){.type=_MutexLock, .index=_Inherit1});
	command(3, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(0)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(1)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(3)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(4)->task_wait_reason);
	command(2, (Command){.type=_MutexDelete, .index=_Inherit1});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_DELETED, RC(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_DELETED, RC(1));
	TEST_ASSERT_TRUE(isPending(3));
	TEST_ASSERT_EQUAL_RC(TN_RC_DELETED, RC(3));
	TEST_ASSERT_TRUE(isPending(4));
	TEST_ASSERT_EQUAL_RC(TN_RC_DELETED, RC(4));
	TEST_ASSERT_EQUAL(priority(0), task(0)->priority);
	TEST_ASSERT_EQUAL(priority(1), task(1)->priority);
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
	TEST_ASSERT_EQUAL(priority(3), task(3)->priority);
	TEST_ASSERT_EQUAL(priority(4), task(4)->priority);
}

static void testTaskTerminateReleases(void) {
	command(3, (Command){.type=_MutexLock, .index=_Ceiling2});
	command(2, (Command){.type=_MutexLock, .index=_Ceiling2});
	TEST_ASSERT_FALSE(isPending(2));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(2)->task_wait_reason);
	tn_task_terminate(task(3));
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_EQUAL_PTR(task(2), mutex(_Ceiling2)->holder);
}

static void testTaskExitReleases(void) {
	command(3, (Command){.type=_MutexLock, .index=_Ceiling2});
	command(2, (Command){.type=_MutexLock, .index=_Ceiling2});
	TEST_ASSERT_FALSE(isPending(2));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(2)->task_wait_reason);
	command(3, (Command){.type=_TaskExit});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_EQUAL_PTR(task(2), mutex(_Ceiling2)->holder);
}

static void testTaskReturnReleases(void) {
	command(3, (Command){.type=_MutexLock, .index=_Ceiling2});
	command(2, (Command){.type=_MutexLock, .index=_Ceiling2});
	TEST_ASSERT_FALSE(isPending(2));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_C, task(2)->task_wait_reason);
	command(3, (Command){.type=_TaskReturn});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_EQUAL_PTR(task(2), mutex(_Ceiling2)->holder);
}

static void testTaskTerminateFixesPriority(void) {
	command(2, (Command){.type=_MutexLock, .index=_Inherit1});
	command(4, (Command){.type=_MutexLock, .index=_Inherit1});
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_MUTEX_I, task(4)->task_wait_reason);
	TEST_ASSERT_EQUAL(priority(4), task(2)->priority);
	tn_task_terminate(task(4));
	TEST_ASSERT_EQUAL(priority(2), task(2)->priority);
}

#include "testMutex.c.run"
