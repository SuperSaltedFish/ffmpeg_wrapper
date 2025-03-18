#include "av_encoder.h"

AVEncoder *av_encoder_alloc(const char *codec_name) {
    AVEncoder *encoder = av_mallocz(sizeof(AVEncoder));
    encoder->codec = avcodec_find_encoder_by_name(codec_name);
    if (encoder->codec == NULL) {
        printf("av_encoder_alloc: codec not found: %s\n", codec_name);
        goto fail;
    }
    return encoder;

fail:
    av_encoder_free(&encoder);
    return NULL;
}

void av_encoder_free(AVEncoder **encoder_addr) {
    if (encoder_addr == NULL) {
        return;
    }
    AVEncoder *encoder = *encoder_addr;
    av_encoder_stop(encoder);
    av_free(encoder);
    encoder_addr = NULL;
}

int av_encoder_start(AVEncoder *encoder, const AVVideoEncodeParams params) {
    av_encoder_stop(encoder);
    encoder->ctx = avcodec_alloc_context3(encoder->codec);
    if (encoder->ctx == NULL) {
        printf("av_encoder_start: avcodec_alloc_context3 == NULL\n");
        return -EPERM;
    }

    encoder->ctx->width = params.width;
    encoder->ctx->height = params.height;
    encoder->ctx->pix_fmt = params.input_format;
    encoder->ctx->gop_size = params.gop_size;
    encoder->ctx->bit_rate = params.bitrate;
    encoder->ctx->time_base = (AVRational){1, params.framerate};
    encoder->ctx->framerate = (AVRational){params.framerate, 1};

    const int ret = avcodec_open2(encoder->ctx, encoder->codec, NULL);
    if (ret < 0) {
        printf("av_encoder_start: avcodec_open2 = %s\n", av_err2str(ret));
        return ret;
    }
    return 0;
}

void av_encoder_stop(AVEncoder *encoder) {
    if (encoder->ctx != NULL) {
        avcodec_free_context(&encoder->ctx);
    }
}

int av_encoder_send_frame(AVEncoder *encoder, const AVFrame *frame) {
    if (encoder->ctx == NULL) {
        printf("please call av_encoder_start first\n");
        return -EPERM;
    }
    const int ret = avcodec_send_frame(encoder->ctx, frame);
    if (ret < 0) {
        printf("av_encoder_send_frame: avcodec_send_frame = %s\n", av_err2str(ret));
    }
    return ret;
}

int av_encoder_receive_packet(AVEncoder *encoder, AVPacket *packet) {
    if (encoder->ctx == NULL) {
        printf("please call av_encoder_start first\n");
        return -EPERM;
    }
    const int ret = avcodec_receive_packet(encoder->ctx, packet);
    if (ret < 0 && ret != -EAGAIN) {
        printf("av_encoder_receive_packet: avcodec_receive_packet = %s\n", av_err2str(ret));
    }
    return ret;
}
