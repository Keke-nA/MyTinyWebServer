#include <unistd.h>
#include "server/WebServer.hpp"

int main() {
    WebServer server(5005, 3, 60000, false, 3306, "root", "root", "webserver", 12, 6, true, 1, 1024);
    server.start();
    return 0;
}