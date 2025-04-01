project(pool)

# 收集库文件源
set(POOL_LIB_SRCS SqlConnPool.cpp)

# 构建库并链接 MySQL 库
add_library(sqlconnpool ${POOL_LIB_SRCS})
target_include_directories(sqlconnpool PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sqlconnpool PRIVATE mysqlclient) # 新增链接 MySQL 库

# 构建测试可执行文件并链接
add_executable(sqlconnpool_test sqlconnpooltest.cpp)
target_link_libraries(sqlconnpool_test PRIVATE sqlconnpool mysqlclient
)# 测试程序也需链接
