# 设置项目名称
project(ServerSubProject)

# 添加库
add_library(ServerLib Epoller.cpp WebServer.cpp)

# 包含头文件目录
target_include_directories(ServerLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 新增：链接 HttpLib、LogLib、PoolLib 库
target_link_libraries(ServerLib PUBLIC HttpLib LogLib PoolLib)
