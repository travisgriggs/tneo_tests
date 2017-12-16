#tneo\_tests

tneo\_tests contains a set of Unity based unit tests for the TNeo micro kernel. There are 172 tests at the time of writing.

I wrote tneo\_tests because I was curious if I could write a simple board setup for my SAMD21 (Cortex M0+), throw Unity and TNeo basically unmodified into the pile of source files, and have a battery of tests that ran native on the board.

There is a certain catch-22 in such a proposal. The fact that it can run tests against itself makes some of the tested behavior (i.e. basic task switching) redundant. IOW, if TNeo was not working at a certain minimal level, it would not even be able to run tests against itself.

Doing so, gave me the following:
1. A much more thorough understanding of the TNeo API.
2. Much increased confidence that it can handle a variety of common micro kernel challenges and is basically "up to snuff."
3. Discovery of a handful of nuanced edge cases that I might otherwise have taken for granted.
4. Even found a bug in TNeo's recovery from mutex deadlocks that I was then able to isolate and prove a fix for.

# Writing the Tests
The tests started out after Dmitry generously shared his original test suite with me. These tests used a custom test engine and followed a feature test pattern, where a single test would test many things in one test. I took these, built a similiar test support engine, and factored the tests into a much higher number of singular tests which each strive to prove the one thing that their function name says they do. Over time, I began deriving my tests as much from the documented APIs, and then consulting the originals to make sure there weren't sequences of assertions I hadn't missed.

Each battery of tests, tests a single TNeo struct type (e.g. testMutex is for testing the struct TN\_Mutex objects). To automate updating the list of test functions to run, each testXXXX.c file includes an additionaly testXXXX.c.run file at the end. The included `buildTests.py` script was used to update these files as well as the allTestsRun.c file at build time so that I could simply define the testXXX() functions and automate the rest. One could use this script or do something different.

# testSupport.c
A single setUp/tearDown is used for all of the tests. I toyed with having per file setUp/tearDown functions, but concluded that there was enough overlap that it wasn't worth it. So each setUp, enough resources are created for all of the different tests, and then torn down after. This surely thrashes the basic operations repeated there, but that kind of thrashing had certain merits.

The basic approach is that for each setUp we set up a number of "testlets" (currently 5) which basically wrap around a task, creating a loop that receives commands through a queue which it then acts on. The testlet can keep track of the rc (return code). A command() function allows the test scripts to issue different TNeo API calls to the various testlets and assert state betwixt. Additionally, a battery of other TNeo objects (mutexes, semaphores, pools, eventgroups, queues, timers) are setUp and torn down.

The Command argument is done as a bitfield which allows us to pack a variety of different paremeters into the same 32 bits the queue's void\* elements. Also, we can use structure initializers to make the order of the fields arbitrary as well as optional and name them for what they are.

Some of the TNeo APIs are meant to be called from an interrupt. To do this, we create an interrupt that we can drive programatically. The interrupt() function drives the same dispatch function that the testlets dispatch their commands through. However, instead of queueing to a testlet, it stores the state globally, and then triggers the interrupt so it can be called from there. The rc value is copied back to a global the test scripts can observe.

#TNeo Configuration
My particular implementation enables `TN\_DYNAMIC\_TICK=1` and uses the chip's RTC to tick at 1024Hz. The idle\_task callback puts the board to sleep.

All of the other TNeo defaults are left as is. 

The TN\_CBUserTaskCreate function creates a single task at priority level 20 (arbitrary, just needs to be below the priority of task(0) from testSupport.c), which runs `allTestsRun(void *\_)`  as it's task body.

Also, before TNeo starts, it should configure the deadlock callback as follows:

    // prototype for function found in testMutex.c
    void deadlockDetected(TN\_BOOL active, struct TN\_Mutex *mutex, struct TN\_Task *task);
	...
    tn\_callback\_deadlock\_set(deadlockDetected);

# Additional Board Support
To get output, you will need to define the Unity macros for IO. E.g.

    #define UNITY\_OUTPUT\_CHAR(a)                    outc(a)
    #define UNITY\_OUTPUT\_CHAR\_HEADER\_DECLARATION    outc(char)

Where outc is a function that drives a single character out of the serial port.

To test interrupts in TNEO, we need to be able to execute code from a user interrupt. On my particular board, I had an unused peripheral at pin 21. The samd21 `DeviceVectors exception\_table` definition had to be changed to set interrupt 21 with an actual handler, rather than null. The handler shows up in testSupport.c where it is enabled at setUp, disabled at tearDown.

#Further ToDo
I would like to find a lighter weight mechanism for the test running task to communicate commands to the testlets. While having a task per testlet seems inevitable, it would be nice to find a mechanism that is less dependent on as much of TNeo's behavior to work correctly. Possibly just having a free variable on the testlet structure and manipulating the task's suspension state directly.
