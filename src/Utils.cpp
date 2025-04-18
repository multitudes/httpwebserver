#include "Utils.hpp"
#include <cstring>

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

} // namespace Utils
