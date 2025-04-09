# 设置项目名称
project(timer)

# 添加库
add_library(TimerLib HeapTimer.cpp)

# 添加测试可执行文件
add_executable(testheaptimer testheaptimer.cpp)

# 链接库
target_link_libraries(testheaptimer TimerLib)
