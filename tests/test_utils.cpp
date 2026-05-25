#include <doctest/doctest.h>
#include "utils.hpp"

TEST_CASE("Testing Base64 encoding and decoding") {
    std::vector<uint8_t> data = { 'H', 'e', 'l', 'l', 'o' };
    std::string encoded = base64_encode(data);
    CHECK(encoded == "SGVsbG8=");
    
    std::vector<uint8_t> decoded = base64_decode(encoded);
    CHECK(decoded == data);
}

TEST_CASE("Testing URL encoding") {
    CHECK(urlEncode("F1 Race Track") == "F1%20Race%20Track");
    CHECK(urlEncode("SimpleTrack") == "SimpleTrack");
}
