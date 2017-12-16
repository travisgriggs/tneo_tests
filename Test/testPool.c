#include "testSupport.h"

TN_FMEM_BUF_DEF(_Mem, uint32_t, 6);

static void testCreateUnaligned(void) {
	struct TN_FMem pool;
	enum TN_RCode rc = tn_fmem_create(&pool, (void*)(1), 10, 0);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testCreateZeroSize(void) {
	struct TN_FMem pool;
	enum TN_RCode rc = tn_fmem_create(&pool, &_Mem, 0, 6);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc); // TODO: I Think this is a bug
}

static void testCreateOneCount(void) {
	struct TN_FMem pool;
	enum TN_RCode rc = tn_fmem_create(&pool, &_Mem, 4, 1);
	TEST_ASSERT_EQUAL_RC(TN_RC_WPARAM, rc);
}

static void testCreateOK(void) {
	struct TN_FMem pool;
	enum TN_RCode rc = tn_fmem_create(&pool, &_Mem, 4, 2);
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
}

static void testSerialGets(void) {
	command(0, (Command){.type=_PoolGet});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 0, PoolValuePointer);
	command(0, (Command){.type=_PoolGet});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 1, PoolValuePointer);
	command(0, (Command){.type=_PoolGet}); // should block
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_WFIXMEM, task(0)->task_wait_reason);
}

static void testSerialGetsPolling(void) {
	command(0, (Command){.type=_PoolGetPoll});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 0, PoolValuePointer);
	command(0, (Command){.type=_PoolGetPoll});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 1, PoolValuePointer);
	command(0, (Command){.type=_PoolGetPoll});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(0));
}

static void testSerialInterruptGetsPolling(void) {
	interrupt((Command){.type=_PoolGetPoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 0, PoolValuePointer);
	interrupt((Command){.type=_PoolGetPoll, .isInterrupt=true});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 1, PoolValuePointer);
	interrupt((Command){.type=_PoolGetPoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RCInterrupt);
}

static void testGetReleaseGet(void) {
	command(0, (Command){.type=_PoolGet});
	command(0, (Command){.type=_PoolGet});
	// all used up now, should block
	command(1, (Command){.type=_PoolGet});
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_WFIXMEM, task(1)->task_wait_reason);
	command(0, (Command){.type=_PoolRelease, .data=0});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL_PTR(PoolMemory, PoolValuePointer);
}

static void testGetUnlockOrder(void) {
	command(0, (Command){.type=_PoolGet});
	command(0, (Command){.type=_PoolGet});
	// all used up now, should block
	command(4, (Command){.type=_PoolGet});
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_WFIXMEM, task(4)->task_wait_reason);
	command(1, (Command){.type=_PoolGet});
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_WFIXMEM, task(1)->task_wait_reason);
	command(0, (Command){.type=_PoolGet});
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_WFIXMEM, task(0)->task_wait_reason);
	command(2, (Command){.type=_PoolRelease, .data=1});
	TEST_ASSERT_TRUE(isPending(4));
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(4));
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 1, PoolValuePointer);
	command(2, (Command){.type=_PoolRelease, .data=0});
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(1));
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 0, PoolValuePointer);
	command(2, (Command){.type=_PoolRelease, .data=1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 1, PoolValuePointer);
}

static void testRedundantRelease(void) {
	command(0, (Command){.type=_PoolRelease, .data=0});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OVERFLOW, RC(0));
	command(0, (Command){.type=_PoolRelease, .data=1});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OVERFLOW, RC(0));
}

static void testRedundantReleaseAfterGet(void) {
	command(0, (Command){.type=_PoolGet});
	command(0, (Command){.type=_PoolRelease, .data=0});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(0, (Command){.type=_PoolRelease, .data=0});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OVERFLOW, RC(0));
}

static void testGetTimeout(void) {
	command(0, (Command){.type=_PoolGet, .hasTimeout=true, .timeout=50});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 0, PoolValuePointer);
	command(0, (Command){.type=_PoolGet, .hasTimeout=true, .timeout=50});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	TEST_ASSERT_EQUAL_PTR(PoolMemory + 1, PoolValuePointer);
	command(0, (Command){.type=_PoolGet, .hasTimeout=true, .timeout=50});
	tn_task_sleep(40);
	TEST_ASSERT_FALSE(isPending(0));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_WFIXMEM, task(0)->task_wait_reason);
	tn_task_sleep(20);
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_TIMEOUT, RC(0));
}

static void testInterruptGetPollFromNormalContext(void) {
	command(0, (Command){.type=_PoolGetPoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RC(0));
}

static void testInterruptReleaseFromNormalContext(void) {
	command(0, (Command){.type=_PoolGet});
	command(0, (Command){.type=_PoolRelease, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RC(0));
}

static void testGetPollFromInterruptContext(void) {
	interrupt((Command){.type=_PoolGetPoll, .isInterrupt=false});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testReleaseFromInterruptContext(void) {
	interrupt((Command){.type=_PoolGetPoll, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RCInterrupt);
	interrupt((Command){.type=_PoolRelease, .isInterrupt=false});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testDeleteFromInterruptContext(void) {
	interrupt((Command){.type=_PoolDelete});
	TEST_ASSERT_EQUAL_RC(TN_RC_WCONTEXT, RCInterrupt);
}

static void testDoubleDelete(void) {
	command(0, (Command){.type=_PoolDelete, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	command(0, (Command){.type=_PoolDelete, .isInterrupt=true});
	TEST_ASSERT_EQUAL_RC(TN_RC_INVALID_OBJ, RC(0));
}

static void testFreeBlocks(void) {
	int count = tn_fmem_free_blocks_cnt_get(pool(0));
	TEST_ASSERT_EQUAL(2, count);
	command(0, (Command){.type=_PoolGet});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	count = tn_fmem_free_blocks_cnt_get(pool(0));
	TEST_ASSERT_EQUAL(1, count);
	command(0, (Command){.type=_PoolGet});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	count = tn_fmem_free_blocks_cnt_get(pool(0));
	TEST_ASSERT_EQUAL(0, count);
	command(0, (Command){.type=_PoolRelease});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	count = tn_fmem_free_blocks_cnt_get(pool(0));
	TEST_ASSERT_EQUAL(1, count);
}

static void testUsedBlocks(void) {
	int count = tn_fmem_used_blocks_cnt_get(pool(0));
	TEST_ASSERT_EQUAL(0, count);
	command(0, (Command){.type=_PoolGet});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	count = tn_fmem_used_blocks_cnt_get(pool(0));
	TEST_ASSERT_EQUAL(1, count);
	command(0, (Command){.type=_PoolGet});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	count = tn_fmem_used_blocks_cnt_get(pool(0));
	TEST_ASSERT_EQUAL(2, count);
	command(0, (Command){.type=_PoolRelease});
	TEST_ASSERT_TRUE(isPending(0));
	TEST_ASSERT_EQUAL_RC(TN_RC_OK, RC(0));
	count = tn_fmem_used_blocks_cnt_get(pool(0));
	TEST_ASSERT_EQUAL(1, count);
}

static void testDeleteReleases(void) {
	command(0, (Command){.type=_PoolGet});
	command(0, (Command){.type=_PoolGet});
	// all used up now, should block
	command(4, (Command){.type=_PoolGet});
	TEST_ASSERT_FALSE(isPending(4));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_WFIXMEM, task(4)->task_wait_reason);
	command(1, (Command){.type=_PoolGet});
	TEST_ASSERT_FALSE(isPending(1));
	TEST_ASSERT_EQUAL_REASON(TN_WAIT_REASON_WFIXMEM, task(1)->task_wait_reason);
	command(0, (Command){.type=_PoolDelete});
	TEST_ASSERT_TRUE(isPending(4));
	TEST_ASSERT_EQUAL_RC(TN_RC_DELETED, RC(4));
	TEST_ASSERT_TRUE(isPending(1));
	TEST_ASSERT_EQUAL_RC(TN_RC_DELETED, RC(1));
}

#include "testPool.c.run"
