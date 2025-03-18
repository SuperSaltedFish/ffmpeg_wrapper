#ifndef AV_UTIL_H
#define AV_UTIL_H

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

inline void thread_sleep(long millisecond) {
#ifdef _WIN32
    Sleep(millisecond);
#else
    struct timespec ts = {millisecond / 1000, (millisecond % 1000) * 1000000};
    nanosleep(&ts, NULL);
#endif
}

static void calculateScaledSize(const int inputWidth,
                         const int inputHeight,
                         const int maxWidth,
                         const int maxHeight,
                         const int byteAlignment,
                         int *outputWidth,
                         int *outputHeight) {
    const double ratio = (double) inputWidth / inputHeight;
    if (inputWidth > maxWidth || inputHeight > maxHeight) {
        if (ratio >= (double) maxWidth / maxHeight) {
            *outputWidth = maxWidth;
            *outputHeight = (int) (maxWidth / ratio);
        } else {
            *outputHeight = maxHeight;
            *outputWidth = (int) (maxHeight * ratio);
        }
    } else {
        *outputWidth = inputWidth;
        *outputHeight = inputHeight;
    }
    if (byteAlignment > 1) {
        *outputWidth -= *outputWidth & byteAlignment;
        *outputHeight -= *outputHeight & byteAlignment;
    }
}


#endif //AV_UTIL_H
