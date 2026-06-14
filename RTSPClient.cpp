#include "RTSPClient.h"
#include "Parser.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")


RTSPClient::RTSPClient(std::string server_ip, int server_port, std::string stream_description, std::string rtsp_url, std::string username, std::string password)
{   
    // For distinguishing streams
    this->stream_description = stream_description;
    
    // For TCP connection 
	this->server_ip = server_ip;
	this->server_port = server_port;
	this->rtsp_url = rtsp_url;

    // For Auth
    this->username = username;
    this->password = password;
}

int RTSPClient::initialize_socket()
{	
    // -----------------------------
    // Initialize Winsock
    // -----------------------------
    WSADATA wsaData;

    int wsaResult =
        WSAStartup(MAKEWORD(2, 2),
            &wsaData);

    if (wsaResult != 0)
    {
        std::cerr
            << "WSAStartup failed: "
            << wsaResult
            << std::endl;
        return -1;
    }

    this->rtsp_socket =
        socket(AF_INET,
            SOCK_STREAM,
            IPPROTO_TCP);

    if (this->rtsp_socket == INVALID_SOCKET)
    {
        std::cerr
            << "Socket creation failed: "
            << WSAGetLastError()
            << std::endl;

        WSACleanup();
        return -1;
    }

    // -----------------------------
    // Configure server address
    // -----------------------------
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(this->server_port);

    inet_pton(AF_INET,
        this->server_ip.c_str(),
        &serverAddr.sin_addr);

    // -----------------------------
    // Connect
    // -----------------------------
    int connectResult =
        connect(this->rtsp_socket,
            (sockaddr*)&serverAddr,
            sizeof(serverAddr));

    if (connectResult == SOCKET_ERROR)
    {
        std::cerr
            << "Connect failed: "
            << WSAGetLastError()
            << std::endl;

        closesocket(this->rtsp_socket);
        WSACleanup();
        return -1;
    }

    std::cout
        << "Connected to RTSP server\n";

}

int RTSPClient::get_rtp_port()
{
    while (true)
    {
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET)
            throw std::runtime_error("Failed to create socket");

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(0); // OS picks free port

        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            closesocket(sock);
            throw std::runtime_error("Bind failed");
        }

        sockaddr_in boundAddr{};
        int len = sizeof(boundAddr);

        getsockname(sock, (sockaddr*)&boundAddr, &len);

        int port = ntohs(boundAddr.sin_port);

        closesocket(sock);

        // RTP should be even
        if (port % 2 != 0)
            port--;

        // Check if RTP and RTCP ports are both free
        SOCKET testRtp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        SOCKET testRtcp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        sockaddr_in rtpAddr{};
        rtpAddr.sin_family = AF_INET;
        rtpAddr.sin_addr.s_addr = INADDR_ANY;
        rtpAddr.sin_port = htons(port);

        sockaddr_in rtcpAddr{};
        rtcpAddr.sin_family = AF_INET;
        rtcpAddr.sin_addr.s_addr = INADDR_ANY;
        rtcpAddr.sin_port = htons(port + 1);

        bool rtpOk =
            bind(testRtp, (sockaddr*)&rtpAddr, sizeof(rtpAddr))
            != SOCKET_ERROR;

        bool rtcpOk =
            bind(testRtcp, (sockaddr*)&rtcpAddr, sizeof(rtcpAddr))
            != SOCKET_ERROR;

        closesocket(testRtp);
        closesocket(testRtcp);

        if (rtpOk && rtcpOk)
            return port;
    }
}

int RTSPClient::terminate_socket()
{
    closesocket(this->rtsp_socket);
    WSACleanup();
    return 0;
}

int RTSPClient::send_request(std::string request_payload, char* buffer, int buffer_size)
{   
    int sendResult =
        send(this->rtsp_socket,
            request_payload.c_str(),
            static_cast<int>(request_payload.size()),
            0);

    if (sendResult == SOCKET_ERROR)
    {
        std::cerr
            << "Send failed: "
            << WSAGetLastError()
            << std::endl;
        return -1;
    }

    int bytesReceived =
        recv(this->rtsp_socket,
            buffer,
            buffer_size,
            0);

    if (bytesReceived == SOCKET_ERROR)
    {
        std::cerr
            << "recv failed: "
            << WSAGetLastError()
            << std::endl;

        return -1;
    }

    std::string server_response(buffer);
    int server_response_code = parse_rtsp_status_code(server_response);
    this->Cseq_num += 1;
    return server_response_code;
}

std::string RTSPClient::set_player_port(const std::string& sdp_from_server, int new_port)
{
    size_t mpos = sdp_from_server.find("m=video");
    if (mpos == std::string::npos)
        return sdp_from_server;

    // 3. Find first space after "m=video"
    size_t first_space = sdp_from_server.find(' ', mpos);
    if (first_space == std::string::npos)
        return sdp_from_server;

    // 4. Find second space (end of port field)
    size_t second_space = sdp_from_server.find(' ', first_space + 1);
    if (second_space == std::string::npos)
        return sdp_from_server;

    // 5. Replace port
    std::string before = sdp_from_server.substr(0, first_space + 1);
    std::string after = sdp_from_server.substr(second_space);

    return before + std::to_string(new_port) + after;
}

std::string RTSPClient::build_auth_response(std::string control_method, std::string media_url)
{   
    std::string response;
    if (this->algo == "SHA-256") 
    {
        std::string ha1 = sha256(this->username + ":" + this->realm + ":" + this->password);
        std::string ha2 = sha256(control_method + ":" + media_url);
        response = sha256(ha1 + ":" + this->nonce + ":" + ha2);
    }
    else if (this->algo == "MD5")
    {
        std::string ha1 = md5(this->username + ":" + this->realm + ":" + this->password);
        std::string ha2 = md5(control_method + ":" + media_url);
        response = md5(ha1 + ":" + this->nonce + ":" + ha2);
    }
    return response;
}

std::string RTSPClient::build_option_request() {
    std::string option_request =
        "OPTIONS " + this->rtsp_url + " RTSP/1.0\r\n"
        "CSeq: " + std::to_string(this->Cseq_num) + "\r\n"
        "User-Agent: RTSPClient\r\n"
        "\r\n";
    return option_request;
}

std::string RTSPClient::build_describe_request(bool is_elevated_request) {
    std::string describe_request =
        "DESCRIBE " + this->rtsp_url + " RTSP/1.0\r\n"
        "CSeq: " + std::to_string(this->Cseq_num) + "\r\n"
        "User-Agent: RTSPClient\r\n"
        "Accept: application/sdp\r\n";

    if (!is_elevated_request) {
        describe_request =
            describe_request +
            "\r\n";
    }
    else
    {   
        std::string response = this->build_auth_response("DESCRIBE", this->rtsp_url);
        describe_request = describe_request + 
            "Authorization: Digest username=\"" + this->username + "\", "
            "realm=\"" + this->realm + "\", "
            "nonce=\"" + this->nonce + "\", "
            "uri=\"" + this->rtsp_url + "\", "
            "response=\"" + response + "\", "
            "algorithm=\"SHA-256\"\r\n"
            "\r\n";
    }
    
    return describe_request;
}

std::string RTSPClient::build_setup_request(const std::string& stream_control, int rtp_port, int rtcp_port)
{   
    std::string sha_response = this->build_auth_response("SETUP", this->rtsp_url + stream_control);
    std::string setup_request =
        "SETUP " + this->rtsp_url + stream_control + " RTSP/1.0\r\n"
        "CSeq: " + std::to_string(this->Cseq_num) + "\r\n"
        "User-Agent: RTSPClient\r\n"
        "Transport: RTP/AVP;unicast;client_port=" + std::to_string(rtp_port) + "-" + std::to_string(rtcp_port) + "\r\n"
        "Authorization: Digest username=\"" + this->username + "\", "
        "realm=\"" + this->realm + "\", "
        "nonce=\"" + this->nonce + "\", "
        "uri=\"" + this->rtsp_url + stream_control + "\", "
        "response=\"" + sha_response + "\", "
        "algorithm=\"" + this->algo + "\"\r\n"
        "\r\n";

    return setup_request;
}

std::string RTSPClient::build_play_request() {
    std::string play_request =
        "PLAY " + this->rtsp_url + " RTSP/1.0\r\n"
        "CSeq: " + std::to_string(this->Cseq_num) + "\r\n"
        "Session: " + this->session_id + "\r\n"
        "Range: npt=0.000-\r\n"
        "\r\n";
    return play_request;
}

std::string RTSPClient::build_teardown_request() {
    std::string play_request =
        "TEARDOWN " + this->rtsp_url + " RTSP/1.0\r\n"
        "CSeq: " + std::to_string(this->Cseq_num) + "\r\n"
        "Session: " + this->session_id + "\r\n"
        "\r\n";
    return play_request;
}

int RTSPClient::get_auth_context(const std::string& server_response)
{
    this->realm = parse_key_value(server_response, "realm");
    this->nonce = parse_key_value(server_response, "nonce");
    this->algo = parse_key_value(server_response, "algorithm");
    return 0;
}

int RTSPClient::get_session_id(const std::string& server_response)
{
    this->session_id = parse_key_value(server_response, "Session: ");
    return 0;
}

int RTSPClient::get_stream_control(const std::string& sdp_from_server)
{
    this->stream_control = "/" + parse_key_value(sdp_from_server, "a=control:");
    return 0;
}

int RTSPClient::initiate_handshake()
{   
    // -----------------------------
    // 1ST DESCRIBE request
    // -----------------------------
    int server_response_code = -1;
    char buffer[8192] = { 0 }; //buffer for tcp response
    std::string describe_request = this->build_describe_request();
    this->send_request(describe_request, buffer, 8192);

    std::string describe_response(buffer);
    this->get_auth_context(describe_response); // get nonce, realm, algo for hashing

    // -----------------------------
    // 2ND DESCRIBE request
    // -----------------------------
    memset(buffer, 0, sizeof(buffer));
    bool is_elevated_describe = 1;
    std::string elevated_describe_request = this->build_describe_request(is_elevated_describe);
    
    server_response_code = this->send_request(elevated_describe_request, buffer, 8192);
    if (server_response_code / 100 != 2)
    {
        std::cout << "2ND DESCRIBE FAILED WITH STATUS CODE: " << server_response_code << std::endl;
    }

    // extract SDP for RTP stream
    std::string sdp_response(buffer);
    this->sdp_from_server = parse_sdp(sdp_response);

    this->get_stream_control(sdp_from_server);

    // -----------------------------
    // SETUP request
    // -----------------------------
    memset(buffer, 0, sizeof(buffer));
    int rtp_port = this->get_rtp_port();
    int rtcp_port = rtp_port + 1;
    std::string setup_request = this->build_setup_request(this->stream_control, rtp_port, rtcp_port);
    
    server_response_code = this->send_request(setup_request, buffer, 8192);
    if (server_response_code / 100 != 2)
    {
        std::cout << "SETUP FAILED WITH STATUS CODE: " << server_response_code << std::endl;
    }    
    
    std::string setup_response(buffer);
    this->get_session_id(setup_response); // get session id

    // Write SDP for player
    this->sdp_to_player = this->set_player_port(sdp_from_server, rtp_port);

    // -----------------------------
    // PLAY request
    // -----------------------------
    memset(buffer, 0, sizeof(buffer));
    std::string play_request = this->build_play_request();
    
    server_response_code = this->send_request(play_request, buffer, 8192);
    if (server_response_code / 100 != 2)
    {
        std::cout << "PLAY FAILED WITH STATUS CODE: " << server_response_code << std::endl;
    }
    return 0;
}

int RTSPClient::teardown_handshake()
{
    int server_response_code = -1;
    char buffer[8192] = { 0 }; //buffer for tcp response
    std::string teardown_request = this->build_teardown_request();
    server_response_code = this->send_request(teardown_request, buffer, 8192);

    if (server_response_code / 100 != 2)
    {
        std::cout << "TEARDOWN FAILED WITH STATUS CODE: " << server_response_code << std::endl;
    }
}
