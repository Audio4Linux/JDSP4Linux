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

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <type_traits>
#include <vector>

#include <async++.h>

// For posix_memalign/_aligned_malloc
#ifdef _WIN32
# include <malloc.h>
# ifdef __MINGW32__
#  define _aligned_malloc __mingw_aligned_malloc
#  define _aligned_free __mingw_aligned_free
# endif
#else
# include <stdlib.h>
#endif

// We don't make use of dynamic TLS initialization/destruction so we can just
// use the legacy TLS attributes.
#ifdef __GNUC__
# define  THREAD_LOCAL __thread
#elif defined (_MSC_VER)
# define THREAD_LOCAL __declspec(thread)
#else
# define THREAD_LOCAL thread_local
#endif

// GCC, Clang and the Linux version of the Intel compiler and MSVC 2015 support
// thread-safe initialization of function-scope static variables.
#ifdef __GNUC__
# define HAVE_THREAD_SAFE_STATIC
#elif _MSC_VER >= 1900 && !defined(__INTEL_COMPILER)
# define HAVE_THREAD_SAFE_STATIC
#endif

// MSVC deadlocks when joining a thread from a static destructor. Use a
// workaround in that case to avoid the deadlock.
#if defined(_MSC_VER) && _MSC_VER < 1900
# define BROKEN_JOIN_IN_DESTRUCTOR
#endif

// Apple's iOS has no thread local support yet. They claim that they don't want to
// introduce a binary compatility issue when they got a better implementation available.
// Luckily, pthreads supports some kind of "emulation" for that. This detects if the we
// are compiling for iOS and enables the workaround accordingly.
// It is also possible enabling it forcibly by setting the EMULATE_PTHREAD_THREAD_LOCAL
// macro. Obviously, this will only works on platforms with pthread available.
#if __APPLE__
# include "TargetConditionals.h"
# if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#  define EMULATE_PTHREAD_THREAD_LOCAL
# endif
#endif

// Force symbol visibility to hidden unless explicity exported
#ifndef LIBASYNC_STATIC
#if defined(__GNUC__) && !defined(_WIN32)
# pragma GCC visibility push(hidden)
#endif
#endif

// Include other internal headers
#include "singleton.h"
#include "task_wait_event.h"
#include "fifo_queue.h"
#include "work_steal_queue.h"
