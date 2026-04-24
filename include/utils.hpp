#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::string base64_encode(const std::vector<uint8_t>& data);
std::vector<uint8_t> base64_decode(std::string const& encoded_string);
std::string urlEncode(const std::string& str);

#endif // UTILS_HPP
