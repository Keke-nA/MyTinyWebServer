project(buffer)

# 编译 buffer 类为静态库
add_library(Buffer STATIC Buffer.cpp)

# 为依赖 buffer_lib 的目标提供头文件路径
target_include_directories(Buffer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 添加可执行文件
add_executable(testbufferserver testbufferserver.cpp)
add_executable(testbufferclient testbufferclient.cpp)

# 链接 buffer 库到可执行文件
target_link_libraries(testbufferserver PRIVATE Buffer)
target_link_libraries(testbufferclient PRIVATE Buffer)
