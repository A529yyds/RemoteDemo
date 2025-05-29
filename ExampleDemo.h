#ifndef EXAMPLEDEMO_H
#define EXAMPLEDEMO_H

#include <QObject>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>
#include <Processing.NDI.Advanced.h>
#include "Sender.h"


// Basic audio frame structure for the pre-compressed AAC frames
struct audio_frame {
    const uint8_t* p_data;
    const uint8_t* p_extra;
    int64_t  dts;
    int64_t  pts;
    uint32_t data_size;
    uint32_t extra_size;
};
// Include the full resolution test data
namespace highQ {
#include "ndi/hevc_highQ.h"
}

// Include the preview resolution test data
namespace lowQ {
#include "ndi/hevc_lowQ.h"
}

// Include the AAC test data
namespace aac {
#include "ndi/aac.h"
}

static const stream_info_type video_lowQ_info = {
    lowQ::video_xres, lowQ::video_yres,
    lowQ::video_frame_rate_n, lowQ::video_frame_rate_d,
    lowQ::video_aspect_ratio_n, lowQ::video_aspect_ratio_d,
    lowQ::num_video_frames,
    lowQ::video_frames
};

static const stream_info_type video_highQ_info = {
    highQ::video_xres, highQ::video_yres,
    highQ::video_frame_rate_n, highQ::video_frame_rate_d,
    highQ::video_aspect_ratio_n, highQ::video_aspect_ratio_d,
    highQ::num_video_frames,
    highQ::video_frames
};

int senderStart();
int receiverStart();

int senderNdiStart();

#endif // EXAMPLEDEMO_H
