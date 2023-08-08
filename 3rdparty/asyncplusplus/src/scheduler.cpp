// Copyright (c) 2015 Amanieu d'Antras
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "internal.h"

// for pthread thread_local emulation
#if defined(EMULATE_PTHREAD_THREAD_LOCAL)
# include <pthread.h>
#endif

namespace async {
namespace detail {

void* aligned_alloc(std::size_t size, std::size_t align)
{
#ifdef _WIN32
	void* ptr = _aligned_malloc(size, align);
	if (!ptr)
		LIBASYNC_THROW(std::bad_alloc());
	return ptr;
#else
	void* result;
	if (posix_memalign(&result, align, size))
		LIBASYNC_THROW(std::bad_alloc());
	else
		return result;
#endif
}

void aligned_free(void* addr) LIBASYNC_NOEXCEPT
{
#ifdef _WIN32
	_aligned_free(addr);
#else
	free(addr);
#endif
}

// Wait for a task to complete (for threads outside thread pool)
static void generic_wait_handler(task_wait_handle wait_task)
{
	// Create an event to wait on
	task_wait_event event;
	event.init();

	// Create a continuation for the task we are waiting for
	wait_task.on_finish([&event] {
		// Just signal the thread event
		event.signal(wait_type::task_finished);
	});

	// Wait for the event to be set
	event.wait();
}

#if defined(EMULATE_PTHREAD_THREAD_LOCAL)
// Wait handler function, per-thread, defaults to generic version
struct pthread_emulation_thread_wait_handler_key_initializer {
	pthread_key_t key;
		
	pthread_emulation_thread_wait_handler_key_initializer()
	{
		pthread_key_create(&key, nullptr);
	}
		
	~pthread_emulation_thread_wait_handler_key_initializer()
	{
		pthread_key_delete(key);
	}
};
	
static pthread_key_t get_thread_wait_handler_key()
{
	static pthread_emulation_thread_wait_handler_key_initializer initializer;
	return initializer.key;
}

#else
static THREAD_LOCAL wait_handler thread_wait_handler = generic_wait_handler;
#endif

static void set_thread_wait_handler(wait_handler handler)
{
#if defined(EMULATE_PTHREAD_THREAD_LOCAL)
	// we need to call this here, because the pthread initializer is lazy,
	// this means the it could be null and we need to set it before trying to
	// get or set it
	pthread_setspecific(get_thread_wait_handler_key(), reinterpret_cast<void*>(handler));
#else
	thread_wait_handler = handler;
#endif
}
	
static wait_handler get_thread_wait_handler()
{
#if defined(EMULATE_PTHREAD_THREAD_LOCAL)
	// we need to call this here, because the pthread initializer is lazy,
	// this means the it could be null and we need to set it before trying to
	// get or set it
	wait_handler handler = (wait_handler) pthread_getspecific(get_thread_wait_handler_key());
	if(handler == nullptr) {
		return generic_wait_handler;
	}
	return handler;
#else
	return thread_wait_handler;
#endif
}

// Wait for a task to complete
void wait_for_task(task_base* wait_task)
{
	// Dispatch to the current thread's wait handler
	wait_handler thread_wait_handler = get_thread_wait_handler();
	thread_wait_handler(task_wait_handle(wait_task));
}

// The default scheduler is just a thread pool which can be configured
// using environment variables.
class default_scheduler_impl: public threadpool_scheduler {
	static std::size_t get_num_threads()
	{
		// Get the requested number of threads from the environment
		// If that fails, use the number of CPUs in the system.
		std::size_t num_threads;
#ifdef _MSC_VER
		char* s;
# ifdef __cplusplus_winrt
		// Windows store applications do not support environment variables
		s = nullptr;
# else
		// MSVC gives an error when trying to use getenv, work around this
		// by using _dupenv_s instead.
		_dupenv_s(&s, nullptr, "LIBASYNC_NUM_THREADS");
# endif
#else
		const char *s = std::getenv("LIBASYNC_NUM_THREADS");
#endif
		if (s)
			num_threads = std::strtoul(s, nullptr, 10);
		else
			num_threads = hardware_concurrency();

#if defined(_MSC_VER) && !defined(__cplusplus_winrt)
		// Free the string allocated by _dupenv_s
		free(s);
#endif

		// Make sure the thread count is reasonable
		if (num_threads < 1)
			num_threads = 1;
		return num_threads;
	}

public:
	default_scheduler_impl()
		: threadpool_scheduler(get_num_threads()) {}
};

// Thread scheduler implementation
void thread_scheduler_impl::schedule(task_run_handle t)
{
	// A shared_ptr is used here because not all implementations of
	// std::thread support move-only objects.
	std::thread([](const std::shared_ptr<task_run_handle>& t) {
		t->run();
	}, std::make_shared<task_run_handle>(std::move(t))).detach();
}

} // namespace detail

threadpool_scheduler& default_threadpool_scheduler()
{
	return detail::singleton<detail::default_scheduler_impl>::get_instance();
}

// FIFO scheduler implementation
struct fifo_scheduler::internal_data {
	detail::fifo_queue queue;
	std::mutex lock;
};
fifo_scheduler::fifo_scheduler()
	: impl(new internal_data) {}
fifo_scheduler::~fifo_scheduler() {}
void fifo_scheduler::schedule(task_run_handle t)
{
	std::lock_guard<std::mutex> locked(impl->lock);
	impl->queue.push(std::move(t));
}
bool fifo_scheduler::try_run_one_task()
{
	task_run_handle t;
	{
		std::lock_guard<std::mutex> locked(impl->lock);
		t = impl->queue.pop();
	}
	if (t) {
		t.run();
		return true;
	}
	return false;
}
void fifo_scheduler::run_all_tasks()
{
	while (try_run_one_task()) {}
}

std::size_t hardware_concurrency() LIBASYNC_NOEXCEPT
{
	// Cache the value because calculating it may be expensive
	static std::size_t value = std::thread::hardware_concurrency();

	// Always return at least 1 core
	return value == 0 ? 1 : value;
}

wait_handler set_thread_wait_handler(wait_handler handler) LIBASYNC_NOEXCEPT
{
	wait_handler old = detail::get_thread_wait_handler();
	detail::set_thread_wait_handler(handler);
	return old;
}

} // namespace async

#ifndef LIBASYNC_STATIC
#if defined(__GNUC__) && !defined(_WIN32)
# pragma GCC visibility pop
#endif
#endif
