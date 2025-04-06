#pragma once

#include <assert.h>
#include "SqlConnPool.hpp"

class SqlConnRAII {
   public:
    SqlConnRAII(MYSQL** sql, SqlConnPool* connpool) {
        assert(connpool);
        *sql = connpool->getConn();
        raii_sql = *sql;
        raii_connpoool = connpool;
    }

    ~SqlConnRAII() {
        if (raii_sql) {
            raii_connpoool->freeConn(raii_sql);
        }
    }

   private:
    MYSQL* raii_sql;
    SqlConnPool* raii_connpoool;
};