#include "library.h"

#include <iostream>
#include <string>

namespace {

void print_usage(const char* program) {
    std::cerr << "Usage: " << program << " search <name>\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3 || std::string(argv[1]) != "search") {
        print_usage(argv[0]);
        return 2;
    }

    const std::string query = argv[2];
    std::cout << "Searching for '" << query << "'...\n\n";

    const auto results = pkg::search(query);
    if (!results) return 1;  // fetch_url already explained why

    if (results->empty()) {
        std::cout << "No packages found matching '" << query << "'.\n";
        return 0;
    }

    int n = 0;
    for (const auto& r : *results) {
        std::cout << "  [" << ++n << "] " << r.full_name << "\n";
        if (!r.description.empty()) std::cout << "      " << r.description << "\n";
    }
    return 0;
}
