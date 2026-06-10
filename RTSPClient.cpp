#include "RTSPClient.h"
#include "Parser.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")


RTSPClient::RTSPClient(std::string serverIP, int port, std::string rtspURL, std::string username, std::string password) 
{
	this->serverIP = serverIP;
	this->port = port;
	this->rtspURL = rtspURL;
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

    this->rtp_socket =
        socket(AF_INET,
            SOCK_DGRAM,
            IPPROTO_UDP);

    this->rtcp_socket =
        socket(AF_INET,
            SOCK_DGRAM,
            IPPROTO_UDP);

    if (this->rtsp_socket == INVALID_SOCKET)
    {
        std::cerr
            << "Socket creation failed: "
            << WSAGetLastError()
            << std::endl;

        WSACleanup();
        return -1;
    }

    if (this->rtp_socket == INVALID_SOCKET)
    {
        std::cerr
            << "RTP socket creation failed: "
            << WSAGetLastError()
            << std::endl;

        return -1;
    }

    if (this->rtcp_socket == INVALID_SOCKET)
    {
        std::cerr
            << "RTCP socket creation failed: "
            << WSAGetLastError()
            << std::endl;

        closesocket(this->rtp_socket);
        return -1;
    }

    // -----------------------------
    // Configure server address
    // -----------------------------
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(this->port);

    inet_pton(AF_INET,
        this->serverIP.c_str(),
        &serverAddr.sin_addr);


    sockaddr_in rtp_addr{};
    rtp_addr.sin_family = AF_INET;
    rtp_addr.sin_addr.s_addr = INADDR_ANY;
    rtp_addr.sin_port = htons(5000);

    int bindResult =
        bind(this->rtp_socket,
            (sockaddr*)&rtp_addr,
            sizeof(rtp_addr));

    if (bindResult == SOCKET_ERROR)
    {
        std::cerr
            << "RTP bind failed: "
            << WSAGetLastError()
            << std::endl;

        return -1;
    }

    sockaddr_in rtcp_addr{};
    rtcp_addr.sin_family = AF_INET;
    rtcp_addr.sin_addr.s_addr = INADDR_ANY;
    rtcp_addr.sin_port = htons(5001);

    bindResult =
        bind(this->rtcp_socket,
            (sockaddr*)&rtcp_addr,
            sizeof(rtcp_addr));

    if (bindResult == SOCKET_ERROR)
    {
        std::cerr
            << "RTCP bind failed: "
            << WSAGetLastError()
            << std::endl;

        return -1;
    }

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

int RTSPClient::terminate_socket()
{
    closesocket(this->rtsp_socket);
    closesocket(this->rtp_socket);
    closesocket(this->rtcp_socket);
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

    this->Cseq_num += 1;
    return 0;
}

std::string RTSPClient::build_sha_response(std::string method, std::string media_url)
{
    std::string ha1 = sha256(this->username + ":" + this->realm + ":" + this->password);
    std::string ha2 = sha256(method + ":" + media_url);
    std::string response = sha256(ha1 + ":" + nonce + ":" + ha2);

    return response;
}

std::string RTSPClient::build_option_request() {
    std::string option_request =
        "OPTIONS " + this->rtspURL + " RTSP/1.0\r\n"
        "CSeq: " + std::to_string(this->Cseq_num) + "\r\n"
        "User-Agent: LibVLC/4.0.0 - dev(LIVE555 Streaming Media v2022.07.14)\r\n"
        "\r\n";
    return option_request;
}

std::string RTSPClient::build_describe_request(int elevated_request) {
    std::string describe_request =
        "DESCRIBE " + this->rtspURL + " RTSP/1.0\r\n"
        "CSeq: " + std::to_string(this->Cseq_num) + "\r\n"
        "User-Agent:LibVLC/4.0.0 - dev(LIVE555 Streaming Media v2022.07.14)\r\n"
        "Accept: application/sdp\r\n";

    if (elevated_request == 0) {
        describe_request =
            describe_request +
            "\r\n";
    }
    else
    {   
        std::string response = this->build_sha_response("DESCRIBE", this->rtspURL);
        describe_request = describe_request + 
            "Authorization: Digest username=\"" + this->username + "\", "
            "realm=\"" + this->realm + "\", "
            "nonce=\"" + this->nonce + "\", "
            "uri=\"" + this->rtspURL + "\", "
            "response=\"" + response + "\", "
            "algorithm=\"SHA-256\"\r\n"
            "\r\n";
    }
    
    return describe_request;
}

std::string RTSPClient::build_setup_request()
{   
    std::string sha_response = this->build_sha_response("SETUP", this->rtspURL + "/TrackId=0");
    std::string setup_request =
        "SETUP " + this->rtspURL + "/TrackId=0" + " RTSP/1.0\r\n"
        "CSeq: " + std::to_string(this->Cseq_num) + "\r\n"
        "User-Agent:LibVLC/4.0.0 - dev(LIVE555 Streaming Media v2022.07.14)\r\n"
        "Transport: RTP/AVP;unicast;client_port=5000-5001\r\n"
        "Authorization: Digest username=\"" + this->username + "\", "
        "realm=\"" + this->realm + "\", "
        "nonce=\"" + this->nonce + "\", "
        "uri=\"" + this->rtspURL + "/TrackId=0" + "\", "
        "response=\"" + sha_response + "\", "
        "algorithm=\"SHA-256\"\r\n"
        "\r\n";

    return setup_request;
}

std::string RTSPClient::build_play_request() {
    std::string play_request =
        "PLAY " + this->rtspURL + " RTSP/1.0\r\n"
        "CSeq: " + std::to_string(this->Cseq_num) + "\r\n"
        "Session: " + this->sessionId + "\r\n"
        "Range: npt=0.000-\r\n"
        "\r\n";
    return play_request;
}

int RTSPClient::parse_context(const std::string& server_response)
{
    std::string realm, nonce, algorithm;

    size_t pos = server_response.find("WWW-Authenticate:");

    if (pos == std::string::npos)
        return -1;

    std::string header = server_response.substr(pos);

    this->realm = parse_value(header, "realm");
    this->nonce = parse_value(header, "nonce");
    this->algo = parse_value(header, "algorithm");
    this->sessionId = parse_value(header, "Session");
    return 0;
}

std::string RTSPClient::extract_session(const std::string& response)
{
    std::string key = "Session:";
    size_t pos = response.find(key);

    if (pos == std::string::npos)
        return "";

    pos += key.length();

    // skip spaces
    while (pos < response.size() &&
        (response[pos] == ' ' || response[pos] == '\t'))
    {
        pos++;
    }

    size_t end = response.find_first_of("\r\n", pos);

    return response.substr(pos, end - pos);
}

int RTSPClient::initiate_handshake()
{
    char buffer[8192] = { 0 }; //buffer for tcp response
    std::string option_request = this->build_option_request();
    this->send_request(option_request, buffer, 8192);
    std::cout << "OPTION OK" << std::endl;

    memset(buffer, 0, sizeof(buffer));

    std::string describe_request = this->build_describe_request();
    this->send_request(describe_request, buffer, 8192);
    std::cout << "1st DESCRIBE OK" << std::endl;

    std::string describe_response(buffer);
    this->parse_context(describe_response); // get nonce, realm, algo for hashing

    memset(buffer, 0, sizeof(buffer));

    int elevated_describe = 1;
    std::string elevated_describe_request = this->build_describe_request(elevated_describe);
    this->send_request(elevated_describe_request, buffer, 8192);
    std::cout << "2nd DESCRIBE OK" << std::endl;

    // extract SDP for RTP stream
    std::string sdp_response(buffer);
    std::string sdp = parse_sdp(sdp_response);
    std::ofstream file("stream.sdp");
    file << sdp;
    file.close();

    memset(buffer, 0, sizeof(buffer));

    int elevated_setup = 1;
    std::string elevated_setup_request = this->build_setup_request();
    this->send_request(elevated_setup_request, buffer, 8192);
    std::cout << "SETUP OK" << std::endl;

    std::string setup_response(buffer);
    this->sessionId = this->extract_session(setup_response);

    memset(buffer, 0, sizeof(buffer));
    std::string play_request = this->build_play_request();
    this->send_request(play_request, buffer, 8192);
    std::cout << "PLAY OK" << std::endl;
}
