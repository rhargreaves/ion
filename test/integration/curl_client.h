#pragma once
#include <curl/curl.h>

#include <string>

class CurlClient {
    CURL* curl_;
    std::string response_body_;

    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
        return size * nmemb;
    }

   public:
    CurlClient() {
        curl_ = curl_easy_init();
        curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body_);
    }

    ~CurlClient() {
        curl_easy_cleanup(curl_);
    }
    long get(const std::string& url) {
        response_body_.clear();
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_perform(curl_);

        long http_code = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
        return http_code;
    }

    const std::string& body() const {
        return response_body_;
    }
};
