#pragma once

#include "Message.h"
#include "LogonMessage.h"
#include <map>
#include <string>
#include <arpa/inet.h>

class FixSession {
public:
    FixSession(const std::string& access_key, const std::string& access_secret, const std::string& target_comp_id, const std::string& host, int port);

    bool logon();
    bool connect_to_server();
    void disconnect();
    std::map<uint32_t, std::string> send_message_and_get_reply(const std::string& message);

private:
    std::string access_key_;
    std::string access_secret_;
    std::string target_comp_id_;
    std::string host_;
    int port_;
    int sockfd_;
    struct sockaddr_in server_addr_;

    void init_server_address();
};