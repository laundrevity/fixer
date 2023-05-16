#include <gtest/gtest.h>
#include "../src/TestableFixSession.h"
#include <string>


TEST(FixMessageTests, LogonMessageSendTest) {
    std::string access_key = std::string(std::getenv("FIX_ACCESS_KEY"));
    std::string access_secret = std::string(std::getenv("FIX_ACCESS_SECRET"));
    std::string target_comp_id = std::string(std::getenv("FIX_TARGET_COMP_ID"));
    std::string host = std::string(std::getenv("FIX_ENDPOINT_HOST"));
    int port = std::stoi(std::getenv("FIX_ENDPOINT_PORT"));

    TestableFixSession fix_session(access_key, access_secret, target_comp_id, host, port, std::chrono::seconds(5));

    if (!fix_session.connect_to_server()) {
        std::cout << "Failed to connect to server" << std::endl;
        exit(1);
    }

    EXPECT_TRUE(fix_session.logon());
    fix_session.disconnect();
}