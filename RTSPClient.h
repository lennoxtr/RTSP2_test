#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

class RTSPClient
{
public:
    RTSPClient(std::string serverIP, int port, std::string rtspURL, std::string username, std::string password);

    int initialize_socket();
    int terminate_socket();
    int initiate_handshake();

    int print_context()
    {
        std::cout << "realm: " << this->realm << std::endl;
        std::cout << "nonce: " << this->nonce << std::endl;
        std::cout << "algo : " << this->algo << std::endl;
        std::cout << "session id : " << this->sessionId << std::endl;
        return 0;
    }

private:

    // For TCP, UDP connection
    std::string serverIP;
    int port;
    std::string rtspURL;
    SOCKET rtsp_socket;
    SOCKET rtp_socket;
    SOCKET rtcp_socket;
    int Cseq_num = 1;

    // For Auth
    std::string username;
    std::string password;
    std::string realm;
    std::string nonce;
    std::string algo;

    // For playing stream
    std::string sessionId;

    // build RTSP handshake payload
    std::string build_option_request();
    std::string build_describe_request(int elevated_request = 0);
    std::string build_setup_request();
    std::string build_play_request();

    std::string build_sha_response(std::string method, std::string media_url);

    // send request
    int send_request(std::string request_payload, char* buffer, int buffer_size);

    // parsing context, session from response
    int parse_context(const std::string& server_response);
    std::string extract_session(const std::string& response);
};

