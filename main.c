#include <stdio.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>

#include "src/av_device_input.h"
#include "src/av_encoder.h"
#include "src/av_filter.h"
#include "src/av_output.h"
#include "src/av_util.h"


AVDeviceInput *create_device_input() {
    AVDeviceInput *device_input = av_device_input_alloc("avfoundation");
    if (device_input == NULL) {
        return NULL;
    }
    // av_device_input_dict_set(device_input, "video_size", "1920x1080");
    av_device_input_dict_set_int(device_input, "pixel_format", AV_PIX_FMT_NV12);
    av_device_input_dict_set(device_input, "framerate", "30");
    if (av_device_input_start(device_input, "1", AVMEDIA_TYPE_VIDEO) < 0) {
        av_device_input_free(&device_input);
        return NULL;
    }
    return device_input;
}

AVEncoder *create_encoder(const int width, const int height) {
    AVEncoder *encoder = av_encoder_alloc("h264_videotoolbox");
    if (encoder == NULL) {
        return NULL;
    }

    AVVideoEncodeParams video_params;
    memset(&video_params, 0, sizeof(video_params));
    video_params.width = width;
    video_params.height = height;
    video_params.input_format = AV_PIX_FMT_VIDEOTOOLBOX;
    video_params.gop_size = 60;
    video_params.framerate = 30;
    if (av_encoder_start(encoder, video_params) < 0) {
        av_encoder_free(&encoder);
        return NULL;
    }
    return encoder;
}

AVFilters *create_filters(const AVDeviceInputParams params, const int outputWidth, const int outputHeight) {
    AVFilters *filters = av_filter_alloc(params.width,
                                         params.height,
                                         params.input_format,
                                         params.time_base);
    const enum AVHWDeviceType type = AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
    char args[32];
    sprintf(args, "derive_device=%s", av_hwdevice_get_type_name(type));
    if (av_filter_add_filter_hw(filters,
                                type,
                                NULL,
                                "hwupload",
                                args)) {
        av_filter_free(&filters);
        return NULL;
    }
    sprintf(args, "w=%d:h=%d", outputWidth, outputHeight);
    if (av_filter_add_filter(filters, "scale_vt", args)) {
        av_filter_free(&filters);
        return NULL;
    }
    if (av_filter_link(filters)) {
        av_filter_free(&filters);
        return NULL;
    }
    return filters;
}

AVOutput *create_output(const AVEncoder *encoder) {
    // AVOutput *output = av_output_alloc_from_encoder(encoder, "h264", "udp://233.233.233.223:6666");
    AVOutput *output = av_output_alloc_from_encoder(encoder, NULL, "test.h264");
    if (output == NULL) {
        return NULL;
    }
    return output;
}


int main(void) {
    av_log_set_level(AV_LOG_VERBOSE);

    AVDeviceInput *device_input = create_device_input();
    if (device_input == NULL) {
        return -1;
    }

    int encodeWidth;
    int encodeHeight;
    AVDeviceInputParams params = av_device_input_get_params(device_input);
    calculateScaledSize(params.width, params.height, 1920, 1080, 16, &encodeWidth, &encodeHeight);

    AVEncoder *encoder = create_encoder(encodeWidth, encodeHeight);
    if (encoder == NULL) {
        return -1;
    }

    AVFilters *filters = create_filters(params, encodeWidth, encodeHeight);
    if (filters == NULL) {
        return -1;
    }

    AVOutput *output = create_output(encoder);


    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();
    while (1) {
        av_frame_unref(frame);
        if (av_device_receive_frame(device_input, frame) >= 0) {
            frame->pict_type = AV_PICTURE_TYPE_NONE;
            frame->flags &= ~AV_FRAME_FLAG_KEY;
            av_filter_send_frame(filters, frame);
        }
        av_frame_unref(frame);
        if (av_filter_receive_frame(filters, frame) >= 0) {
            av_encoder_send_frame(encoder, frame);
        }
        av_packet_unref(packet);
        if (av_encoder_receive_packet(encoder, packet) >= 0) {
            // printf("av_encoder_receive_packet: %ld %d  %d\n", packet->pts, packet->flags & AV_PKT_FLAG_KEY,
            //        packet->size);
            av_output_send_packet(output, packet);
        }
    }

    return 0;
}
