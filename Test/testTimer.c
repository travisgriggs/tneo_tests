#include "testSupport.h"

static void testCreate(void) {
	struct TN_Timer timer;
	enum TN_RCode rc = tn_timer_create(&timer, timerCallback, NULL);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testRedundantCreate(void) {
	enum TN_RCode rc = tn_timer_create(timer(0), timerCallback, NULL);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testDelete(void) {
	enum TN_RCode rc = tn_timer_delete(timer(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testDoubleDelete(void) {
	tn_timer_delete(timer(0));
	enum TN_RCode rc = tn_timer_delete(timer(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_INVALID_OBJ, rc);
}

static void testDeleteCreate(void) {
	tn_timer_delete(timer(0));
	enum TN_RCode rc = tn_timer_create(timer(0), timerCallback, NULL);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testFires(void) {
	enum TN_RCode rc = tn_timer_start(timer(0), 50);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL(0, TimerCallbackValue);
	tn_task_sleep(100);
	TEST_ASSERT_EQUAL(1, TimerCallbackValue);
}

static void testStart0(void) {
	enum TN_RCode rc = tn_timer_start(timer(0), 0);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
	TEST_ASSERT_EQUAL(0, TimerCallbackValue);
	tn_task_sleep(2);
	TEST_ASSERT_EQUAL(0, TimerCallbackValue);
}

static void testCancel(void) {
	tn_timer_start(timer(0), 50);
	TEST_ASSERT_EQUAL(0, TimerCallbackValue);
	enum TN_RCode rc = tn_timer_cancel(timer(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	tn_task_sleep(100);
	TEST_ASSERT_EQUAL(0, TimerCallbackValue);
}

static void testExpiredCancel(void) {
	tn_timer_start(timer(0), 50);
	TEST_ASSERT_EQUAL(0, TimerCallbackValue);
	tn_task_sleep(100);
	enum TN_RCode rc = tn_timer_cancel(timer(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL(1, TimerCallbackValue);
}

static void testIsActive(void) {
	tn_timer_start(timer(0), 50);
	TN_BOOL isActive = false;
	enum TN_RCode rc = tn_timer_is_active(timer(0), &isActive);
	TEST_ASSERT_TRUE(isActive);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL(0, TimerCallbackValue);
	tn_task_sleep(100);
	rc = tn_timer_is_active(timer(0), &isActive);
	TEST_ASSERT_FALSE(isActive);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	TEST_ASSERT_EQUAL(1, TimerCallbackValue);
}

static void testIsActivePostCancel(void) {
	tn_timer_start(timer(0), 50);
	TN_BOOL isActive = false;
	enum TN_RCode rc = tn_timer_is_active(timer(0), &isActive);
	TEST_ASSERT_TRUE(isActive);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	tn_timer_cancel(timer(0));
	rc = tn_timer_is_active(timer(0), &isActive);
	TEST_ASSERT_FALSE(isActive);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testTimeLeftAtCreate(void) {
	TN_TickCnt timeLeft = 42;
	enum TN_RCode rc = tn_timer_time_left(timer(0), &timeLeft);
	TEST_ASSERT_EQUAL(TN_WAIT_INFINITE, timeLeft);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

TN_TickCnt tickGet(void);

static void testTimeLeftAfterStart(void) {
	TN_TickCnt timeLeft = 42;
	TN_TickCnt now = tickGet();
	while (tickGet() == now) { } // wait for the falling edge of a tick change
	tn_timer_start(timer(0), 50);
	enum TN_RCode rc = tn_timer_time_left(timer(0), &timeLeft);
	TEST_ASSERT_EQUAL(50, timeLeft);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	tn_task_sleep(20);
	tn_timer_time_left(timer(0), &timeLeft);
	TEST_ASSERT_EQUAL(28, timeLeft);
	tn_task_sleep(25);
	tn_timer_time_left(timer(0), &timeLeft);
	TEST_ASSERT_EQUAL(1, timeLeft);
	tn_task_sleep(10);
	tn_timer_time_left(timer(0), &timeLeft);
	TEST_ASSERT_EQUAL(TN_WAIT_INFINITE, timeLeft);
}

static void testTimeLeftAfterCancel(void) {
	TN_TickCnt timeLeft = 42;
	tn_timer_start(timer(0), 50);
	tn_timer_cancel(timer(0));
	enum TN_RCode rc = tn_timer_time_left(timer(0), &timeLeft);
	TEST_ASSERT_EQUAL(TN_WAIT_INFINITE, timeLeft);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}



static void testAccuracy(void) {
	TN_TickCnt start;
	TN_TickCnt now = tickGet();
	while ((start = tickGet()) == now) { } // wait for the falling edge of a tick change
	tn_timer_start(timer(0), 10);
	while (TimerCallbackValue == 0) { tn_tick_int_processing(); } // spin wait for timer to trip // why do i need to drive the scheduler here?
	TN_TickCnt done = tickGet();
	TEST_ASSERT_EQUAL(10, done - start);
}



#include "testTimer.c.run"
