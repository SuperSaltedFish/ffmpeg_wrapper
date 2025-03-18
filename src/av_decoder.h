//
// Created by zhixingye on 2025/3/15.
//

#ifndef AV_DECODER_H
#define AV_DECODER_H
#include <libavcodec/avcodec.h>

typedef struct AVDecoder {
    const AVCodec *codec;
    AVCodecContext *ctx;
    AVFrame *frame;
} AVDecoder;

typedef struct {
    int width;
    int height;
    int framerate;
    enum AVPixelFormat output_format;
} AVVideoDecoderParams;

AVDecoder *av_decoder_alloc(const char *codec_name);

void av_decoder_free(AVDecoder **decoder_addr);

int av_decoder_start(AVDecoder *decoder, AVVideoDecoderParams params);

void av_decoder_stop(AVDecoder *decoder);

int av_decoder_send_oacket(AVDecoder *decoder, const AVPacket *packet);

AVFrame *av_decoder_receive_frame(const AVDecoder *decoder);


#endif //AV_DECODER_H
