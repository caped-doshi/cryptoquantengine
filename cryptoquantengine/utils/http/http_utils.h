/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

namespace utils::http {

/*
 * https://gist.github.com/alghanmi/c5d7b761b2c9ab199157#file-curl_example-cpp
 */
static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
    ((std::string *)userp)->append(static_cast<char *>(contents), size * nmemb);
    return size * nmemb;
}

std::string http_get(const std::string &url) {
    CURL *curl = curl_easy_init();
    std::string readBuffer;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[http_get] curl_easy_perform() failed: "
                      << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}
} // namespace utils::http