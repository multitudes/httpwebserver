#pragma once

#include <sstream>
#include <string>

using std::string;

/**
 * @brief Utility functions for string manipulation and conversion
 *
 * The inline functions are our own implementations of to_string for int,
 * long, and size_t. The original was overloaded too. this avoids unnecessary
 * castings
 */
namespace Utils {

inline string to_string(int n) {
  std::ostringstream oss;
  oss << n;
  return oss.str();
}

inline string to_string(long n) {
  std::ostringstream oss;
  oss << n;
  return oss.str();
}

inline string to_string(size_t n) {
  std::ostringstream oss;
  oss << n;
  return oss.str();
}

char *binToHex(const unsigned char *input, size_t len);
string trim(const string &str);
string ensureTrailinSlash(string path);
string removeLeadingSlash(string path);
string generateRandomFilename(const string& prefix);

} // namespace Utils