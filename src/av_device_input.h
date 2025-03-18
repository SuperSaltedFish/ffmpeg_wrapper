#ifndef AV_DEVICE_INPUT_H
#define AV_DEVICE_INPUT_H

#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>

typedef struct AVDeviceInput {
    const AVInputFormat *input_format;
    const AVStream *stream;
    AVFormatContext *input_ctx;
    AVCodecContext *decoder_ctx;
    AVPacket *packet;
    AVDictionary *dict;
} AVDeviceInput;

typedef struct {
    int width;
    int height;
    enum AVPixelFormat input_format;
    AVRational time_base;
} AVDeviceInputParams;

AVDeviceInput *av_device_input_alloc(const char *input_format_name);

void av_device_input_free(AVDeviceInput **device_addr);

int av_device_input_dict_set(AVDeviceInput *device, const char *key, const char *value);

int av_device_input_dict_set_int(AVDeviceInput *device, const char *key, long long value);

int av_device_input_start(AVDeviceInput *device, const char *device_name, enum AVMediaType type);

AVDeviceInputParams av_device_input_get_params(AVDeviceInput *device);

void av_device_input_stop(AVDeviceInput *device, int clear_dict);

int av_device_receive_frame(AVDeviceInput *device, AVFrame *frame);

#endif //AV_DEVICE_INPUT_H
