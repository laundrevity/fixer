#pragma once

#include "FixSession.h"

class TestableFixSession : public FixSession {
public:
    TestableFixSession(const std::string& access_key, const std::string& access_secret,
                       const std::string& target_comp_id, const std::string& host, int port);

    TestableFixSession(const std::string& access_key, const std::string& access_secret,
               const std::string& target_comp_id, const std::string& host, int port,
               std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
};