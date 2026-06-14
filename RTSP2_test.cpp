#include <iostream>
#include <string>
#include "RTSPClient.h"
#include "Parser.h"
#include <fstream>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <windows.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}

struct SdpBuffer {
    const char* ptr;
    size_t size_left;
};

int read_sdp_callback(void* opaque, uint8_t* buf, int buf_size) {
    SdpBuffer* sdp_state = static_cast<SdpBuffer*>(opaque);
    if (sdp_state->size_left == 0) {
        return AVERROR_EOF;
    }

    int read_bytes = (buf_size > sdp_state->size_left) ? sdp_state->size_left : buf_size;
    memcpy(buf, sdp_state->ptr, read_bytes);

    sdp_state->ptr += read_bytes;
    sdp_state->size_left -= read_bytes;

    return read_bytes;
}

int main()
{   
    std::string ONB_IP = "10.51.12.21";
    int ONB_port = 554;
    std::string username = "admin";
    std::string password = "Password123!";
    std::string rtsp_url = "rtsp://10.51.12.21:554/live/fc89be67-10d0-4c7d-8d51-66cb958ab128";
    std::string stream_description = "Test";
    size_t FFMPEG_IO_BUFFER_SIZE = 8192;
    
    RTSPClient rtsp_client = RTSPClient(ONB_IP, ONB_port, stream_description, rtsp_url, username, password);
    rtsp_client.initialize_socket();
    rtsp_client.initiate_handshake();

    avformat_network_init();

    // Get sdp to player and pass to ffmpeg buffer
    std::string sdp_content = rtsp_client.get_player_sdp();
    SdpBuffer sdp_state = { sdp_content.c_str(), sdp_content.size()};

    uint8_t* io_buffer = static_cast<uint8_t*>(av_malloc(FFMPEG_IO_BUFFER_SIZE));
    AVIOContext* avio_ctx = avio_alloc_context(
        io_buffer,
        FFMPEG_IO_BUFFER_SIZE,
        0,               // 0 for reading
        &sdp_state,      
        &read_sdp_callback,
        nullptr,         // No write callback
        nullptr          // No seek callback
    );

    AVFormatContext* format_ctx = avformat_alloc_context();
    format_ctx->pb = avio_ctx;
    const AVInputFormat* sdp_format = av_find_input_format("sdp");

    AVDictionary* opts = nullptr;

    av_dict_set(&opts, "protocol_whitelist",
        "file,udp,rtp,tcp,crypto,data", 0);

    av_dict_set(&opts, "fflags", "nobuffer", 0);
    av_dict_set(&opts, "flags", "low_delay", 0);
    av_dict_set(&opts, "max_delay", "0", 0);

    int ret = avformat_open_input(
        &format_ctx,
        nullptr,
        sdp_format,
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

    int ret2 = avformat_find_stream_info(format_ctx, nullptr);
    if (ret2 < 0)
    {
        char err[256];
        av_strerror(ret2, err, sizeof(err));
        std::cerr << "stream info failed: " << err << std::endl;
    }

    // Find video stream
    int videoStream = -1;

    for (unsigned int i = 0; i < format_ctx->nb_streams; i++)
    {
        if (format_ctx->streams[i]->codecpar->codec_type
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
            format_ctx->streams[videoStream]
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
        format_ctx->streams[videoStream]->codecpar);

    ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0)
    {
        std::cerr << "Failed to open decoder\n";
        return -1;
    }

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    std::cout << "Waiting for RTP packets..." << std::endl;

    // Initialize player
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return -1;
    }

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

    while (av_read_frame(format_ctx, pkt) >= 0)
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

    rtsp_client.teardown_handshake();
    rtsp_client.terminate_socket();

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&format_ctx);

    avio_context_free(&avio_ctx);
    av_free(io_buffer);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
