# 设置项目名称
project(buffer)

# 添加库
add_library(BufferLib 
    Buffer.cpp
)

# 添加测试可执行文件
add_executable(testbufferclient testbufferclient.cpp)
add_executable(testbufferserver testbufferserver.cpp)

# 链接库
target_link_libraries(testbufferclient BufferLib)
target_link_libraries(testbufferserver BufferLib)    