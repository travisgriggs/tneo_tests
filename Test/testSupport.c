#include "testSupport.h"
#include "samd21g18a.h"
#include "unity_internals.h"

#define TestletCount 5
#define TestStackSize (TN_MIN_STACK_SIZE + 50)
TN_STACK_ARR_DEF(Stacks, TestStackSize * TestletCount);
typedef struct {
	struct TN_Task task;
	void *fifo;
	struct TN_DQueue commandQueue;
	enum TN_RCode rc;
	bool isPending;
} Testlet;
Testlet Testlets[TestletCount];

#define MutexCount 4
struct TN_Mutex Mutexes[MutexCount];

#define SemaphoreCount 1
struct TN_Sem Semaphores[SemaphoreCount];

#define TimerCount 1
struct TN_Timer Timers[TimerCount];

#define QueueCount 3
struct TN_DQueue Queues[QueueCount];
static void *LittleFifo[1];
static void *BigFifo[4];
uint32_t QueueReceivedValue;

#define EventGroupCount 1
struct TN_EventGrp EventGroups[EventGroupCount];

#define PoolCount 1
struct TN_FMem Pools[PoolCount];
uint32_t PoolMemory[2];
void* PoolValuePointer;

struct TN_Mutex* mutex(int n) { return Mutexes + n; }
struct TN_Sem* semaphore(int n) { return Semaphores + n; }
struct TN_Timer* timer(int n) { return Timers + n; }
struct TN_DQueue* queue(int n) { return Queues + n; }
struct TN_EventGrp* eventgroup(int n) { return EventGroups + n; }
struct TN_FMem* pool(int n) { return Pools + n; }

enum TN_RCode RC(int n) { return (Testlets + n)->rc; }
struct TN_Task* task(int n) { return &(Testlets + n)->task; }
int priority(int n) { return TestletCount + 3 - n; }
bool isPending(int n)  {return (Testlets + n)->isPending; }

const char* rcToString(enum TN_RCode rc) {
	switch(rc) {
	case TN_RC_OK:
		return "OK";
	case TN_RC_DELETED:
		return "DELETED";
	case TN_RC_FORCED:
		return "FORCED";
	case TN_RC_ILLEGAL_USE:
		return "ILLEGAL_USE";
	case TN_RC_INTERNAL:
		return "INTERNAL";
	case TN_RC_INVALID_OBJ:
		return "INVALID_OBJECT";
	case TN_RC_OVERFLOW:
		return "OVERFLOW";
	case TN_RC_TIMEOUT:
		return "TIMEOUT";
	case TN_RC_WCONTEXT:
		return "WRONG_CONTEXT";
	case TN_RC_WPARAM:
		return "WRONG_PARAMETER";
	case TN_RC_WSTATE:
		return "WRONG_STATE";
	default:
		return "UnknownRC";
	}
}

const char* reasonToString(enum TN_WaitReason reason) {
	switch(reason) {
	case TN_WAIT_REASON_NONE:
		return "NONE";
	case TN_WAIT_REASON_DQUE_WRECEIVE:
		return "QUEUE_RECEIVE";
	case TN_WAIT_REASON_DQUE_WSEND:
		return "QUEUE_SEND";
	case TN_WAIT_REASON_EVENT:
		return "EVENT";
	case TN_WAIT_REASON_MUTEX_C:
		return "MUTEX (ceiling)";
	case TN_WAIT_REASON_MUTEX_I:
		return "MUTEX (inherit)";
	case TN_WAIT_REASON_SEM:
		return "SEMAPHORE";
	case TN_WAIT_REASON_SLEEP:
		return "SLEEP";
	case TN_WAIT_REASON_WFIXMEM:
		return "POOL";
	default:
		return "UnknownReason";
	}
}

void out(const char* s);

static enum TN_RCode dispatch(Command command) {
	TN_TickCnt timeout = command.hasTimeout ? command.timeout : TN_WAIT_INFINITE;
	void **queueReceiveTarget = command.mode == true ? NULL: (void**)&QueueReceivedValue;
	switch (command.type) {
	case _EventWait:
		return tn_eventgrp_wait(eventgroup(command.index), command.data, command.mode, NULL, timeout);
	case _EventWaitPoll:
		return command.isInterrupt ? tn_eventgrp_iwait_polling(eventgroup(command.index), command.data, command.mode, NULL) : tn_eventgrp_wait_polling(eventgroup(command.index), command.data, command.mode, NULL);
	case _EventModify:
		return command.isInterrupt ? tn_eventgrp_imodify(eventgroup(command.index), command.mode, command.data) : tn_eventgrp_modify(eventgroup(command.index), command.mode, command.data);
	case _EventDelete:
		return tn_eventgrp_delete(eventgroup(command.index));
	case _MutexLock:
		return tn_mutex_lock(mutex(command.index), timeout);
	case _MutexLockPoll:
		return tn_mutex_lock_polling(mutex(command.index));
	case _MutexUnlock:
		return tn_mutex_unlock(mutex(command.index));
	case _MutexDelete:
		return tn_mutex_delete(mutex(command.index));
	case _PoolGet:
		return tn_fmem_get(pool(command.index), &PoolValuePointer, timeout);
	case _PoolGetPoll:
		return command.isInterrupt ? tn_fmem_iget_polling(pool(command.index), &PoolValuePointer) : tn_fmem_get_polling(pool(command.index), &PoolValuePointer);
	case _PoolRelease:
		return command.isInterrupt ? tn_fmem_irelease(pool(command.index), PoolMemory + (command.data)) : tn_fmem_release(pool(command.index), PoolMemory + command.data);
	case _PoolDelete:
		return tn_fmem_delete(pool(command.index));
	case _QueueReceive:
		return tn_queue_receive(queue(command.index), queueReceiveTarget, timeout);
	case _QueueReceivePoll:
		return command.isInterrupt ? tn_queue_ireceive_polling(queue(command.index), queueReceiveTarget) : tn_queue_receive_polling(queue(command.index), queueReceiveTarget);
	case _QueueSend:
		return tn_queue_send(queue(command.index), (void*)(uint32_t)command.data, timeout);
	case _QueueSendPoll:
		return command.isInterrupt ? tn_queue_isend_polling(queue(command.index), (void*)(uint32_t)command.data) : tn_queue_send_polling(queue(command.index), (void*)(uint32_t)command.data);
	case _QueueDelete:
		return tn_queue_delete(queue(command.index));
	case _SemaphoreWait:
		return tn_sem_wait(semaphore(command.index), timeout);
	case _SemaphoreWaitPoll:
		return command.isInterrupt ? tn_sem_iwait_polling(semaphore(command.index)) : tn_sem_wait_polling(semaphore(command.index));
	case _SemaphoreSignal:
		return command.isInterrupt ? tn_sem_isignal(semaphore(command.index)) : tn_sem_signal(semaphore(command.index));
	case _SemaphoreDelete:
		return tn_sem_delete(semaphore(command.index));
	case _ReleaseTask:
		return tn_task_release_wait(task(command.index));
	case _TaskActivate:
		return command.isInterrupt ? tn_task_iactivate(task(command.index)) : tn_task_activate(task(command.index));
	case _TaskWakeup:
		return command.isInterrupt ? tn_task_iwakeup(task(command.index)) : tn_task_wakeup(task(command.index));
	case _TaskSleep:
		return tn_task_sleep(timeout);
	case _TaskSuspend:
		return tn_task_suspend(task(command.index));
	default:
		return -128;
	}
}

static void taskLoop(void *arg) {
	Testlet *testlet = (Testlet*)arg;
	tn_queue_create(&testlet->commandQueue, &testlet->fifo, 1);
	for ( ; ; ) {
		void* bits;
		testlet->isPending = true;
		tn_queue_receive(&testlet->commandQueue, &bits, TN_WAIT_INFINITE);
		testlet->isPending = false;
		Command command = (Command){.bits = (uint32_t)bits};
		if (command.type == _TaskExit) {
			tn_task_exit(TN_TASK_EXIT_OPT_DELETE);
		}
		if (command.type == _TaskReturn) {
			return;
		}
		testlet->rc = dispatch(command);
	}
}

void command(int which, Command command) {
	tn_queue_send(&(Testlets + which)->commandQueue, (void*)command.bits, TN_WAIT_INFINITE);
}

volatile static Command InterruptCommand;
volatile enum TN_RCode RCInterrupt;
void interrupt(Command command) {
	InterruptCommand = command;
	while (NVIC_GetPendingIRQ(21)) {  }
	NVIC_SetPendingIRQ(21);
	while (NVIC_GetPendingIRQ(21)) {  }
}

void PV21Handler(void) {
	RCInterrupt = dispatch(InterruptCommand);
}

uint32_t TimerCallbackValue = 0;
void timerCallback(struct TN_Timer *timer, void *arg) {
	*(uint32_t*)arg += 1;
}

void tearDown(void) {
	NVIC_DisableIRQ(21);
	for (Testlet *testlet = Testlets; testlet - Testlets < TestletCount; testlet++) {
		tn_task_terminate(&testlet->task);
		tn_task_delete(&testlet->task);
		tn_queue_delete(&testlet->commandQueue);
	}
	for (struct TN_Mutex *mutex = Mutexes; mutex - Mutexes < MutexCount; mutex++) {
		tn_mutex_delete(mutex);
	}
	for (struct TN_Sem *semaphore = Semaphores; semaphore - Semaphores < SemaphoreCount; semaphore++) {
		tn_sem_delete(semaphore);
	}
	for (struct TN_Timer *timer = Timers; timer - Timers < TimerCount; timer++) {
		tn_timer_delete(timer);
	}
	for (struct TN_DQueue *queue = Queues; queue - Queues < QueueCount; queue++) {
		tn_queue_eventgrp_disconnect(queue);
		tn_queue_delete(queue);
	}
	for (struct TN_EventGrp *eventgroup = EventGroups; eventgroup - EventGroups < EventGroupCount; eventgroup++) {
		tn_eventgrp_delete(eventgroup);
	}
	for (struct TN_FMem *pool = Pools; pool - Pools < PoolCount; pool++) {
		tn_fmem_delete(pool);
	}
}

void setUp(void) {
	NVIC_EnableIRQ(21);
	const char *oldFile = Unity.TestFile;
	Unity.TestFile = __FILE__;
	enum TN_RCode rc;
	unsigned int offset = 0;
	unsigned int index = 0;

	for (Testlet *testlet = Testlets; testlet - Testlets < TestletCount; testlet++) {
		rc = tn_task_create(&testlet->task, taskLoop, priority(index), Stacks + offset, TestStackSize, (void*)(testlet), TN_TASK_CREATE_OPT_START);
		TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
		tn_tick_int_processing(); // force reschedule
		TEST_ASSERT_EQUAL(testlet->task.priority, testlet->task.base_priority);
		TEST_ASSERT_EQUAL(TN_TASK_STATE_WAIT, testlet->task.task_state);
		TEST_ASSERT_EQUAL(TN_WAIT_REASON_DQUE_WRECEIVE, testlet->task.task_wait_reason);
		index += 1;
		offset += TestStackSize;
	}

	index = 0;
	for (struct TN_Mutex *mutex = Mutexes; mutex - Mutexes < MutexCount; mutex++) {
		rc = tn_mutex_create(mutex, (index > 1 ? TN_MUTEX_PROT_CEILING : TN_MUTEX_PROT_INHERIT), priority(index));
		TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
		TEST_ASSERT_EQUAL(NULL, mutex->holder);
		TEST_ASSERT_EQUAL(0, mutex->cnt);
		TEST_ASSERT_EQUAL(priority(index), mutex->ceil_priority);
		index += 1;
	}

	for (struct TN_Sem *semaphore = Semaphores; semaphore - Semaphores < SemaphoreCount; semaphore++) {
		rc = tn_sem_create(semaphore, 1, 1);
		TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
		TEST_ASSERT_EQUAL(1, semaphore->count);
		TEST_ASSERT_EQUAL(1, semaphore->max_count);
	}

	for (struct TN_Timer *timer = Timers; timer - Timers < TimerCount; timer++) {
		rc = tn_timer_create(timer, timerCallback, &TimerCallbackValue);
		TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	}
	TimerCallbackValue = 0;

	index = 0;
	void **fifo = NULL;
	for (struct TN_DQueue *queue = Queues; queue - Queues < QueueCount; queue++) {
		rc = tn_queue_create(queue, fifo, index * index);
		TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
		fifo = fifo == NULL ? LittleFifo : BigFifo;
		index += 1;
	}

	for (struct TN_EventGrp *eventgroup = EventGroups; eventgroup - EventGroups < EventGroupCount; eventgroup++) {
		rc = tn_eventgrp_create(eventgroup, 0);
		TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	}

	for (struct TN_FMem *pool = Pools; pool - Pools < PoolCount; pool++) {
		rc = tn_fmem_create(pool, PoolMemory, TN_MAKE_ALIG_SIZE(sizeof(uint32_t)), 2);
		TEST_ASSERT_EQUAL_RC(TN_RC_OK, rc);
	}
	Unity.TestFile = oldFile;
}


