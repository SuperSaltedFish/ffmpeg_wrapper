#ifndef AV_EXPORTER_H
#define AV_EXPORTER_H

#include <libavformat/avformat.h>
#include "av_encoder.h"

typedef struct AVOutput {
    AVFormatContext *ctx;
    AVStream *stream;
} AVOutput;

AVOutput *av_output_alloc_from_encoder(const AVEncoder *encoder, const char *protocol, const char *url);

int av_output_send_packet(AVOutput *encoder,AVPacket *packet);

void av_output_free(AVOutput **output_addr);

#endif //AV_EXPORTER_H
