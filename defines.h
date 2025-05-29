#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>
#define W_4K 2560
#define H_4K 1440
#define DELETE(a) if(a){delete a;a=nullptr;}


struct video_frame {
    const uint8_t* p_data;
    const uint8_t* p_extra;
    int64_t  dts;
    int64_t  pts;
    uint32_t data_size;
    uint32_t extra_size;
    bool     is_keyframe;
};

#endif // DEFINES_H
