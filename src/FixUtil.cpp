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
#include <regex>


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

#if OPENSSL_VERSION_NUMBER < 0x30000000L
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.data(), input.size());
    SHA256_Final(hash, &sha256);
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();
#else
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();

    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, input.data(), input.size());
    EVP_DigestFinal_ex(mdctx, hash, NULL);
    EVP_MD_CTX_free(mdctx);
#endif

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

static std::map<uint32_t, std::string> parse_fix_message(const std::string& message) {
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

static std::string set_message_sequence_number(int seq_num, const std::string& message) {
    std::string seq_num_field = "34=" + std::to_string(seq_num) + '\x01';
    std::regex seq_num_regex("34=[0-9]+\x01");
    return std::regex_replace(message, seq_num_regex, seq_num_field);
}