#include "FixSession.h"
#include "FixUtil.cpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "LogonMessage.h"
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <cstring>

FixSession::FixSession(const std::string &access_key, const std::string &access_secret,
                       const std::string &target_comp_id, const std::string &host, int port)
    : access_key_(access_key)
    , access_secret_(access_secret)
    , target_comp_id_(target_comp_id)
    , host_(host)
    , port_(port)
    , sockfd_(-1)
    , running_(true) {
    sender_receiver_thread_ = std::thread(&FixSession::sender_receiver_loop, this);
}

FixSession::~FixSession() {
    running_ = false;
    cv_.notify_all();
    sender_receiver_thread_.join();
    disconnect();
}

bool FixSession::logon() {
    size_t nonce_length = 32;
    std::string nonce = generate_nonce(nonce_length);
    std::string base64_nonce = base64_encode(nonce);

    std::time_t timestamp = std::time(nullptr);
    std::stringstream raw_data_ss;
    raw_data_ss << timestamp << "." << base64_nonce;
    std::string raw_data = raw_data_ss.str();

    const auto logon_message = LogonMessage(
            std::getenv("FIX_ACCESS_KEY"),
            std::getenv("FIX_ACCESS_SECRET"),
            std::getenv("FIX_TARGET_COMP_ID"),
            raw_data
    );

    bool logon_success = false;
    std::mutex logon_mutex;
    std::condition_variable logon_cv;

    send_message_and_get_reply(logon_message.to_string(), [&](const std::map<uint32_t, std::string>& response) {
        auto response_type_it = response.find(35);
        if (response_type_it != response.end()) {
            logon_success = response_type_it->second == "A";
        }
        std::unique_lock<std::mutex> lock(logon_mutex);
        logon_cv.notify_all();
    });
    ++seq_num_;
    std::unique_lock<std::mutex> lock(logon_mutex);
    logon_cv.wait(lock);

    return true;

}

void FixSession::send_message_and_get_reply(const std::string& message, const ResponseCallback& callback) {
    std::unique_lock<std::mutex> lock(submission_mutex_);

    // Set the sequence number for the message
    std::cout << "Submitting message to queue: " << message << std::endl;
    submission_queue_.push(QueueEntry{message, callback});
    cv_.notify_all();
}

void FixSession::send_order(const NewOrderSingle& order) {
    send_message_and_get_reply(order.to_string(), [&](const std::map<uint32_t, std::string>& response) {
        // Handle the response here
        auto it = response.begin();
        while (it != response.end()) {
            std::cout << it->first << "=" << it->second << std::endl;
            ++it;
        }
    });
}

bool FixSession::connect_to_server() {
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }

    init_server_address();

    if (connect(sockfd_, reinterpret_cast<struct sockaddr *>(&server_addr_), sizeof(server_addr_)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        close(sockfd_);
        return false;
    }

    return true;
}

void FixSession::disconnect() {
    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }
}

void FixSession::init_server_address() {
    memset(&server_addr_, 0, sizeof(server_addr_));
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(port_);

    struct hostent *dns_host = gethostbyname(host_.c_str());
    if (!dns_host) {
        std::cerr << "Error resolving hostname" << std::endl;
        close(sockfd_);
        return;
    }
    memcpy(&server_addr_.sin_addr, dns_host->h_addr, dns_host->h_length);
}

void FixSession::sender_receiver_loop() {
    while (running_) {
        std::unique_lock<std::mutex> submission_lock(submission_mutex_);
        cv_.wait(submission_lock, [this]() { return !submission_queue_.empty() || !running_; });

        if (!running_) {
            break;
        }

        auto entry = submission_queue_.front();
        submission_queue_.pop();
        submission_lock.unlock();

        std::cout << "Sending message: " << entry.message << std::endl;

        // Send the message over the socket
        std::string message = entry.message;
        ssize_t sent = send(sockfd_, message.c_str(), message.size(), 0);

        if (sent < 0) {
            std::cerr << "Error sending message" << std::endl;
        } else {
            // Read the response from the server
            char buffer[4096];
            ssize_t received = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
            if (received > 0) {
                buffer[received] = '\0';
                std::string response_str(buffer);
                std::cout << "Received response: " << response_str << std::endl;

                auto parsed_response = parse_fix_message(response_str);

                // Add the received message to the completion queue
                std::unique_lock<std::mutex> completion_lock(completion_mutex_);
                completion_queue_.push({response_str, entry.callback});
                completion_lock.unlock();

                // Execute the callback
                entry.callback(parsed_response);
            } else {
                std::cerr << "Error receiving message" << std::endl;
            }
        }
    }
}

void FixSession::process_response() {
    std::cout << "got response, but unclear how to access it, I guess pop it from completion_queue?" << std::endl;
}
