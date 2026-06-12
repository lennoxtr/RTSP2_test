#include <iostream>
#include <string>
#include "RTSPClient.h"
#include "Parser.h"
#include <fstream>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

/* add once reading from .ini is setup
#include <QSettings>
#include <QCoreApplication>
#include <QString>
#include <QDebug>
*/

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}

int main()
{   
    SDL_SetMainReady();
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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    avformat_network_init();

    AVFormatContext* fmtCtx = nullptr;
    AVDictionary* opts = nullptr;

    av_dict_set(&opts, "protocol_whitelist",
        "file,udp,rtp,crypto,data", 0);

    av_dict_set(&opts, "fflags", "nobuffer", 0);
    av_dict_set(&opts, "flags", "low_delay", 0);
    av_dict_set(&opts, "max_delay", "0", 0);

    int ret = avformat_open_input(
        &fmtCtx,
        "stream.sdp",
        nullptr,
        &opts
    );

    av_dict_free(&opts);

    if (ret < 0)
    {
        char err[256];
        av_strerror(ret, err, sizeof(err));

        std::cerr << "Failed to open SDP: " << err << std::endl;
        return -1;
    }

    std::cout << "SDP opened successfully\n";

    int ret2 = avformat_find_stream_info(fmtCtx, nullptr);
    if (ret2 < 0)
    {
        char err[256];
        av_strerror(ret2, err, sizeof(err));
        std::cerr << "stream info failed: " << err << std::endl;
    }

    // Find video stream
    int videoStream = -1;

    for (unsigned int i = 0; i < fmtCtx->nb_streams; i++)
    {
        if (fmtCtx->streams[i]->codecpar->codec_type
            == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        }
    }

    if (videoStream == -1)
    {
        std::cerr << "No video stream found\n";
        return -1;
    }

    // Find decoder
    const AVCodec* codec =
        avcodec_find_decoder(
            fmtCtx->streams[videoStream]
            ->codecpar->codec_id);

    if (!codec)
    {
        std::cerr << "Decoder not found\n";
        return -1;
    }

    // Create decoder context
    AVCodecContext* codecCtx =
        avcodec_alloc_context3(codec);

    avcodec_parameters_to_context(
        codecCtx,
        fmtCtx->streams[videoStream]->codecpar);

    ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0)
    {
        std::cerr << "Failed to open decoder\n";
        return -1;
    }

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    std::cout << "Waiting for RTP packets..." << std::endl;


    SDL_Window* window = SDL_CreateWindow(
        "RTSP Player",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        codecCtx->width,
        codecCtx->height,
        SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, 0);

    SDL_Texture* texture =
        SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING,
            codecCtx->width,
            codecCtx->height);

    SDL_Event event;

    while (av_read_frame(fmtCtx, pkt) >= 0)
    {
        if (pkt->stream_index == videoStream)
        {   
            ret = avcodec_send_packet(codecCtx, pkt);

            if (ret >= 0)
            {
                while (avcodec_receive_frame(codecCtx, frame) >= 0)
                {
                    std::cout << frame->width << ", " << frame->height << std::endl;
                    // -----------------------------
                    // render frame
                    // -----------------------------
                    SDL_UpdateYUVTexture(
                        texture,
                        nullptr,
                        frame->data[0],
                        frame->linesize[0],
                        frame->data[1],
                        frame->linesize[1],
                        frame->data[2],
                        frame->linesize[2]);

                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
                    SDL_RenderPresent(renderer);
                }
            }
        }

        av_packet_unref(pkt);
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
