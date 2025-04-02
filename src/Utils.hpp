#pragma once

#include <sstream>
#include <string>

using std::string;

namespace Utils {

	inline string to_string(int n) {
        std::ostringstream oss;
        oss << n;
        return oss.str();
    }

	char *binToHex(const unsigned char *input, size_t len) ;
	string trim(const string &str);

} // UTILS_HPP