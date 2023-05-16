#include "FixSession.h"
#include "LogonMessage.h"
#include "FixUtil.cpp"
#include <iostream>
#include <cstdlib>

int main() {
    std::string access_key = std::string(std::getenv("FIX_ACCESS_KEY"));
    std::string access_secret = std::string(std::getenv("FIX_ACCESS_SECRET"));
    std::string target_comp_id = std::string(std::getenv("FIX_TARGET_COMP_ID"));
    std::string host = std::string(std::getenv("FIX_ENDPOINT_HOST"));
    int port = std::stoi(std::getenv("FIX_ENDPOINT_PORT"));

    FixSession fix_session(access_key, access_secret, target_comp_id, host, port);
    if (!fix_session.connect_to_server()) {
        std::cout << "Failed to connect to server" << std::endl;
        exit(1);
    }

    if (fix_session.logon()) {
        std::cout << "Logged on successfully" << std::endl;
    } else {
        std::cout << "Logon failed" << std::endl;
    }

    fix_session.disconnect();
    return 0;
}