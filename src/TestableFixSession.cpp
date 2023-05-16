#include "TestableFixSession.h"

TestableFixSession::TestableFixSession(const std::string& access_key, const std::string& access_secret,
                                       const std::string& target_comp_id, const std::string& host, int port)
        : FixSession(access_key, access_secret, target_comp_id, host, port) {}

TestableFixSession::TestableFixSession(const std::string &access_key, const std::string &access_secret,
                       const std::string &target_comp_id, const std::string &host, int port,
                       std::chrono::milliseconds timeout)
        : FixSession(access_key, access_secret, target_comp_id, host, port, timeout) {}
