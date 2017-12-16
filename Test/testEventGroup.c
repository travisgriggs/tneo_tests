#include "testSupport.h"

static void testDeleteFromInterruptContext(void) {
	interrupt((Command){.type=_EventDelete, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testWaitForZero(void) {
	command(0, (Command){.type=_EventWait, .data=0, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(0));
}

static void testWaitForBothModes(void) {
	command(0, (Command){.type=_EventWait, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR | TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(0));
}

static void testWaitForNoMode(void) {
	command(0, (Command){.type=_EventWait, .data=0b11, .mode=0});
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(0));
}

static void testWaitPollForZero(void) {
	command(0, (Command){.type=_EventWaitPoll, .data=0, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(0));
}

static void testWaitPollForBothModes(void) {
	command(0, (Command){.type=_EventWaitPoll, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR | TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(0));
}

static void testWaitPollForNoMode(void) {
	command(0, (Command){.type=_EventWaitPoll, .data=0b11, .mode=0});
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(0));
}

static void testInterruptWaitPollForZero(void) {
	interrupt((Command){.type=_EventWaitPoll, .isInterrupt=true, .data=0, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(0));
}

static void testInterruptWaitPollForBothModes(void) {
	interrupt((Command){.type=_EventWaitPoll, .isInterrupt=true, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR | TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(0));
}

static void testInterruptWaitPollForNoMode(void) {
	interrupt((Command){.type=_EventWaitPoll, .isInterrupt=true, .data=0b11, .mode=0});
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(0));
}

static void testWaitForDeletedGroup(void) {
	enum TN_RCode rc = tn_eventgrp_delete(eventgroup(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	command(0, (Command){.type=_EventWait, .index=0, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_EQUAL_RC(TN_RC_INVALID_OBJ, RC(0));
}

static void testWaitPollForDeletedGroup(void) {
	enum TN_RCode rc = tn_eventgrp_delete(eventgroup(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	command(0, (Command){.type=_EventWaitPoll, .index=0, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_EQUAL_RC(TN_RC_INVALID_OBJ, RC(0));
}

static void testInterruptWaitForDeletedGroup(void) {
	enum TN_RCode rc = tn_eventgrp_delete(eventgroup(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	interrupt((Command){.type=_EventWaitPoll, .isInterrupt=true, .index=0, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_EQUAL_RC(TN_RC_INVALID_OBJ, RCInterrupt);
}

static void testWaitPollFromInterruptContext(void) {
	command(0, (Command){.type=_EventWaitPoll, .isInterrupt=true, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RC(0));
}

static void testInterruptWaitPollFromNormalContext(void) {
	interrupt((Command){.type=_EventWaitPoll, .isInterrupt=false, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testModifyFromInterruptContext(void) {
	command(0, (Command){.type=_EventModify, .isInterrupt=true, .data=0b11, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RC(0));
}

static void testInterruptModifyFromNormalContext(void) {
	interrupt((Command){.type=_EventModify, .isInterrupt=false, .data=0b11, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testWaitForOr(void) {
	command(0, (Command){.type=_EventWait, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
	command(1, (Command){.type=_EventModify, .data=0b01, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
}

static void testWaitForOrAlreadySet(void) {
	command(1, (Command){.type=_EventModify, .data=0b11, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	command(0, (Command){.type=_EventWait, .data=0b10, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_TRUE(isPending(0));
}

static void testWaitForAnd(void) {
	command(0, (Command){.type=_EventWait, .data=0b11, .mode=TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
	command(1, (Command){.type=_EventModify, .data=0b01, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	command(1, (Command){.type=_EventModify, .data=0b01, .mode=TN_EVENTGRP_OP_CLEAR}); // make sure the set is cumalitive by clearing it before we set the other bit
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
	command(1, (Command){.type=_EventModify, .data=0b10, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
	command(1, (Command){.type=_EventModify, .data=0b01, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(0));
}

static void testWaitForAndAlreadySet(void) {
	command(1, (Command){.type=_EventModify, .data=0b1111, .mode=TN_EVENTGRP_OP_SET});
	command(0, (Command){.type=_EventWait, .data=0b1010, .mode=TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
}

static void testMultipleWaitingTasks(void) {
	command(0, (Command){.type=_EventWait, .data=0b0011, .mode=TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
	command(1, (Command){.type=_EventWait, .data=0b0110, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(1)->task_wait_reason);
	command(2, (Command){.type=_EventWait, .data=0b1100, .mode=TN_EVENTGRP_WMODE_OR | TN_EVENTGRP_WMODE_AUTOCLR});
	TEST_ASSERT_FALSE(isPending(2));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(2)->task_wait_reason);
	command(3, (Command){.type=_EventWait, .data=0b1100, .mode=TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(3)->task_wait_reason);
	interrupt((Command){.type=_EventModify, .isInterrupt=true, .data=0b0111, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_EQUAL(0b0011, eventgroup(0)->pattern);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_FALSE(isPending(3));
}

static void testWaitForAndWithAutoclear(void) {
	command(1, (Command){.type=_EventModify, .data=0b101, .mode=TN_EVENTGRP_OP_SET});
	command(0, (Command){.type=_EventWait, .data=0b11, .mode=TN_EVENTGRP_WMODE_AND | TN_EVENTGRP_WMODE_AUTOCLR});
	TEST_ASSERT_EQUAL(0b101, eventgroup(0)->pattern);
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
	command(1, (Command){.type=_EventModify, .data=0b010, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL(0b100, eventgroup(0)->pattern);
}

static void testWaitForOrWithAutoclear(void) {
	command(0, (Command){.type=_EventWait, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR | TN_EVENTGRP_WMODE_AUTOCLR});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
	command(1, (Command){.type=_EventModify, .data=0b111, .mode=TN_EVENTGRP_OP_SET});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL(0b100, eventgroup(0)->pattern);
}

static void testWaitTimeout(void) {
	command(0, (Command){.type=_EventWait, .data=0b11, .mode=TN_EVENTGRP_WMODE_OR | TN_EVENTGRP_WMODE_AUTOCLR, .hasTimeout=true, .timeout=50});
	tn_task_sleep(40);
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
	tn_task_sleep(20);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(0));
}

#include "testEventGroup.c.run"
