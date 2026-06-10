#include <iostream>
#include <string>
#include "RTSPClient.h"
#include "Parser.h"
#include <fstream>

/* add once reading from .ini is setup
#include <QSettings>
#include <QCoreApplication>
#include <QString>
#include <QDebug>
*/

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

int main()
{   
    /*
    QString path = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(path, QSettings::IniFormat);

    // ONB Info
    std::string ONB_IP = settings.value("ONB_INFO/ONB_IP").toString().toStdString();
    int ONB_port = settings.value("ONB_INFO/ONB_port").toInt();
    std::string Username = settings.value("ONB_INFO/Username").toString().toStdString();
    std::string Pwd = settings.value("ONB_INFO/Pwd").toString().toStdString();
    std::string Auth_Algo = settings.value("ONB_INFO/Auth_Algo").toString().toStdString();

    // RTSP URL
    std::string Dayfront_URL = settings.value("ONB_INFO/Dayfront_URL").toString().toStdString();
    std::string Dayrear_URL = settings.value("ONB_INFO/Dayrear_URL").toString().toStdString();
    std::string Nightfront_URL = settings.value("ONB_INFO/Nightfront_URL").toString().toStdString();
    std::string Nightrear_URL = settings.value("ONB_INFO/Nightrear_URL").toString().toStdString();

    std::string SEOS_URL = settings.value("ONB_INFO/SEOS_URL").toString().toStdString();

    std::string Wheelhouse_URL = settings.value("ONB_INFO/Wheelhouse_URL").toString().toStdString();
    std::string Serverroom_URL = settings.value("ONB_INFO/Serverroom_URL").toString().toStdString();
    std::string Engineroom_URL = settings.value("ONB_INFO/Engineroom_URL").toString().toStdString();
    std::string Platform_URL = settings.value("ONB_INFO/Platform_URL").toString().toStdString();

    // Stereo Streams
    RTSPClient dayfront_client = RTSPClient(ONB_IP, ONB_port, rtspURL);
    RTSPClient dayrear_client = RTSPClient(ONB_IP, ONB_port, rtspURL);
    RTSPClient nightfront_client = RTSPClient(ONB_IP, ONB_port, rtspURL);
    RTSPClient nightrear_client = RTSPClient(ONB_IP, ONB_port, rtspURL);

    // SEOS Streams
    RTSPClient seos_client = RTSPClient(ONB_IP, ONB_port, rtspURL);

    // CCTV Streams
    RTSPClient wheelhouse_client = RTSPClient(ONB_IP, ONB_port, rtspURL);
    RTSPClient serverroom_client = RTSPClient(ONB_IP, ONB_port, rtspURL);
    RTSPClient engineroom_client = RTSPClient(ONB_IP, ONB_port, rtspURL);
    RTSPClient platform_client = RTSPClient(ONB_IP, ONB_port, rtspURL);
    

    // Initialize TCP Socket to ONB
    dayfront_client.initialize_socket();
    dayrear_client.initialize_socket();
    nightfront_client.initialize_socket();
    nightrear_client.initialize_socket();
    seos_client.initialize_socket();
    wheelhouse_client.initialize_socket();
    serverroom_client.initialize_socket();
    engineroom_client.initialize_socket();
    platform_client.initialize_socket();

    // Initiate Handshake for Auth
    dayfront_client.initiate_handshake();
    dayrear_client.initiate_handshake();
    nightfront_client.initiate_handshake();
    nightrear_client.initiate_handshake();
    seos_client.initiate_handshake();
    wheelhouse_client.initiate_handshake();
    serverroom_client.initiate_handshake();
    engineroom_client.initiate_handshake();
    platform_client.initiate_handshake();

    */

    std::string ONB_IP = "10.51.12.21";
    int ONB_port = 554;
    std::string username = "test";
    std::string password = "Password123!";
    std::string test_URL = "rtsp://10.51.12.21:554/live/fc89be67-10d0-4c7d-8d51-66cb958ab128";
    
    RTSPClient test_client = RTSPClient(ONB_IP, ONB_port, test_URL, username, password);
    test_client.initialize_socket();
    test_client.initiate_handshake();

    // random nonsense for playing stream

    avformat_network_init();

    AVFormatContext* fmtCtx = nullptr;

    AVDictionary* opts = nullptr;

    av_dict_set(&opts, "protocol_whitelist",
        "file,udp,rtp,crypto,data", 0);

    av_dict_set(&opts, "fflags", "nobuffer", 0);
    av_dict_set(&opts, "flags", "low_delay", 0);

    int ret = avformat_open_input(
        &fmtCtx,
        "stream.sdp",
        nullptr,
        &opts
    );

    if (ret < 0)
    {
        char err[256];
        av_strerror(ret, err, sizeof(err));

        std::cerr << "Failed to open SDP: " << err << std::endl;
        return -1;
    }

    std::cout << "SDP opened successfully\n";

    avformat_find_stream_info(fmtCtx, nullptr);

    int ret2 = avformat_find_stream_info(fmtCtx, nullptr);
    if (ret2 < 0)
    {
        char err[256];
        av_strerror(ret2, err, sizeof(err));
        std::cerr << "stream info failed: " << err << std::endl;
    }

    AVPacket pkt;

    while (av_read_frame(fmtCtx, &pkt) >= 0)
    {
        std::cout << "Got packet, stream: "
            << pkt.stream_index
            << " size: "
            << pkt.size
            << std::endl;

        av_packet_unref(&pkt);
    }
    return 0;
}
