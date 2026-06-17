enable_testing()

add_executable(LockfreeQueueTests LockfreeTest.cpp)

target_link_libraries(LockfreeQueueTests
    gtest_main
    gtest
    LockfreeQueue
)
