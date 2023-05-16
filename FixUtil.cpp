#pragma once

#include <string>
#include <map>
#include <sstream>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <chrono>
#include <random>
#include <iomanip>

static std::string generate_nonce(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(33, 126);

    std::stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << static_cast<char>(dis(gen));
    }

    return ss.str();
}


static std::string base64_encode(const std::string& input) {
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

static std::string sha256_hash(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.data(), input.size());
    SHA256_Final(hash, &sha256);

    std::string binary_hash(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
    return binary_hash;
}

static int calculate_checksum(const std::string& message) {
    int sum = 0;
    for (char c : message) {
        sum += c;
    }
    return sum % 256;
}

static std::string current_utc_time() {
    auto now = std::chrono::system_clock::now();
    auto itt = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(gmtime(&itt), "%Y%m%d-%H:%M:%S");
    return ss.str();
}

static std::map<uint32_t, std::string> ParseFixMessage(const std::string& message) {
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

static std::string generate_password(const std::string& raw_data, const std::string& access_secret) {
    std::string combined = raw_data + access_secret;
    std::string hashed = sha256_hash(combined);
    return base64_encode(hashed);
}