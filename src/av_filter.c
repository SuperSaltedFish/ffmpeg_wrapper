//
// Created by zhixingye on 2025/3/16.
//

#include "av_filter.h"

#include <libavutil/hwcontext.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>

AVFilters *av_filter_alloc(int src_width, int src_height, enum AVPixelFormat src_format, AVRational src_time_base) {
    char inArgsStream[64] = {};
    sprintf(inArgsStream, "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",
            src_width, src_height, src_format, src_time_base.num, src_time_base.den);
    return av_filter_alloc_by_buffer_params(inArgsStream);
}

AVFilters *av_filter_alloc_by_buffer_params(const char *buffer_filter_params) {
    const AVFilter *filter_buffer = avfilter_get_by_name("buffer");
    if (filter_buffer == NULL) {
        printf("av_filter_alloc: avfilter_get_by_name(%s) = NULL\n", "buffer");
        return NULL;
    }

    const AVFilter *filter_sink = avfilter_get_by_name("buffersink");
    if (filter_sink == NULL) {
        printf("av_filter_alloc: avfilter_get_by_name(%s) = NULL\n", "buffersink");
        return NULL;
    }

    AVFilters *av_filters = av_malloc(sizeof(AVFilters));
    av_filters->graph = avfilter_graph_alloc();
    if (av_filters->graph == NULL) {
        printf("av_filter_alloc: avfilter_graph_alloc = NULL\n");
        goto fail;
    }

    int ret = 0;

    ret = avfilter_graph_create_filter(&av_filters->ctx_buffer_in,
                                       filter_buffer,
                                       filter_buffer->name,
                                       buffer_filter_params,
                                       NULL,
                                       av_filters->graph);
    if (ret < 0) {
        printf("av_filter_alloc_by_buffer_params avfilter_graph_create_filter(buffer) = %s\n",av_err2str(ret));
        goto fail;
    }

    ret = avfilter_graph_create_filter(&av_filters->ctx_buffer_sinkOut,
                                       filter_sink,
                                       filter_sink->name,
                                       NULL,
                                       NULL,
                                       av_filters->graph);
    if (ret < 0) {
        printf("av_filter_alloc_by_buffer_params avfilter_graph_create_filter(buffersink) = %s\n",av_err2str(ret));
        goto fail;
    }

    return av_filters;

fail:
    av_filter_free(&av_filters);
    return NULL;
}

void av_filter_free(AVFilters **filters_addr) {
    if (filters_addr == NULL) {
        return;
    }
    AVFilters *filters = *filters_addr;
    if (filters->graph != NULL) {
        avfilter_graph_free(&filters->graph);
    }
    filters->ctx_buffer_in = NULL;
    filters->ctx_buffer_sinkOut = NULL;
    filters->ctx_custom_list = NULL;
    filters->custom_list_count = 0;
}

int av_filter_add_filter(AVFilters *filters,
                         const char *name,
                         const char *params) {
    return av_filter_add_filter_hw(filters, AV_HWDEVICE_TYPE_NONE,NULL, name, params);
}

int av_filter_add_filter_hw(AVFilters *filters,
                            enum AVHWDeviceType type,
                            const char *device_path,
                            const char *name,
                            const char *params) {
    const AVFilter *filter_custom = avfilter_get_by_name(name);
    if (filter_custom == NULL) {
        printf("av_filter_add_filter: avfilter_get_by_name(%s) = NULL\n", name);
        return -EINVAL;
    }


    AVFilterContext *ctx = avfilter_graph_alloc_filter(filters->graph,
                                                       filter_custom,
                                                       filter_custom->name);
    if (ctx == NULL) {
        printf("av_filter_add_filter: avfilter_graph_alloc_filter(%s) = NULL\n", name);
        return -EINVAL;
    }

    if (type != AV_HWDEVICE_TYPE_NONE) {
        AVBufferRef *hw_device_ctx = NULL;
        const int ret = av_hwdevice_ctx_create(&hw_device_ctx, type, device_path, NULL, 0);
        if (ret < 0) {
            printf("av_filter_add_filter av_hwdevice_ctx_create(%s) = %s\n", name,av_err2str(ret));
            avfilter_free(ctx);
            return ret;
        }
        ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    }

    const int ret = avfilter_init_str(ctx, params);
    if (ret < 0) {
        printf("av_filter_add_filter avfilter_init_str(%s) = %s\n", name,av_err2str(ret));
        if (ctx->hw_device_ctx != NULL) {
            av_buffer_unref(&ctx->hw_device_ctx);
        }
        avfilter_free(ctx);
        return ret;
    }

    if (filters->ctx_custom_list == NULL) {
        filters->custom_list_capacity = 2;
        filters->ctx_custom_list = malloc(sizeof(AVFilterContext *) * filters->custom_list_capacity);
    }
    if (filters->custom_list_capacity <= filters->custom_list_count) {
        filters->custom_list_capacity += 2;
        filters->ctx_custom_list = realloc(filters->ctx_custom_list,
                                           sizeof(AVFilterContext *) * filters->custom_list_capacity);
    }
    filters->ctx_custom_list[filters->custom_list_count++] = ctx;

    return 0;
}

int av_filter_link(AVFilters *filters) {
    for (int i = 0; i <= filters->custom_list_count; i++) {
        AVFilterContext *src;
        AVFilterContext *dest;
        if (i == 0) {
            src = filters->ctx_buffer_in;
            dest = filters->ctx_custom_list[i];
        } else if (i == filters->custom_list_count) {
            src = filters->ctx_custom_list[i - 1];
            dest = filters->ctx_buffer_sinkOut;
        } else {
            src = filters->ctx_custom_list[i - 1];
            dest = filters->ctx_custom_list[i];
        }
        const int ret = avfilter_link(src, 0, dest, 0);
        if (ret < 0) {
            printf("av_filter_link: avfilter_link(%s to %s) = %s\n", src->name, dest->name,av_err2str(ret));
            return ret;
        }
    }

    const int ret = avfilter_graph_config(filters->graph, NULL);
    if (ret < 0) {
        printf("av_filter_link avfilter_graph_config = %s\n",av_err2str(ret));
    }
    return ret;
}

int av_filter_send_frame(AVFilters *filters, AVFrame *frame) {
    const int ret = av_buffersrc_add_frame(filters->ctx_buffer_in, frame);
    if (ret < 0 && ret != -EAGAIN) {
        printf("av_filter_send_frame: av_buffersrc_add_frame = %s\n", av_err2str(ret));
    }
    return ret;
}

int av_filter_receive_frame(AVFilters *filters, AVFrame *frame) {
    const int ret = av_buffersink_get_frame(filters->ctx_buffer_sinkOut, frame);
    if (ret < 0 && ret != -EAGAIN) {
        printf("av_filter_send_frame: av_buffersrc_add_frame = %s\n", av_err2str(ret));
    }
    return ret;
}
