#include "testSupport.h"

static void testExit(void) {
	// tn_task_exit() and then re-activate
	command(0, (Command){.type=_TaskExit});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_DORMANT, task(0)->task_state);
}

static void testReturn(void) {
	command(0, (Command){.type=_TaskReturn});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_DORMANT, task(0)->task_state);
}

static void testTerminate(void) {
	tn_task_terminate(task(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_DORMANT, task(0)->task_state);
}

static void testSecondTerminateFails(void) {
	tn_task_terminate(task(0));
	enum TN_RCode rc = tn_task_terminate(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_WSTATE, rc);
}

static void testActivateActiveFails(void) {
	enum TN_RCode rc = tn_task_activate(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_WSTATE, rc);
}

static void testActivateDormantSucceeds(void) {
	tn_task_terminate(task(0));
	enum TN_RCode rc = tn_task_activate(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testInterruptActivateDormantSucceeds(void) {
	tn_task_terminate(task(0));
	interrupt((Command){.type=_TaskActivate, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testInterruptActivateActiveFails(void) {
	interrupt((Command){.type=_TaskActivate, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_WSTATE, RCInterrupt);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testActivateFromInterruptContext(void) {
	interrupt((Command){.type=_TaskActivate, .isInterrupt=false});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testInterruptActivateFromNormalContext(void) {
	tn_task_terminate(task(0));
	enum TN_RCode rc = tn_task_iactivate(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, rc);
}

static void testSleep(void) {
	command(0, (Command){.type=_TaskSleep, .hasTimeout=true, .timeout=100});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_SLEEP, task(0)->task_wait_reason);
}

static void testSleepTerminate(void) {
	command(0, (Command){.type=_TaskSleep, .hasTimeout=true, .timeout=100});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_SLEEP, task(0)->task_wait_reason);
	tn_task_terminate(task(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_DORMANT, task(0)->task_state);
}

static void testSleepTerminateActivate(void) {
	command(0, (Command){.type=_TaskSleep, .hasTimeout=true, .timeout=100});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_SLEEP, task(0)->task_wait_reason);
	tn_task_terminate(task(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_DORMANT, task(0)->task_state);
	enum TN_RCode rc = tn_task_activate(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testSleepExpiry(void) {
	command(0, (Command){.type=_TaskSleep, .hasTimeout=true, .timeout=100});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_SLEEP, task(0)->task_wait_reason);
	tn_task_sleep(105);
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testWakup(void) {
	command(0, (Command){.type=_TaskSleep});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_SLEEP, task(0)->task_wait_reason);
	enum TN_RCode rc = tn_task_wakeup(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testWakeupFromInterruptContext(void) {
	command(0, (Command){.type=_TaskSleep});
	interrupt((Command){.type=_TaskWakeup, .isInterrupt=false});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_SLEEP, task(0)->task_wait_reason);
}

static void testInterruptWakeup(void) {
	command(0, (Command){.type=_TaskSleep});
	interrupt((Command){.type=_TaskWakeup, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testInterruptWakeupFromNormalContext(void) {
	command(0, (Command){.type=_TaskSleep});
	enum TN_RCode rc = tn_task_iwakeup(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, rc);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_SLEEP, task(0)->task_wait_reason);
}

static void testWakupNotSleeping(void) {
	enum TN_RCode rc = tn_task_wakeup(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_WSTATE, rc);
}

static void testSuspend(void) {
	enum TN_RCode rc = tn_task_suspend(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAITSUSP, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testSuspendFromInterruptContext(void) {
	interrupt((Command){.type=_TaskSuspend, .isInterrupt=false});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testDoubleSuspend(void) {
	tn_task_suspend(task(0));
	enum TN_RCode rc = tn_task_suspend(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_WSTATE, rc);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAITSUSP, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testResumeSuspended(void) {
	tn_task_suspend(task(0));
	enum TN_RCode rc = tn_task_resume(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testResumeRunning(void) {
	enum TN_RCode rc = tn_task_resume(task(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_WSTATE, rc);
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}



#include "testTask.c.run"
