#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

class RTSPClient
{
public:
    RTSPClient(std::string server_ip, int server_port, std::string stream_description, std::string rtsp_url, std::string username, std::string password);

    int initialize_socket();
    int terminate_socket();
    int initiate_handshake();
   
    std::string get_sdp_filename()
    {
        return this->sdp_filename;
    }

    std::string get_stream_description()
    {
        return this->stream_description;
    }

    int print_context()
    {
        std::cout << "realm: " << this->realm << std::endl;
        std::cout << "nonce: " << this->nonce << std::endl;
        std::cout << "algo : " << this->algo << std::endl;
        std::cout << "session id : " << this->session_id << std::endl;
        return 0;
    }

private:
    // For distinguishing streams
    std::string stream_description;

    // For TCP connection
    std::string server_ip;
    int server_port;
    std::string rtsp_url;
    SOCKET rtsp_socket;

    // For Auth
    std::string username;
    std::string password;
    std::string realm;
    std::string nonce;
    std::string algo;
    int Cseq_num = 1;

    // For method control
    std::string stream_control;
    std::string session_id;
    std::string sdp_filename;

    // build RTSP handshake payload
    std::string build_option_request();
    std::string build_describe_request(bool is_elevated_request = 0);
    std::string build_setup_request(const std::string& stream_control, int rtp_port, int rtcp_port);
    std::string build_play_request();
    std::string build_auth_response(std::string control_method, std::string media_url);

    // For video player
    std::string set_player_port(const std::string& sdp_from_server, int new_port);

    // send request
    int send_request(std::string request_payload, char* buffer, int buffer_size);

    // parsing context, session from response
    int get_auth_context(const std::string& server_response);
    int get_session_id(const std::string& server_response);
    int get_stream_control(const std::string& sdp_from_server);

    // allocate rtp, rtcp port
    // rtcp port = rtp port + 1
    int get_rtp_port();
};

