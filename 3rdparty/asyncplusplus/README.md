Async++
=======

Async++ is a lightweight concurrency framework for C++11. The concept was inspired by the [Microsoft PPL library](http://msdn.microsoft.com/en-us/library/dd492418.aspx) and the [N3428](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3428.pdf) C++ standard proposal.

Example
-------
Here is a short example which shows some features of Async++:

```c++
#include <iostream>
#include <async++.h>

int main()
{
    auto task1 = async::spawn([] {
        std::cout << "Task 1 executes asynchronously" << std::endl;
    });
    auto task2 = async::spawn([]() -> int {
        std::cout << "Task 2 executes in parallel with task 1" << std::endl;
        return 42;
    });
    auto task3 = task2.then([](int value) -> int {
        std::cout << "Task 3 executes after task 2, which returned "
                  << value << std::endl;
        return value * 3;
    });
    auto task4 = async::when_all(task1, task3);
    auto task5 = task4.then([](std::tuple<async::task<void>,
                                          async::task<int>> results) {
        std::cout << "Task 5 executes after tasks 1 and 3. Task 3 returned "
                  << std::get<1>(results).get() << std::endl;
    });

    task5.get();
    std::cout << "Task 5 has completed" << std::endl;

    async::parallel_invoke([] {
        std::cout << "This is executed in parallel..." << std::endl;
    }, [] {
        std::cout << "with this" << std::endl;
    });

    async::parallel_for(async::irange(0, 5), [](int x) {
        std::cout << x;
    });
    std::cout << std::endl;

    int r = async::parallel_reduce({1, 2, 3, 4}, 0, [](int x, int y) {
        return x + y;
    });
    std::cout << "The sum of {1, 2, 3, 4} is " << r << std::endl;
}

// Output (order may vary in some places):
// Task 1 executes asynchronously
// Task 2 executes in parallel with task 1
// Task 3 executes after task 2, which returned 42
// Task 5 executes after tasks 1 and 3. Task 3 returned 126
// Task 5 has completed
// This is executed in parallel...
// with this
// 01234
// The sum of {1, 2, 3, 4} is 10
```

Supported Platforms
-------------------

The only requirement to use Async++ is a C++11 compiler and standard library. Unfortunately C++11 is not yet fully implemented on most platforms. Here is the list of OS and compiler combinations which are known to work.

- Linux: Works with GCC 4.7+, Clang 3.2+ and Intel compiler 15+.
- Mac: Works with Apple Clang (using libc++). GCC also works but you must get a recent version (4.7+).
- iOS: Works with Apple Clang (using libc++). Note: because iOS has no thread local support, the library uses a workaround based on pthreads.
- Windows: Works with GCC 4.8+ (with pthread-win32) and Visual Studio 2013+.

Building and Installing
-----------------------
Instructions for compiling Async++ and using it in your code are available on the [Building and Installing](https://github.com/Amanieu/asyncplusplus/wiki/Building-and-Installing) page.

Documentation
------------
The Async++ documentation is split into four parts:
- [Tasks](https://github.com/Amanieu/asyncplusplus/wiki/Tasks): This describes task objects which are the core Async++. Reading this first is strongly recommended.
- [Parallel algorithms](https://github.com/Amanieu/asyncplusplus/wiki/Parallel-algorithms): This describes functions to run work on ranges in parallel.
- [Schedulers](https://github.com/Amanieu/asyncplusplus/wiki/Schedulers): This describes the low-level details of Async++ and how to customize it.
- [API Reference](https://github.com/Amanieu/asyncplusplus/wiki/API-Reference): This gives detailed descriptions of all the classes and functions available in Async++.

Contact
-------
You can contact me by email at amanieu@gmail.com.
