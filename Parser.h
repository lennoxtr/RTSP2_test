#include <string>
#include <openssl/evp.h>

// For auth
std::string hash(const std::string& input, const EVP_MD* algorithm);
std::string sha256(const std::string& input);
std::string md5(const std::string& input);

// Parsing context, session
std::string parse_value(const std::string& text, const std::string& key);
std::string parse_sdp(const std::string& response);

// Change rtp listening port for player
std::string set_sdp_port(const std::string& sdp, int new_port);

// Get rtsp return code
int get_rtsp_status_code(const std::string& response);