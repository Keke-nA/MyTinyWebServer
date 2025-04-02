project(timer)

# 编译 HeapTimer 类为静态库
add_library(HeapTimer STATIC HeapTimer.cpp)

# 为依赖 HeapTimer_lib 的目标提供头文件路径
target_include_directories(HeapTimer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 添加可执行文件
add_executable(testheaptimer testheaptimer.cpp)

# 链接 HeapTimer 库到可执行文件
target_link_libraries(testheaptimer PRIVATE HeapTimer)

