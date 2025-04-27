#include "Utils.hpp"
#include <cstring>
#include <cstdlib> 
#include <sstream>

using std::string;

namespace Utils {

/**
 * @brief [Debug func] Convert a binary buffer to a hex string
 *
 * @param input The binary buffer to convert
 * @param len The length of the buffer
 *
 * In input I have an array of unsigned chars, binary code like
 *   const unsigned char data[] = {0xDE, 0xAD, 0xBE, 0xEF};
 * As input I do not need to pass a null terminated string, it supports
 binary
 * data Good for debugging. A binary buffer can be also of type uint8_t in
 * output I will have the same but in hex like "DE AD BE EF" The function
 return
 * a string which will need to be freed!
 */
char *binToHex(const unsigned char *input, size_t len) {
  char *result;

  if (input == NULL || len <= 0) {
    return (NULL);
  }

  // (2 hexits+space/chr + NULL
  size_t resultlen = (len * 3) + 1;
  result = new char[resultlen];
  std::memset(result, 0, resultlen);

  for (size_t i = 0; i < len; i++) {
    result[i * 3] = "0123456789ABCDEF"[input[i] >> 4];
    result[(i * 3) + 1] = "0123456789ABCDEF"[input[i] & 0x0F];
    result[(i * 3) + 2] = ' '; // for readability
  }
  return (result);
}

/**
 * @brief Trim leading and trailing whitespace from a string
 *
 * @param str The string to trim
 * @return A new string with leading and trailing whitespace removed
 *
 * This function removes leading and trailing whitespace characters from the
 * input string. It uses find_first_not_of and find_last_not_of to locate the
 * first and last non-whitespace characters, respectively.
 */
string trim(const string &str) {
  string trimmed = str;
  string whitespaces = " \r\n\t";
  size_t start = trimmed.find_first_not_of(whitespaces);
  if (start == string::npos) {
    return "";
  }
  size_t end = trimmed.find_last_not_of(whitespaces);
  return trimmed.substr(start, end - start + 1);
}

string ensureTrailinSlash(string path) {
  if (!path.empty() && *path.rbegin() != '/') {
    path += '/';
  }
  return path;
}

string removeLeadingSlash(string path) {
  if (!path.empty() && *path.begin() == '/') {
    path.erase(0, 1);
  }
  return path;
}

/**
 * @brief Generates a filename with a prefix and a random number (C++98 compatible).
 * @param prefix The prefix for the filename (e.g., "upload_").
 * @return A string like "prefix_12345".
 * @note Requires srand() to be called once at program startup for varied results.
 */
string generateRandomFilename(const string& prefix) {
  // Generate a pseudo-random integer
  int random_number = std::rand();
  // Convert the integer to a string using stringstream
  std::stringstream ss;
  ss << random_number;
  std::string number_str = ss.str();
  return prefix + number_str;
}

} // namespace Utils
