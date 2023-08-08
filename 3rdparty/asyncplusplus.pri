
INCLUDEPATH += $$PWD/asyncplusplus/include

HEADERS += \
    $$PWD/asyncplusplus/include/async++.h \
    $$PWD/asyncplusplus/src/fifo_queue.h \
    $$PWD/asyncplusplus/src/internal.h \
    $$PWD/asyncplusplus/src/singleton.h \
    $$PWD/asyncplusplus/src/task_wait_event.h \
    $$PWD/asyncplusplus/src/work_steal_queue.h

SOURCES += \
    $$PWD/asyncplusplus/src/scheduler.cpp \
    $$PWD/asyncplusplus/src/threadpool_scheduler.cpp
