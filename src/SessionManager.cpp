#include "SessionManager.hpp"
#include "ConsoleOutputManager.hpp"
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

using json = nlohmann::json;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    output->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string SessionManager::base64Encode(const std::string &binary)
{
    BIO *bmem = BIO_new(BIO_s_mem());
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, binary.data(), (int)binary.size());
    BIO_flush(b64);

    BUF_MEM *bptr = nullptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string result(bptr->data, bptr->length);

    BIO_free_all(b64);
    return result;
}

std::string SessionManager::base64Decode(const std::string& in) {
    // 1. 创建并配置 Base64 BIO，关闭换行检查
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    // 2. 从内存缓冲区创建 BIO，并将其压入链中
    BIO* mem = BIO_new_mem_buf(in.data(), static_cast<int>(in.size()));
    BIO* bio = BIO_push(b64, mem);

    // 3. 分配缓冲区并解码
    int max_len = static_cast<int>(in.size() * 3 / 4) + 1;
    std::vector<char> buf(max_len);
    int len = BIO_read(bio, buf.data(), max_len);

    // 4. 释放 BIO 资源
    BIO_free_all(bio);

    if (len <= 0) {
        return {};  // 解码失败或无内容
    }

    // 5. 构造字符串并去除首尾的 CR/LF
    std::string out(buf.data(), len);
    size_t first = out.find_first_not_of("\r\n");
    size_t last  = out.find_last_not_of("\r\n");
    if (first == std::string::npos) {
        return {};  // 全是回车或换行
    }
    return out.substr(first, last - first + 1);
}

std::string encrypt(const std::string &rsaPublicKeyPEM, const std::string &adminKeyPlain)
{
    BIO *bio = BIO_new_mem_buf(rsaPublicKeyPEM.data(), (int)rsaPublicKeyPEM.size());
    if (!bio)
        buffer("无法创建 BIO", MessageType::Error);

    EVP_PKEY *pubkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pubkey)
        buffer("无法读取公钥 PEM", MessageType::Error);

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pubkey, nullptr);
    if (!ctx)
        buffer("无法创建加密上下文", MessageType::Error);

    if (EVP_PKEY_encrypt_init(ctx) <= 0)
        buffer("加密初始化失败", MessageType::Error);

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0)
        buffer("设置填充失败", MessageType::Error);

    size_t outlen = 0;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outlen,
                         (const unsigned char *)adminKeyPlain.data(),
                         adminKeyPlain.size()) <= 0)
        buffer("计算加密长度失败", MessageType::Error);

    std::string encrypted(outlen, '\0');
    if (EVP_PKEY_encrypt(ctx, (unsigned char *)encrypted.data(), &outlen,
                         (const unsigned char *)adminKeyPlain.data(),
                         adminKeyPlain.size()) <= 0)
        buffer("加密失败", MessageType::Error);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pubkey);

    encrypted.resize(outlen);
    return SessionManager::base64Encode(encrypted);
}

json SessionManager::createSession(const std::string &serverUrl)
{
    CURL *curl = curl_easy_init();

    if (!curl)
    {
        buffer("无法初始化 CURL", MessageType::Error);
    }

    std::string url = serverUrl + "/muip/create_session";
    json requestBody = {
        {"key_type", "PEM"}};
    std::string postData = requestBody.dump();
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string responseStr;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);

    CURLcode res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK)
        buffer("创建会话请求失败: " + std::string(curl_easy_strerror(res)), MessageType::Error);
    return json::parse(responseStr);
}

json SessionManager::authorize(const std::string &serverUrl, const std::string &sessionId, const std::string &rsaPublicKeyPEM, const std::string &adminKeyPlain)
{
    std::string encryptedKey = encrypt(rsaPublicKeyPEM, adminKeyPlain);

    json requestBody = {
        {"session_id", sessionId},
        {"admin_key", encryptedKey}};

    CURL *curl = curl_easy_init();
    if (!curl)
        buffer("无法初始化 CURL", MessageType::Error);
    std::string url = serverUrl + "/muip/auth_admin";
    std::string postData = requestBody.dump();
    std::string responseStr;

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        buffer("授权请求失败: " + std::string(curl_easy_strerror(res)), MessageType::Error);

    return json::parse(responseStr);
}

json SessionManager::getServerStatus(const std::string &serverUrl, const std::string &sessionId)
{
    json requestBody = {
        {"SessionId", sessionId},
    };
    CURL *curl = curl_easy_init();
    if (!curl)
        buffer("无法初始化 CURL", MessageType::Error);

    std::string url = serverUrl + "/muip/server_information";
    std::string postData = requestBody.dump();
    std::string responseStr;
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        buffer("获取服务器状态失败: " + std::string(curl_easy_strerror(res)), MessageType::Error);
    return json::parse(responseStr);
}

json SessionManager::getPlayerInfo(const std::string &serverUrl, const std::string &sessionId, const std::string &playerUid)
{
    json requestBody = {
        {"SessionId", sessionId},
        {"Uid", playerUid}
    };
    CURL *curl = curl_easy_init();
    if (!curl)
        buffer("无法初始化 CURL", MessageType::Error);

    std::string url = serverUrl + "/muip/player_information";
    std::string postData = requestBody.dump();
    std::string responseStr;
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        buffer("获取玩家信息失败: " + std::string(curl_easy_strerror(res)), MessageType::Error);
    return json::parse(responseStr);
}

json SessionManager::submitCommand(const std::string &serverUrl, const std::string &sessionId, const std::string &rsaPublicKeyPEM, const std::string &commandPlain, const std::string &targetUid)
{
    std::string encryptedCommand = encrypt(rsaPublicKeyPEM, commandPlain);

    json requestBody = {
        {"SessionId", sessionId},
        {"Command", encryptedCommand},
        {"TargetUid", targetUid}};

    CURL *curl = curl_easy_init();
    if (!curl)
        buffer("无法初始化 CURL", MessageType::Error);

    std::string url = serverUrl + "/muip/exec_cmd";
    std::string postData = requestBody.dump();
    std::string responseStr;
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseStr);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        buffer("命令提交失败: " + std::string(curl_easy_strerror(res)), MessageType::Error);
    return json::parse(responseStr);
}