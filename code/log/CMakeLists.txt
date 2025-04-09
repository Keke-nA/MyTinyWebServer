# 设置项目名称
project(log)

# 添加库
add_library(LogLib Log.cpp)

# 添加测试可执行文件
add_executable(testblockdeque testblockdeque.cpp)
add_executable(testtime testtime.cpp)

# 链接库
target_link_libraries(testblockdeque LogLib)
target_link_libraries(testtime LogLib)
