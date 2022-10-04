#ifndef HEADER_H
#define HEADER_H

#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
# include "stdint.h"
#endif

extern "C"
{
#include "libavfilter/avfilter.h"
#include "libswresample/swresample.h"
#include "libpostproc/postprocess.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavformat/version.h"

#include "libavutil/avutil.h"
#include "libavutil/ffversion.h"
#include "libavutil/time.h"
#include "libavutil/mathematics.h"
#include "libavutil/imgutils.h"

#undef main

}

#endif // HEADER_H
