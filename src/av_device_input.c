#include "av_device_input.h"

AVDeviceInput *av_device_input_alloc(const char *input_format_name) {
    avdevice_register_all();
    AVDeviceInput *device = av_mallocz(sizeof(AVDeviceInput));
    memset(device, 0, sizeof(AVDeviceInput));
    device->input_format = av_find_input_format(input_format_name);
    if (device->input_format == NULL) {
        printf("av_device_input_alloc: not found %s\n", input_format_name);
        goto fail;
    }
    device->packet = av_packet_alloc();
    return device;

fail:
    av_device_input_free(&device);
    return NULL;
}

void av_device_input_free(AVDeviceInput **device_addr) {
    if (device_addr == NULL) {
        return;
    }
    AVDeviceInput *device = *device_addr;
    av_device_input_stop(device, 1);
    if (device->packet != NULL) {
        av_packet_free(&device->packet);
    }
    av_free(device);
    device_addr = NULL;
}

int av_device_input_dict_set(AVDeviceInput *device, const char *key, const char *value) {
    const int ret = av_dict_set(&device->dict, key, value, 0);
    if (ret < 0) {
        printf("av_device_input_dict_set: av_dict_set = %s\n", av_err2str(ret));
    }
    return ret;
}

int av_device_input_dict_set_int(AVDeviceInput *device, const char *key, const long long value) {
    const int ret = av_dict_set_int(&device->dict, key, value, 0);
    if (ret < 0) {
        printf("av_device_input_dict_set_int: av_dict_set_int = %s\n", av_err2str(ret));
    }
    return ret;
}

int av_device_input_start(AVDeviceInput *device,
                          const char *device_name,
                          const enum AVMediaType type) {
    int ret = 0;
    AVDictionary *dictionary = NULL;
    if (device->dict != NULL) {
        ret = av_dict_copy(&dictionary, device->dict, 0);
        if (ret < 0) {
            printf("av_device_input_start: av_dict_copy = %s\n", av_err2str(ret));
            goto fail;
        }
    }

    ret = avformat_open_input(&device->input_ctx, device_name, device->input_format, &dictionary);
    if (ret < 0) {
        printf("av_device_input_start: avformat_open_input = %s\n", av_err2str(ret));
        goto fail;
    }

    ret = avformat_find_stream_info(device->input_ctx, NULL);
    if (ret < 0) {
        printf("av_device_input_start: avformat_open_input = %s\n", av_err2str(ret));
        goto fail;
    }

    const int videoIndex = av_find_best_stream(device->input_ctx, type, -1, -1, NULL, 0);
    if (videoIndex < 0) {
        printf("av_device_input_start: avformat_open_input = %s\n", av_err2str(videoIndex));
        goto fail;
    }
    av_dump_format(device->input_ctx, videoIndex, device_name, 0);

    device->stream = device->input_ctx->streams[videoIndex];
    const AVCodecParameters *codec_params = device->stream->codecpar;
    const AVCodec *input_decoder = avcodec_find_decoder(codec_params->codec_id);
    if (input_decoder == NULL) {
        printf("av_device_input_start: avcodec_find_decoder-codec_id = %d\n", codec_params->codec_id);
        goto fail;
    }

    device->decoder_ctx = avcodec_alloc_context3(input_decoder);
    ret = avcodec_parameters_to_context(device->decoder_ctx, codec_params);
    if (ret < 0) {
        printf("av_device_input_start: avcodec_parameters_to_context = %s\n", av_err2str(ret));
        goto fail;
    }

    ret = avcodec_open2(device->decoder_ctx, input_decoder, NULL);
    if (ret < 0) {
        printf("av_device_input_start: avcodec_open2 = %s\n", av_err2str(ret));
        goto fail;
    }

    return ret;

fail:
    av_device_input_stop(device, 0);
    return ret;
}

AVDeviceInputParams av_device_input_get_params(AVDeviceInput *device) {
    AVDeviceInputParams params;
    memset(&params, 0, sizeof(AVDeviceInputParams));
    if (device->stream != NULL) {
        const AVCodecParameters *codec_params = device->stream->codecpar;
        params.width = codec_params->width;
        params.height = codec_params->height;
        params.input_format = codec_params->format;
        params.time_base = device->stream->time_base;
    }
    return params;
}

void av_device_input_stop(AVDeviceInput *device, const int clear_dict) {
    if (device->input_ctx != NULL) {
        avformat_close_input(&device->input_ctx);
    }
    if (device->decoder_ctx != NULL) {
        avcodec_free_context(&device->decoder_ctx);
    }
    if (clear_dict && device->dict != NULL) {
        av_dict_free(&device->dict);
    }
    device->stream = NULL;
}

int av_device_receive_frame(AVDeviceInput *device, AVFrame *frame) {
    av_packet_unref(device->packet);
    int ret = av_read_frame(device->input_ctx, device->packet);
    if (ret < 0) {
        if (ret != -EAGAIN) {
            printf("av_device_receive_frame: av_read_frame = %s\n", av_err2str(ret));
        }
    } else {
        ret = avcodec_send_packet(device->decoder_ctx, device->packet);
        if (ret < 0) {
            printf("av_device_receive_frame: av_read_frame = %s\n", av_err2str(ret));
        }
    }

    ret = avcodec_receive_frame(device->decoder_ctx, frame);
    if (ret < 0 && ret != -EAGAIN) {
        printf("av_encoder_receive_frame: avcodec_receive_frame = %s\n", av_err2str(ret));
    }
    return ret;
}
