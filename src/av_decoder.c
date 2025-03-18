//
// Created by zhixingye on 2025/3/15.
//

#include "av_decoder.h"

AVDecoder *av_decoder_alloc(const char *codec_name) {
    AVDecoder *decoder = av_mallocz(sizeof(AVDecoder));
    decoder->codec = avcodec_find_encoder_by_name(codec_name);
    if (decoder->codec == NULL) {
        printf("av_decoder_alloc: codec not found: %s\n", codec_name);
        goto fail;
    }
    decoder->frame = av_frame_alloc();
    if (decoder->frame == NULL) {
        printf("av_decoder_alloc: alloc AVFrame fail\n");
        goto fail;
    }

    return decoder;

fail:
    av_decoder_free(&decoder);
    return NULL;
}

void av_decoder_free(AVDecoder **decoder_addr) {
    if (decoder_addr == NULL) {
        return;
    }
    AVDecoder *decoder = *decoder_addr;
    av_decoder_stop(decoder);
    if (decoder->frame != NULL) {
        av_frame_free(&decoder->frame);
    }
    av_free(decoder);
    decoder_addr = NULL;
}

int av_decoder_start(AVDecoder *decoder, AVVideoDecoderParams params) {
    av_decoder_stop(decoder);
    decoder->ctx = avcodec_alloc_context3(decoder->codec);
    if (decoder->ctx == NULL) {
        printf("av_decoder_start: avcodec_alloc_context3 == NULL\n");
        return -EPERM;
    }

    decoder->ctx->width = params.width;
    decoder->ctx->height = params.height;
    decoder->ctx->pix_fmt = params.output_format;

    const int ret = avcodec_open2(decoder->ctx, decoder->codec, NULL);
    if (ret < 0) {
        printf("av_decoder_start: avcodec_open2 = %s\n", av_err2str(ret));
        return ret;
    }
    return 0;
}

void av_decoder_stop(AVDecoder *decoder) {
    if (decoder->ctx != NULL) {
        avcodec_free_context(&decoder->ctx);
    }
    if (decoder->frame != NULL) {
        av_frame_unref(decoder->frame);
    }
}

int av_decoder_send_oacket(AVDecoder *decoder, const AVPacket *packet) {
    if (decoder->ctx == NULL) {
        printf("please call av_decoder_start first\n");
        return -EPERM;
    }
    const int ret = avcodec_send_packet(decoder->ctx, packet);
    if (ret < 0) {
        printf("av_decoder_send_oacket: avcodec_send_packet = %s\n", av_err2str(ret));
    }
    return ret;
}

AVFrame *av_decoder_receive_frame(const AVDecoder *decoder) {
    if (decoder->ctx == NULL) {
        printf("please call av_decoder_start first\n");
        return NULL;
    }
    av_frame_unref(decoder->frame);
    if (avcodec_receive_frame(decoder->ctx, decoder->frame) >= 0) {
        return decoder->frame;
    }
    return NULL;
}
