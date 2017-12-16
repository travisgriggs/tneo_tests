#include "testSupport.h"

static void testCreateMaxCount0(void) {
	struct TN_Sem semaphore;
	enum TN_RCode rc = tn_sem_create(&semaphore, 0, 0);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testCreateMaxCountNegative(void) {
	struct TN_Sem semaphore;
	enum TN_RCode rc = tn_sem_create(&semaphore, -1, -1);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testCreateCountGreaterThanMax(void) {
	struct TN_Sem semaphore;
	enum TN_RCode rc = tn_sem_create(&semaphore, 3, 2);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testCreateCountNegative(void) {
	struct TN_Sem semaphore;
	enum TN_RCode rc = tn_sem_create(&semaphore, -1, 2);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testCreateOK(void) {
	struct TN_Sem semaphore;
	enum TN_RCode rc = tn_sem_create(&semaphore, 1, 3);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL(3, semaphore.max_count);
	TEST_ASSERT_EQUAL(1, semaphore.count);
}

static void testTaskReleaseReleases(void) {
	TEST_ASSERT_EQUAL(1, semaphore(0)->count);
	command(0, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL(0, semaphore(0)->count);
	command(0, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_EQUAL(0, semaphore(0)->count);
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(0)->task_wait_reason);
	// T1 forcibly releases T0 from wait
	command(1, (Command){.type=_ReleaseTask, .index=0});
	TEST_ASSERT_EQUAL(0, semaphore(0)->count);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_FORCED, RC(0));
}

static void testDeleteReleases(void) {
	TEST_ASSERT_EQUAL(1, semaphore(0)->count);
	command(0, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL(0, semaphore(0)->count);
	command(0, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_EQUAL(0, semaphore(0)->count);
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(0)->task_wait_reason);
	// T1 forcibly deletes sem
	command(1, (Command){.type=_SemaphoreDelete, .index=0});
	TEST_ASSERT_EQUAL(0, semaphore(0)->count);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_DELETED, RC(0));
}

static void testWaitWaitSignal(void) {
	command(0, (Command){.type=_SemaphoreWait});
	command(1, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(0, (Command){.type=_SemaphoreSignal});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
}

static void testSignalFromInterrupt(void) {
	command(0, (Command){.type=_SemaphoreWait});
	command(1, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(1)->task_wait_reason);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	interrupt((Command){.type=_SemaphoreSignal, .isInterrupt=true});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
}

static void testWaitTimeout(void) {
	command(0, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(1, (Command){.type=_SemaphoreWait, .hasTimeout=true, .timeout=50});
	tn_task_sleep(40);
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(1)->task_wait_reason);
	tn_task_sleep(20);
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(1));
}

static void testWaitPoll(void) {
	command(0, (Command){.type=_SemaphoreWaitPoll});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(1, (Command){.type=_SemaphoreWaitPoll});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(1));
}

static void testUnlockOrder(void) {
	command(2, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	command(3, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(3)->task_wait_reason);
	command(0, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(0)->task_wait_reason);
	command(4, (Command){.type=_SemaphoreWait});
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(4)->task_wait_reason);
	command(2, (Command){.type=_SemaphoreSignal});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_TRUE(isPending(3));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(3));
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(0)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(4)->task_wait_reason);
	command(2, (Command){.type=_SemaphoreSignal});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_SEM, task(4)->task_wait_reason);
	command(2, (Command){.type=_SemaphoreSignal});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_TRUE(isPending(4));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(4));
}

static void testInteruptWaitPoll(void) {
	interrupt((Command){.type=_SemaphoreWaitPoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL(0, semaphore(0)->count);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
	interrupt((Command){.type=_SemaphoreWaitPoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL(0, semaphore(0)->count);
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RCInterrupt);
}

static void testSignalOverflow(void) {
	TEST_ASSERT_EQUAL(semaphore(0)->max_count, semaphore(0)->count);
	command(0, (Command){.type=_SemaphoreSignal});
	TEST_ASSERT_EQUAL(semaphore(0)->max_count, semaphore(0)->count);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OVERFLOW, RC(0));
}

static void testInterruptSignalOverflow(void) {
	TEST_ASSERT_EQUAL(semaphore(0)->max_count, semaphore(0)->count);
	interrupt((Command){.type=_SemaphoreSignal, .isInterrupt=true});
	TEST_ASSERT_EQUAL(semaphore(0)->max_count, semaphore(0)->count);
	TEST_ASSERT_EQUAL_RC(TN_RC_OVERFLOW, RCInterrupt);
}

static void testWaitPollFromInterruptContext(void) {
	interrupt((Command){.type=_SemaphoreWaitPoll});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testWaitFromInterruptContext(void) {
	interrupt((Command){.type=_SemaphoreWait});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testDeleteFromInterruptContext(void) {
	interrupt((Command){.type=_SemaphoreDelete});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testSignalFromInterruptContext(void) {
	interrupt((Command){.type=_SemaphoreSignal});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testInterruptWaitPollFromNormalContext(void) {
	command(0, (Command){.type=_SemaphoreWaitPoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RC(0));
}

static void testInterruptSignalFromNormalContext(void) {
	command(0, (Command){.type=_SemaphoreSignal, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RC(0));
}

#include "testSemaphore.c.run"
