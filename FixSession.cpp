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
    , sockfd_(-1) {}

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

    if (sockfd_ < 0) {
        std::cout << "Disconnected, trying to connect again...";
        if (!connect_to_server()) {
            std::cout << "Failed to connect, aborting send" << std::endl;
            return false;
        }
    }

    auto reply = send_message_and_get_reply(logon_message.to_string());

    auto response_type_it = reply.find(35);

    if (response_type_it != reply.end()) {
        return response_type_it->second == "A";
    } else {
        std::cout << "Got reply without a message type" << std::endl;
        return false;
    }

}

std::map<uint32_t, std::string> FixSession::send_message_and_get_reply(const std::string& message) {
    if (sockfd_ < 0) {
        std::cout << "Disconnected, trying to logon again...";
        if (!connect_to_server()) {
            std::cout << "Failed to reconnect, aborting send" << std::endl;
            return std::map<uint32_t, std::string>{};
        }
    }

    ssize_t bytes_sent = send(sockfd_, message.c_str(), message.size(), 0);
    if (bytes_sent != static_cast<ssize_t>(message.size())) {
        std::cerr << "Error sending message" << std::endl;
        close(sockfd_);
        return std::map<uint32_t, std::string>{};
    }

    char buffer[4096];
    ssize_t bytes_received = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::cerr << "Error receiving message: " << std::endl;
        close(sockfd_);
        return std::map<uint32_t, std::string>{};
    }

    buffer[bytes_received] = '\0';

    auto msg = ParseFixMessage(std::string(buffer));
    return msg;
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