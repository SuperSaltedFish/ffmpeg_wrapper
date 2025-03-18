#ifndef AV_ENCODER_H
#define AV_ENCODER_H

#include <libavcodec/avcodec.h>

typedef struct AVEncoder {
    const AVCodec *codec;
    AVCodecContext *ctx;
} AVEncoder;

typedef struct {
    int width;
    int height;
    int framerate;
    int gop_size;
    long long bitrate;
    enum AVPixelFormat input_format;
} AVVideoEncodeParams;

AVEncoder *av_encoder_alloc(const char *codec_name);

void av_encoder_free(AVEncoder **encoder_addr);

int av_encoder_start(AVEncoder *encoder, AVVideoEncodeParams params);

void av_encoder_stop(AVEncoder *encoder);

int av_encoder_send_frame(AVEncoder *encoder, const AVFrame *frame);

int av_encoder_receive_packet(AVEncoder *encoder, AVPacket *packet);

#endif //AV_ENCODER_H
