find_package(Qt5 5.6.0 REQUIRED
    COMPONENTS
        Concurrent
        Test
)

include(QtPromiseAddTest)

add_subdirectory(auto)
