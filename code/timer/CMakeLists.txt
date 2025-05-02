# 设置项目名称
project(timer)

# 添加库
add_library(TimerLib HeapTimer.cpp)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED) 

add_executable(HeapTimerUT HeapTimerUT.cpp) 
add_executable(HeapTimerUT1 HeapTimerUT1.cpp) 
target_link_libraries(HeapTimerUT 
    TimerLib  # 使用明确的库名称
    GTest::GTest 
    GTest::gtest_main
    Threads::Threads  # 使用导入的线程目标
)
target_link_libraries(HeapTimerUT1 
    TimerLib  # 使用明确的库名称
    GTest::GTest 
    GTest::gtest_main
    Threads::Threads  # 使用导入的线程目标
)

# 添加测试可执行文件
add_executable(testheaptimer testheaptimer.cpp)

# 链接库
target_link_libraries(testheaptimer TimerLib)
