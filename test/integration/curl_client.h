#pragma once
#include <curl/curl.h>

#include <map>
#include <string>

struct CurlResult {
    uint16_t status_code;
    std::string body;
    std::map<std::string, std::string> headers;

    void reset() {
        status_code = 0;
        body.clear();
        headers.clear();
    }
};

class CurlClient {
    CURL* curl_;
    CurlResult curl_result_{};

    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
        return size * nmemb;
    }

    static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
        const size_t total_size = size * nitems;
        auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);

        std::string header(buffer, total_size);
        const size_t colon_pos = header.find(':');
        if (colon_pos != std::string::npos) {
            const std::string key = header.substr(0, colon_pos);
            std::string value = header.substr(colon_pos + 1);

            // Trim whitespace from value
            size_t value_start = value.find_first_not_of(" \t\r\n");
            size_t value_end = value.find_last_not_of(" \t\r\n");
            if (value_start != std::string::npos && value_end != std::string::npos) {
                value = value.substr(value_start, value_end - value_start + 1);
            }

            (*headers)[key] = value;
        }

        return total_size;
    }

   public:
    CurlClient() {
        curl_ = curl_easy_init();
        curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &curl_result_.body);
        curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &curl_result_.headers);
    }

    ~CurlClient() {
        curl_easy_cleanup(curl_);
    }

    CurlResult get(const std::string& url) {
        curl_result_.reset();

        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        const auto res = curl_easy_perform(curl_);
        if (res != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &curl_result_.status_code);
        return curl_result_;
    }

    CurlResult head(const std::string& url) {
        curl_result_.reset();

        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_NOBODY, 1L);
        const auto res = curl_easy_perform(curl_);
        if (res != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &curl_result_.status_code);
        return curl_result_;
    }
};
