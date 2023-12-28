/*
This file is a part of the JThread package, which contains some object-
oriented thread wrappers for different thread implementations.

Copyright (c) 2000-2006  Jori Liesenborgs (jori.liesenborgs@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "threading/thread.h"
#include "threading/mutex_auto_lock.h"
#include "log.h"
#include "porting.h"

// for setName
#if defined(__linux__)
	#include <sys/prctl.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
	#include <pthread_np.h>
#elif defined(__NetBSD__)
	#include <sched.h>
#endif

Thread::Thread(const std::string &name) :
	m_name(name),
	m_request_stop(false),
	m_running(false)
{
}


Thread::~Thread()
{
	// kill the thread if running
	if (!m_running) {
		wait();
	} else {

		m_running = false;

		pthread_cancel(getThreadHandle());
		wait();
	}

	// Make sure start finished mutex is unlocked before it's destroyed
	if (m_start_finished_mutex.try_lock())
		m_start_finished_mutex.unlock();
}


bool Thread::start()
{
	MutexAutoLock lock(m_mutex);

	if (m_running)
		return false;

	m_request_stop = false;

	// The mutex may already be locked if the thread is being restarted
	// FIXME: what if this fails, or if already locked by same thread?
	MutexAutoLock sf_lock(m_start_finished_mutex, std::try_to_lock);

	try {
		m_thread_obj = new std::thread(threadProc, this);
	} catch (const std::system_error &e) {
		return false;
	}

	while (!m_running)
		sleep_ms(1);

	// Allow spawned thread to continue
	sf_lock.unlock();

	m_joinable = true;

	return true;
}


bool Thread::stop()
{
	m_request_stop = true;
	return true;
}


bool Thread::wait()
{
	MutexAutoLock lock(m_mutex);

	if (!m_joinable)
		return false;


	m_thread_obj->join();

	delete m_thread_obj;
	m_thread_obj = nullptr;

	assert(m_running == false);
	m_joinable = false;
	return true;
}



bool Thread::getReturnValue(void **ret)
{
	if (m_running)
		return false;

	*ret = m_retval;
	return true;
}


void Thread::threadProc(Thread *thr)
{
	thr->setName(thr->m_name);

	g_logger.registerThread(thr->m_name);
	thr->m_running = true;

	// Wait for the thread that started this one to finish initializing the
	// thread handle so that getThreadId/getThreadHandle will work.
	MutexAutoLock sf_lock(thr->m_start_finished_mutex);

	thr->m_retval = thr->run();

	thr->m_running = false;
	// Unlock m_start_finished_mutex to prevent data race condition on Windows.
	// On Windows with VS2017 build TerminateThread is called and this mutex is not
	// released. We try to unlock it from caller thread and it's refused by system.
	sf_lock.unlock();
	g_logger.deregisterThread();
}


void Thread::setName(const std::string &name)
{
#if defined(__linux__)

	// It would be cleaner to do this with pthread_setname_np,
	// which was added to glibc in version 2.12, but some major
	// distributions are still runing 2.11 and previous versions.
	prctl(PR_SET_NAME, name.c_str());

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)

	pthread_set_name_np(pthread_self(), name.c_str());

#elif defined(__NetBSD__)

	pthread_setname_np(pthread_self(), "%s", const_cast<char*>(name.c_str()));

#elif defined(__HAIKU__)

	rename_thread(find_thread(NULL), name.c_str());

#else
	#warning "Unrecognized platform, thread names will not be available."
#endif
}


unsigned int Thread::getNumberOfProcessors()
{
	return std::thread::hardware_concurrency();
}


bool Thread::bindToProcessor(unsigned int proc_number)
{
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(proc_number, &cpuset);

	return pthread_setaffinity_np(getThreadHandle(), sizeof(cpuset), &cpuset) == 0;
}


bool Thread::setPriority(int prio)
{
	struct sched_param sparam;
	int policy;

	if (pthread_getschedparam(getThreadHandle(), &policy, &sparam) != 0)
		return false;

	int min = sched_get_priority_min(policy);
	int max = sched_get_priority_max(policy);

	sparam.sched_priority = min + prio * (max - min) / THREAD_PRIORITY_HIGHEST;
	return pthread_setschedparam(getThreadHandle(), policy, &sparam) == 0;
}

