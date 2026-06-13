#include <string>
#include <openssl/evp.h>

// For auth
std::string hash(const std::string& input, const EVP_MD* algorithm);
std::string sha256(const std::string& input);
std::string md5(const std::string& input);

// Parsing context, session
std::string parse_key_value(const std::string& text, const std::string& key);
std::string parse_sdp(const std::string& response);

// Get rtsp return code
int parse_rtsp_status_code(const std::string& response);