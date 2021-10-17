#include "split.h"

std::vector<std::string> split(const std::string& str, char separator) {
    std::vector<std::string> result;
    size_t parsed = 0;

    while (1) {
        auto find_result = str.find(separator, parsed);
        if (find_result == str.npos) {
            result.push_back(str.substr(parsed));
            return result;
        } else {
            result.push_back(str.substr(parsed, find_result - parsed));
            parsed = find_result + 1;
        }
    }
}
