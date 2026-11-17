#include <curl/curl.h>
#include <iostream>
#include <string>
#include <algorithm>

// Callback that appends incoming HTTP response data to a std::string
size_t write_to_string(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append(static_cast<char*>(contents), total_size);
    return total_size;
}

// Fetches text/JSON from a URL into memory
std::string fetch_url(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response_body;

    if (!curl) {
        std::cerr << "Failed to initialize libcurl.\n";
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    // GitHub and package APIs require a User-Agent header
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "my-macos-pm/1.0");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Search request failed: " << curl_easy_strerror(res) << "\n";
        return "";
    }

    return response_body;
}

int main(int argc, char* argv[]) {
    // 1. Validate command-line user input
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <package-name>\n";
        return 1;
    }

    std::string query = argv[1];
    std::cout << "Searching online for '" << query << "'...\n\n";

    // 2. Query an online package index (GitHub Repositories API as a live example)
    // Note: Replace this URL with your own registry URL or GitHub raw JSON file
    std::string api_url = "https://github.com" + query + "+language:cpp&per_page=5";

    std::string json_response = fetch_url(api_url);

    if (json_response.empty()) {
        std::cerr << "No data received from registry.\n";
        return 1;
    }

    // 3. Parse results (Simple string extraction shown here; use nlohmann/json in production)
    size_t pos = 0;
    int count = 0;
    while ((pos = json_response.find("\"full_name\":\"", pos)) != std::string::npos && count < 5) {
        pos += 13; // Skip past "full_name":"
        size_t end = json_response.find("\"", pos);
        std::string pkg_name = json_response.substr(pos, end - pos);

        std::cout << "  [" << ++count << "] " << pkg_name << "\n";
        pos = end;
    }

    if (count == 0) {
        std::cout << "No packages found matching '" << query << "'.\n";
    }

    return 0;
}
