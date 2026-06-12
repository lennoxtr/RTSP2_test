#include <string>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

std::string hash(
    const std::string& input,
    const EVP_MD* algorithm)
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    EVP_DigestInit_ex(ctx, algorithm, nullptr);

    EVP_DigestUpdate(
        ctx,
        input.c_str(),
        input.size()
    );

    EVP_DigestFinal_ex(
        ctx,
        digest,
        &digest_len
    );

    EVP_MD_CTX_free(ctx);

    std::stringstream ss;

    for (unsigned int i = 0;
        i < digest_len;
        ++i)
    {
        ss << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(digest[i]);
    }

    return ss.str();
}

std::string md5(const std::string& input)
{
    return hash(input, EVP_md5());
}

std::string sha256(const std::string& input)
{
    return hash(input, EVP_sha256());
}

std::string parse_value(const std::string& text,
    const std::string& key)
{
    size_t pos = text.find(key);
    if (pos == std::string::npos)
        return "";

    pos += key.length();

    // skip spaces and '='
    while (pos < text.size() &&
        (text[pos] == ' ' || text[pos] == '=' || text[pos] == '"'))
    {
        pos++;
    }

    size_t end = pos;

    while (end < text.size() &&
        text[end] != '"' &&
        text[end] != ',' &&
        text[end] != '\r' &&
        text[end] != '\n')
    {
        end++;
    }

    return text.substr(pos, end - pos);
}


std::string parse_sdp(const std::string& response)
{
    size_t pos = response.find("\r\n\r\n");

    if (pos == std::string::npos)
        return "";

    return response.substr(pos + 4);
}

std::string set_sdp_port(const std::string& sdp, int new_port)
{
    size_t mpos = sdp.find("m=video");
    if (mpos == std::string::npos)
        return sdp;

    // 3. Find first space after "m=video"
    size_t first_space = sdp.find(' ', mpos);
    if (first_space == std::string::npos)
        return sdp;

    // 4. Find second space (end of port field)
    size_t second_space = sdp.find(' ', first_space + 1);
    if (second_space == std::string::npos)
        return sdp;

    // 5. Replace port
    std::string before = sdp.substr(0, first_space + 1);
    std::string after = sdp.substr(second_space);

    return before + std::to_string(new_port) + after;
}