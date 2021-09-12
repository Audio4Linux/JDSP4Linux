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

namespace async {
namespace detail {

// Thread-safe singleton wrapper class
#ifdef HAVE_THREAD_SAFE_STATIC
// C++11 guarantees thread safety for static initialization
template<typename T>
class singleton {
public:
	static T& get_instance()
	{
		static T instance;
		return instance;
	}
};
#else
// Some compilers don't support thread-safe static initialization, so emulate it
template<typename T>
class singleton {
	std::mutex lock;
	std::atomic<bool> init_flag;
	typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type storage;

	static singleton instance;

	// Use a destructor instead of atexit() because the latter does not work
	// properly when the singleton is in a library that is unloaded.
	~singleton()
	{
		if (init_flag.load(std::memory_order_acquire))
			reinterpret_cast<T*>(&storage)->~T();
	}

public:
	static T& get_instance()
	{
		T* ptr = reinterpret_cast<T*>(&instance.storage);
		if (!instance.init_flag.load(std::memory_order_acquire)) {
			std::lock_guard<std::mutex> locked(instance.lock);
			if (!instance.init_flag.load(std::memory_order_relaxed)) {
				new(ptr) T;
				instance.init_flag.store(true, std::memory_order_release);
			}
		}
		return *ptr;
	}
};

template<typename T> singleton<T> singleton<T>::instance;
#endif

} // namespace detail
} // namespace async
