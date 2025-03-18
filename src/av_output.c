#include "av_output.h"


AVOutput *av_output_alloc_from_encoder(const AVEncoder *encoder, const char *protocol, const char *url) {
    AVOutput *output = av_mallocz(sizeof(AVOutput));
    int ret = avformat_alloc_output_context2(&output->ctx, NULL, protocol, url);
    if (ret < 0) {
        printf("av_output_alloc: avformat_alloc_output_context2 = %s, protocol = %s, url = %s\n",
               av_err2str(ret), protocol, url);
        goto fail;
    }

    AVStream *stream = avformat_new_stream(output->ctx, NULL);
    if (stream == NULL) {
        printf("av_output_alloc: avformat_new_stream = NULL\n");
        goto fail;
    }

    ret = avcodec_parameters_from_context(stream->codecpar, encoder->ctx);
    if (ret < 0) {
        printf("av_output_alloc: avcodec_parameters_from_context = %s",av_err2str(ret));
        goto fail;
    }

    ret = avio_open(&output->ctx->pb, url, AVIO_FLAG_WRITE);
    if (ret < 0) {
        printf("av_output_alloc: avio_open = %s",av_err2str(ret));
        goto fail;
    }

    ret = avformat_write_header(output->ctx, NULL);
    if (ret < 0) {
        printf("av_output_alloc: avformat_write_header = %s",av_err2str(ret));
        goto fail;
    }

    output->stream = stream;
    return output;

fail:
    av_output_free(&output);
    return NULL;
}

int av_output_send_packet(AVOutput *output, AVPacket *packet) {
    const int ret = av_interleaved_write_frame(output->ctx, packet);
    if (ret < 0) {
        printf("av_output_send_packet: av_interleaved_write_frame = %s\n",av_err2str(ret));
    }
    return ret;
}

void av_output_free(AVOutput **output_addr) {
    if (output_addr == NULL) {
        return;
    }
    AVOutput *output = *output_addr;
    if (output->stream != NULL) {
        av_write_trailer(output->ctx);
        output->ctx = NULL;
    }
    if (output->ctx != NULL) {
        avformat_close_input(&output->ctx);
    }
    av_free(output);
    output_addr = NULL;
}
