#pragma once

#include "Message.h"
#include "LogonMessage.h"
#include "NewOrderSingle.h"
#include <map>
#include <string>
#include <arpa/inet.h>
#include <queue>
#include <mutex>
#include <thread>
#include <functional>
#include <condition_variable>

typedef std::function<void(const std::map<uint32_t, std::string>& response)> ResponseCallback;


struct QueueEntry {
    std::string message;
    ResponseCallback callback;
};

class FixSession {
public:
    FixSession(const std::string& access_key, const std::string& access_secret, const std::string& target_comp_id, const std::string& host, int port);
    ~FixSession();

    bool logon();
    void send_message_and_get_reply(const std::string& message, const ResponseCallback& callback);
    void send_order(const NewOrderSingle& order);
    void process_response();
    void disconnect();
    bool connect_to_server();

private:
    std::string access_key_;
    std::string access_secret_;
    std::string target_comp_id_;
    std::string host_;
    int port_;
    int sockfd_;
    struct sockaddr_in server_addr_;
    std::queue<QueueEntry> submission_queue_;
    std::queue<QueueEntry> completion_queue_;
    std::mutex submission_mutex_;
    std::mutex completion_mutex_;
    std::thread sender_receiver_thread_;
    std::condition_variable cv_;
    bool running_;
    ssize_t seq_num_{1};

    void init_server_address();
    void sender_receiver_loop();
};