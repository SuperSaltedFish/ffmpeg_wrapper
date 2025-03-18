#ifndef AV_FILTER_H
#define AV_FILTER_H
#include <libavfilter/avfilter.h>
#include <libavutil/hwcontext.h>

typedef struct AVFilters {
    AVFilterGraph *graph;
    AVFilterContext *ctx_buffer_in;
    AVFilterContext *ctx_buffer_sinkOut;
    AVFilterContext **ctx_custom_list;
    int custom_list_capacity;
    int custom_list_count;
} AVFilters;

AVFilters *av_filter_alloc(int src_width,
                           int src_height,
                           enum AVPixelFormat src_format,
                           AVRational src_time_base);

void av_filter_free(AVFilters **filters_addr);

AVFilters *av_filter_alloc_by_buffer_params(const char *buffer_filter_params);

int av_filter_add_filter(AVFilters *filters,
                         const char *name,
                         const char *params);

int av_filter_add_filter_hw(AVFilters *filters,
                         enum AVHWDeviceType type,
                         const char *device_path,
                         const char *name,
                         const char *params);

int av_filter_link(AVFilters *filters);

int av_filter_send_frame(AVFilters *filters, AVFrame *frame);

int av_filter_receive_frame(AVFilters *filters, AVFrame *frame);


#endif //AV_FILTER_H
