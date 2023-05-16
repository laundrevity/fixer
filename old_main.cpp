#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>
#include <chrono>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <map>

std::map<uint32_t, std::string> parse_fix_message(const std::string& message) {
    std::map<uint32_t, std::string> parsed_message;

    std::stringstream ss(message);
    std::string field;
    while (std::getline(ss, field, '\x01')) {
        uint32_t key;
        std::string value;
        size_t delimiter_position = field.find('=');
        if (delimiter_position != std::string::npos) {
            key = std::stoi(field.substr(0, delimiter_position));
            value = field.substr(delimiter_position + 1);
            parsed_message[key] = value;
        }
    }

    return parsed_message;
}


std::map<uint32_t, std::string> send_and_get_reply(const std::string& message) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return std::map<uint32_t, std::string>{};
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(std::stoi(std::getenv("FIX_ENDPOINT_PORT")));


    struct hostent *host = gethostbyname(std::getenv("FIX_ENDPOINT_HOST"));
    if (!host) {
        std::cerr << "Error resolving hostname" << std::endl;
        close(sockfd);
        return std::map<uint32_t, std::string>{};
    }
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to server" << std::endl;
        close(sockfd);
        return std::map<uint32_t, std::string>{};;
    }

    ssize_t bytes_sent = send(sockfd, message.c_str(), message.size(), 0);
    if (bytes_sent != static_cast<ssize_t>(message.size())) {
        std::cerr << "Error sending message" << std::endl;
        close(sockfd);
        return std::map<uint32_t, std::string>{};;
    }

    char buffer[4096];
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::cerr << "Error receiving message" << std::endl;
        close(sockfd);
        return std::map<uint32_t, std::string>{};;
    }

    buffer[bytes_received] = '\0';
    std::cout << "Received message: " << buffer << std::endl;

    auto msg = parse_fix_message(std::string(buffer));
    close(sockfd);
    return msg;
}

std::string generate_nonce(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(33, 126);

    std::stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << static_cast<char>(dis(gen));
    }

    return ss.str();
}

std::string base64_encode(const std::string& input) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);

    BIO_write(b64, input.data(), input.size());
    BIO_flush(b64);

    BUF_MEM* buffer_ptr;
    BIO_get_mem_ptr(b64, &buffer_ptr);

    std::string result(buffer_ptr->data, buffer_ptr->length);

    BIO_free_all(b64);

    return result;
}

std::string sha256_hash(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.data(), input.size());
    SHA256_Final(hash, &sha256);

    std::string binary_hash(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
    return binary_hash;
}

int calculate_checksum(const std::string& message) {
    int sum = 0;
    for (char c : message) {
        sum += c;
    }
    return sum % 256;
}

std::string build_logon_message(const std::string& header, const std::string& body) {
    std::stringstream logon_message;

    // add header and body to logon message
    logon_message << header << body;

    // calculate and add checksum
    int checksum = calculate_checksum(logon_message.str());
    logon_message << "10=" << std::setw(3) << std::setfill('0') << checksum << '\x01';

    return logon_message.str();
}

std::string generate_password(const std::string& raw_data, const std::string& access_secret) {
    std::string combined = raw_data + access_secret;
    std::string hashed = sha256_hash(combined);
    return base64_encode(hashed);
}

std::string current_utc_time() {
    auto now = std::chrono::system_clock::now();
    auto itt = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(gmtime(&itt), "%Y%m%d-%H:%M:%S");
    return ss.str();
}

int main() {
    std::string access_key = std::string(std::getenv("FIX_ACCESS_KEY"));
    std::string access_secret = std::string(std::getenv("FIX_ACCESS_SECRET"));

    size_t nonce_length = 32;
    std::string nonce = generate_nonce(nonce_length);
    std::string base64_nonce = base64_encode(nonce);

    std::time_t timestamp = std::time(nullptr);
    std::stringstream raw_data_ss;
    raw_data_ss << timestamp << "." << base64_nonce;
    std::string raw_data = raw_data_ss.str();
    std::string sending_time = current_utc_time();

    std::string password = generate_password(raw_data, access_secret);

    std::stringstream header;
    header << "8=FIX.4.4" << '\x01' << "9=";  // BeginString and placeholder for BodyLength

    std::string target_comp_id = std::string(std::getenv("FIX_TARGET_COMP_ID"));
    std::stringstream body;
    body << "35=A" << '\x01' << "49=client" << '\x01' << "56=" << target_comp_id << '\x01' << "34=1" << '\x01' << "52=" << sending_time << '\x01'
        << "98=0" << '\x01'
        << "108=30" << '\x01'
        << "141=Y" << '\x01'
        << "96=" << raw_data << '\x01' << "553=" << access_key << '\x01' << "554=" << password << '\x01';

    std::string body_string = body.str();
    int body_length = body_string.size();

    // Insert BodyLength into the header
    header << std::setw(3) << std::setfill('0') << body_length << '\x01';

    std::string logon_message_str = build_logon_message(header.str(), body.str());
    auto reply = parse_fix_message(logon_message_str);

    std::cout << "Got reply: " << std::endl;
    auto it = reply.begin();
    while (it != reply.end()) {
        std::cout << it->first << ": " << it->second << std::endl;
        ++it;
    }

    return 0;
}