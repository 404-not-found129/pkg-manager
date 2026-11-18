#ifndef PKG_MANAGER_LIBRARY_H
#define PKG_MANAGER_LIBRARY_H

#include <optional>
#include <string>
#include <vector>

namespace pkg {

struct SearchResult {
    std::string full_name;    // "owner/repo"
    std::string description;  // may be empty
};

// Percent-encodes a string for use inside a URL query.
std::string url_escape(const std::string& text);

// GET a URL and return the response body, or std::nullopt on failure
// (the reason is written to stderr).
std::optional<std::string> fetch_url(const std::string& url);

// Builds the GitHub repository-search URL for a query.
std::string search_url(const std::string& query, int limit);

// Extracts the repositories from a GitHub search API JSON response.
std::vector<SearchResult> parse_search_results(const std::string& json);

// Searches GitHub for C++ repositories matching `query`.
// Returns std::nullopt if the request failed.
std::optional<std::vector<SearchResult>> search(const std::string& query, int limit = 5);

}  // namespace pkg

#endif  // PKG_MANAGER_LIBRARY_H
