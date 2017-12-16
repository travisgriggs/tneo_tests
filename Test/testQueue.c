#include "testSupport.h"

static void testNoBuffer(void) {
	// Interrupt tries to read polling from dqueue
	interrupt((Command){.type=_QueueReceivePoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RCInterrupt);
	// Interrupt tries to write polling to dqueue
	interrupt((Command){.type=_QueueSendPoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RCInterrupt);
	// T0 tries to read polling from dqueue
	command(0, (Command){.type=_QueueReceivePoll});
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(0));
}

static void testEmptySendReceive(void) {
	// T1 tries to write to dqueue and blocks
	command(1, (Command){.type=_QueueSend, .data=42});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(1)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WSEND, task(1)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(1));
	// T0 reads from dqueue -> T1 unblocks
	command(0, (Command){.type=_QueueReceive});
	TEST_ASSERT_EQUAL(QueueReceivedValue, 42);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
}

static void testEmptyReceiveSend(void) {
	// T0 tries to read from dqueue and blocks
	command(0, (Command){.type=_QueueReceive});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(0));
	// T1 writes to dqueue -> T0 unblocks
	command(1, (Command){.type=_QueueSend, .data=13});
	TEST_ASSERT_EQUAL(QueueReceivedValue, 13);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
}

static void testEmptySendReceiveFromInterrupt(void) {
	// T1 tries to write to dqueue and blocks
	command(1, (Command){.type=_QueueSend, .data=42});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(1)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WSEND, task(1)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(1));
	// Interrupt reads from dqueue -> T1 unblocks
	interrupt((Command){.type=_QueueReceivePoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL(QueueReceivedValue, 42);
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
}

static void testEmptyReceiveSendFromInterrupt(void) {
	// T0 tries to read from dqueue and blocks
	command(0, (Command){.type=_QueueReceive});
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(0));
	// Interrupt writes to dqueue -> T0 unblocks
	interrupt((Command){.type=_QueueSendPoll, .isInterrupt=true, .data=13});
	TEST_ASSERT_EQUAL(13, QueueReceivedValue);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
}

static void testReceiveDeletion(void) {
   // T0 tries to read from dqueue and blocks
   QueueReceivedValue = 8888;
   command(0, (Command){.type=_QueueReceive});
   TEST_ASSERT_FALSE(isPending(0));
   // dqueue gets deleted -> T0 unblocks with TERR_DLT");
   enum TN_RCode rc = tn_queue_delete(queue(0));
   TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
   TEST_ASSERT_EQUAL_RC(TN_RC_DELETED, RC(0));
   TEST_ASSERT_EQUAL(8888, QueueReceivedValue);
}

static void testSendDeletion(void) {
   // T0 tries to write to dqueue and blocks
   QueueReceivedValue = 8888;
   command(0, (Command){.type=_QueueSend, .data=99});
   TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
   TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WSEND, task(0)->task_wait_reason);
   TEST_ASSERT_FALSE(isPending(0));
   // dqueue gets deleted -> T0 unblocks with TERR_DLT");
   enum TN_RCode rc = tn_queue_delete(queue(0));
   TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
   TEST_ASSERT_EQUAL_RC(TN_RC_DELETED, RC(0));
   TEST_ASSERT_EQUAL(8888, QueueReceivedValue);
}

static void testCreateNegativeCount(void) {
	struct TN_DQueue q;
	enum TN_RCode rc = tn_queue_create(&q, NULL, -1);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testReceiveTimeout(void) {
	command(0, (Command){.type=_QueueReceive, .index=0, .hasTimeout=true, .timeout=50});
	tn_task_sleep(40);
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
	tn_task_sleep(20);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(0));
}

static void testSendTimeout(void) {
	command(0, (Command){.type=_QueueSend, .index=0, .hasTimeout=true, .timeout=50});
	tn_task_sleep(40);
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WSEND, task(0)->task_wait_reason);
	tn_task_sleep(20);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(0));
}

static void testReceiveUntilEmpty(void) {
	QueueReceivedValue = 0;
	command(0, (Command){.type=_QueueSend, .index=2, .data=13});
	command(0, (Command){.type=_QueueSend, .index=2, .data=31});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(0));
	command(1, (Command){.type=_QueueReceive, .index=2});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL(13, QueueReceivedValue);
	command(1, (Command){.type=_QueueReceive, .index=2});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL(31, QueueReceivedValue);
	command(1, (Command){.type=_QueueReceive, .index=2});
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(1)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(1)->task_wait_reason);
}

static void testSendUntilFull(void) {
	QueueReceivedValue = 0;
	command(0, (Command){.type=_QueueSend, .index=2, .data=5});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(0));
	command(0, (Command){.type=_QueueSend, .index=2, .data=8});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(0));
	command(0, (Command){.type=_QueueSend, .index=2, .data=13});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(0));
	command(0, (Command){.type=_QueueSend, .index=2, .data=21});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(0));
	command(0, (Command){.type=_QueueSend, .index=2, .data=34});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WSEND, task(0)->task_wait_reason);
	command(1, (Command){.type=_QueueReceive, .index=2});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
	TEST_ASSERT_EQUAL(5, QueueReceivedValue);
	command(1, (Command){.type=_QueueReceive, .index=2});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL(8, QueueReceivedValue);
	command(1, (Command){.type=_QueueReceive, .index=2});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL(13, QueueReceivedValue);
	command(1, (Command){.type=_QueueReceive, .index=2});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL(21, QueueReceivedValue);
	command(1, (Command){.type=_QueueReceive, .index=2});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL(34, QueueReceivedValue);
}

static void testUnblockReceiveOrder(void) {
	QueueReceivedValue = 100;
	command(0, (Command){.type=_QueueReceive, .index=1});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(0)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
	command(4, (Command){.type=_QueueReceive, .index=1});
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(4)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(4)->task_wait_reason);
	command(1, (Command){.type=_QueueReceive, .index=1});
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(1)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(1)->task_wait_reason);
	command(2, (Command){.type=_QueueReceive, .index=1});
	TEST_ASSERT_FALSE(isPending(2));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(2)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(2)->task_wait_reason);
	TEST_ASSERT_EQUAL(100, QueueReceivedValue);
	// everyone but 3 is blocked, now start sending values and make sure reads happen in the correct order
	command(3, (Command){.type=_QueueSend, .index=1, .data=1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL(1, QueueReceivedValue);
	command(3, (Command){.type=_QueueSend, .index=1, .data=44});
	TEST_ASSERT_TRUE(isPending(4));
	TEST_ASSERT_EQUAL(44, QueueReceivedValue);
	command(3, (Command){.type=_QueueSend, .index=1, .data=11});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL(11, QueueReceivedValue);
	command(3, (Command){.type=_QueueSend, .index=1, .data=22});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL(22, QueueReceivedValue);
}

static void testUnblockSendOrder(void) {
	QueueReceivedValue = 1111;
	command(3, (Command){.type=_QueueSend, .data=30});
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(3)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WSEND, task(3)->task_wait_reason);
	command(1, (Command){.type=_QueueSend, .data=10});
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(1)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WSEND, task(1)->task_wait_reason);
	command(4, (Command){.type=_QueueSend, .data=40});
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, task(4)->task_state);
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WSEND, task(4)->task_wait_reason);
	// 3, 1, and 4 are blocked, now read the values and make they show up in the queued order
	command(2, (Command){.type=_QueueReceive});
	TEST_ASSERT_TRUE(isPending(3));
	TEST_ASSERT_EQUAL(30, QueueReceivedValue);
	command(2, (Command){.type=_QueueReceive});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL(10, QueueReceivedValue);
	command(2, (Command){.type=_QueueReceive});
	TEST_ASSERT_TRUE(isPending(4));
	TEST_ASSERT_EQUAL(40, QueueReceivedValue);
}

static void testInterruptReceiveToNull(void) {
	QueueReceivedValue = 44;
	command(0, (Command){.type=_QueueSend, .data=42});
	interrupt((Command){.type=_QueueReceivePoll, .mode=true, .isInterrupt=true}); // an abuse of the .data argument, true in this case means we try to store in NULL
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RCInterrupt);
	TEST_ASSERT_FALSE(isPending(0)); // still blocked
	TEST_ASSERT_EQUAL(44, QueueReceivedValue);
	// now make sure we didn't screw up the latent value by actually pulling it off correctly
	interrupt((Command){.type=_QueueReceivePoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL(42, QueueReceivedValue);
}

static void testReceiveToNull(void) {
	QueueReceivedValue = 44;
	command(0, (Command){.type=_QueueSend, .data=42});
	command(1, (Command){.type=_QueueReceive, .mode=true}); // an abuse of the .data argument, true in this case means we try to store in NULL
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, RC(1));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_FALSE(isPending(0)); // still blocked
	TEST_ASSERT_EQUAL(44, QueueReceivedValue);
	// now make sure we didn't screw up the latent value by actually pulling it off correctly
	command(1, (Command){.type=_QueueReceive});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL(42, QueueReceivedValue);
}

static void testDeleteFromInterruptContext(void) {
	interrupt((Command){.type=_QueueDelete});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testSendFromInterruptContext(void) {
	interrupt((Command){.type=_QueueSend, .isInterrupt=false});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testSendPollFromInterruptContext(void) {
	interrupt((Command){.type=_QueueSendPoll, .isInterrupt=false});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testInterruptSendPollFromNormalContext(void) {
	command(0, (Command){.type=_QueueSendPoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RC(0));
}

static void testReceiveFromInterruptContext(void) {
	interrupt((Command){.type=_QueueReceive, .isInterrupt=false});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testReceivePollFromInterruptContext(void) {
	interrupt((Command){.type=_QueueReceivePoll, .isInterrupt=false});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testInterruptReceivePollFromNormalContext(void) {
	command(0, (Command){.type=_QueueReceivePoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RC(0));
}

static void testFreeItems(void) {
	TEST_ASSERT_EQUAL(0, tn_queue_free_items_cnt_get(queue(0)));
	TEST_ASSERT_EQUAL(1, tn_queue_free_items_cnt_get(queue(1)));
	TEST_ASSERT_EQUAL(4, tn_queue_free_items_cnt_get(queue(2)));
	command(0, (Command){.type=_QueueSend, .index=1}); // add 1 to Q1
	command(1, (Command){.type=_QueueSend, .index=2}); // add 2 to Q2
	command(2, (Command){.type=_QueueSend, .index=2});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL(0, tn_queue_free_items_cnt_get(queue(1)));
	TEST_ASSERT_EQUAL(2, tn_queue_free_items_cnt_get(queue(2)));
}

static void testUsedItems(void) {
	TEST_ASSERT_EQUAL(0, tn_queue_used_items_cnt_get(queue(0)));
	TEST_ASSERT_EQUAL(0, tn_queue_used_items_cnt_get(queue(1)));
	TEST_ASSERT_EQUAL(0, tn_queue_used_items_cnt_get(queue(2)));
	command(0, (Command){.type=_QueueSend, .index=1}); // add 1 to Q1
	command(1, (Command){.type=_QueueSend, .index=2}); // add 2 to Q2
	command(2, (Command){.type=_QueueSend, .index=2});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL(1, tn_queue_used_items_cnt_get(queue(1)));
	TEST_ASSERT_EQUAL(2, tn_queue_used_items_cnt_get(queue(2)));
}

static void testEventGroup(void) {
	enum TN_RCode rc = tn_queue_eventgrp_connect(queue(1), eventgroup(0), 0b01);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	rc = tn_queue_eventgrp_connect(queue(2), eventgroup(0), 0b10);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	command(3, (Command){.type=_EventWait, .data=0b11, .mode=TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(3)->task_wait_reason);
	command(1, (Command){.type=_QueueSend, .index=1, .data=22});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(3)->task_wait_reason);
	command(2, (Command){.type=_QueueSend, .index=2, .data=33});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	TEST_ASSERT_TRUE(isPending(3));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(3)->task_wait_reason);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(3));
}

static void testEventGroupAlreadyHasElements(void) {
	enum TN_RCode rc = tn_queue_eventgrp_connect(queue(1), eventgroup(0), 0b01);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	rc = tn_queue_eventgrp_connect(queue(2), eventgroup(0), 0b10);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	command(1, (Command){.type=_QueueSend, .index=1, .data=11});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	command(2, (Command){.type=_QueueSend, .index=2, .data=22});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	command(0, (Command){.type=_EventWait, .data=0b11, .mode=TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, task(0)->task_wait_reason);
}

static void testEventGroupConnectAfterAlreadyHasElements(void) {
	command(1, (Command){.type=_QueueSend, .index=1, .data=11});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	command(2, (Command){.type=_QueueSend, .index=2, .data=22});
	TEST_ASSERT_TRUE(isPending(2));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(2));
	enum TN_RCode rc = tn_queue_eventgrp_connect(queue(1), eventgroup(0), 0b01);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	rc = tn_queue_eventgrp_connect(queue(2), eventgroup(0), 0b10);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	command(0, (Command){.type=_EventWait, .data=0b11, .mode=TN_EVENTGRP_WMODE_AND});
	TEST_ASSERT_FALSE(isPending(0)); // because connect doesn't set the events based on current state
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(0)->task_wait_reason);
}

static void testEmptyDoesNotDriveEventGroup(void) {
	enum TN_RCode rc = tn_queue_eventgrp_connect(queue(0), eventgroup(0), 0b001);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	command(3, (Command){.type=_EventWait, .data=0b001, .mode=TN_EVENTGRP_WMODE_OR});
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(3)->task_wait_reason);
	command(0, (Command){.type=_QueueSend, .index=0, .data=11});
	TEST_ASSERT_FALSE(isPending(0)); // because Q0 is zero sized and therefore blocks
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WSEND, task(0)->task_wait_reason);
	TEST_ASSERT_FALSE(isPending(3));
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(3)->task_wait_reason);
	// even if we read the value through the zero size, it doesn't trigger
	command(4, (Command){.type=_QueueReceive, .index=0});
	TEST_ASSERT_TRUE(isPending(4));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(4));
	TEST_ASSERT_TRUE(isPending(0)); // should unblock now
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_FALSE(isPending(3)); // still stuck
	TEST_ASSERT_EQUAL(TN_WAIT_REASON_EVENT, task(3)->task_wait_reason);
	rc = tn_queue_eventgrp_disconnect(queue(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

#include "testQueue.c.run"
