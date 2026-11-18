#include "library.h"

#include <curl/curl.h>

#include <iostream>

namespace pkg {

namespace {

// libcurl callback: appends each received chunk to a std::string.
size_t write_to_string(void* contents, size_t size, size_t nmemb, void* userdata) {
    const size_t total = size * nmemb;
    static_cast<std::string*>(userdata)->append(static_cast<const char*>(contents), total);
    return total;
}

// Reads the JSON string literal that starts at `pos` (just after the opening
// quote), decoding the common escapes. Advances `pos` past the closing quote.
std::string read_json_string(const std::string& json, size_t& pos) {
    std::string out;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            ++pos;
            switch (json[pos]) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'u':  // keep \uXXXX as-is; decoding is not needed for names
                    out += "\\u";
                    break;
                default: out += json[pos]; break;
            }
        } else {
            out += json[pos];
        }
        ++pos;
    }
    if (pos < json.size()) ++pos;  // closing quote
    return out;
}

// Finds `"key"` followed by a string value at or after `from`, but not past
// `limit`. Returns std::nullopt if absent or the value is not a string (null).
std::optional<std::string> find_string_field(const std::string& json, const std::string& key,
                                             size_t from, size_t limit) {
    const std::string needle = "\"" + key + "\"";
    const size_t at = json.find(needle, from);
    if (at == std::string::npos || at >= limit) return std::nullopt;

    size_t pos = at + needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) ++pos;
    if (pos >= json.size() || json[pos] != '"') return std::nullopt;
    ++pos;
    return read_json_string(json, pos);
}

}  // namespace

std::string url_escape(const std::string& text) {
    CURL* curl = curl_easy_init();
    if (!curl) return text;
    char* escaped = curl_easy_escape(curl, text.c_str(), static_cast<int>(text.size()));
    std::string out = escaped ? escaped : text;
    curl_free(escaped);
    curl_easy_cleanup(curl);
    return out;
}

std::optional<std::string> fetch_url(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialise libcurl.\n";
        return std::nullopt;
    }

    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "pkg-manager/0.1");  // GitHub requires a User-Agent

    const CURLcode res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Request failed: " << curl_easy_strerror(res);
        if (status == 403 || status == 429) std::cerr << " (GitHub rate limit; try again in a minute)";
        std::cerr << "\n";
        return std::nullopt;
    }
    return body;
}

std::string search_url(const std::string& query, int limit) {
    // GitHub's repository search: "<terms> language:cpp". The space in the
    // qualifier is sent as '+'; the user's own text is percent-encoded.
    return "https://api.github.com/search/repositories?q=" + url_escape(query) +
           "+language:cpp&per_page=" + std::to_string(limit);
}

std::vector<SearchResult> parse_search_results(const std::string& json) {
    std::vector<SearchResult> results;
    const std::string marker = "\"full_name\"";

    size_t pos = 0;
    while ((pos = json.find(marker, pos)) != std::string::npos) {
        // Each repository's fields sit between its "full_name" and the next one.
        const size_t next = json.find(marker, pos + marker.size());
        const size_t limit = next == std::string::npos ? json.size() : next;

        auto name = find_string_field(json, "full_name", pos, limit);
        if (name) {
            SearchResult r;
            r.full_name = *name;
            // "description" is a repo-level field that follows "full_name"'s
            // sibling keys; it is null (not a string) when the repo has none.
            r.description = find_string_field(json, "description", pos, limit).value_or("");
            results.push_back(std::move(r));
        }
        pos += marker.size();
    }
    return results;
}

std::optional<std::vector<SearchResult>> search(const std::string& query, int limit) {
    auto body = fetch_url(search_url(query, limit));
    if (!body) return std::nullopt;
    return parse_search_results(*body);
}

}  // namespace pkg
