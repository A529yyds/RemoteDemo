#include "ExampleDemo.h"
#include <cstdio>
#include <chrono>
#include <Processing.NDI.Advanced.h>
#include <QDebug>
#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.Advanced.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.Advanced.x86.lib")
#endif // _WIN64
#endif // _WIN32

#include "OptFFmpeg.h"
#include "Sender.h"

//class OptFFmpeg;
OptFFmpeg * _opt = nullptr;

static std::atomic<bool> exit_loop(false);
static void sigint_handler(int)
{
    exit_loop = true;
}

template <typename FrameType>
uint32_t get_max_frame_size(const FrameType* p_frames, int num_frames)
{
    uint32_t max_frame_size = 0;

    // Iterate to find the largest frame size
    for (int i = 0; i != num_frames; i++)
        max_frame_size = std::max(max_frame_size, p_frames[i].data_size + p_frames[i].extra_size);

    return max_frame_size;
}

void init_video_data(video_send_data* p_send_data, int num_frames, const stream_info_type& stream_info, NDIlib_FourCC_video_type_ex_e stream_type)
{
    for (int i = 0; i != num_frames; i++) {
        NDIlib_video_frame_v2_t& frame = p_send_data[i].frame;

        // The following data is pretty much constant throughout the stream
        frame.FourCC = (NDIlib_FourCC_video_type_e)stream_type;
        frame.xres = stream_info.xres;
        frame.yres = stream_info.yres;
        frame.p_data = nullptr;
        frame.data_size_in_bytes = 0;
        frame.frame_rate_N = stream_info.frame_rate_n;
        frame.frame_rate_D = stream_info.frame_rate_d;
        frame.frame_format_type = NDIlib_frame_format_type_progressive;
        frame.picture_aspect_ratio = (float)stream_info.aspect_ratio_n / (float)stream_info.aspect_ratio_d;
    }
}

void setup_frame(int frame_num, const stream_info_type& stream_info, video_send_data& dst_data)
{
    const video_frame& src_frame = stream_info.p_frames[frame_num];

    // You should make sure that your timecode value is something "correct". Our display time-stamp is in the
    // 100ns format already so we can just use this as the timecode.
    dst_data.frame.timecode = src_frame.dts;

    // The compressed frame buffer needs to begin with the NDIlib_compressed_packet_t header. Note that we do
    // support arbitrary PTS and DTS values and we support full IBP frame sequences, we do not use the values
    // for anything other than correctly re-ordering frames so that they can be decoded and displayed
    // correctly; meaning that the time-base is not important. With all this said, it is somewhat our
    // recommendation to not use B frames since these increase the latency. Theoretically we would support
    // GOPs of almost any length although I-frames must be inserted when the SDK asks you too.
    dst_data.packet.version = sizeof(NDIlib_compressed_packet_t);
    dst_data.packet.pts = src_frame.pts;
    dst_data.packet.dts = src_frame.dts;
    dst_data.packet.flags = src_frame.is_keyframe ? NDIlib_compressed_packet_t::flags_keyframe : NDIlib_compressed_packet_t::flags_none;
    dst_data.packet.data_size = src_frame.data_size;
    dst_data.packet.extra_data_size = src_frame.extra_size;
    dst_data.packet.fourCC = NDIlib_compressed_FourCC_type_HEVC;

    // Ensure the scatter-gather lists are cleared for the sending of the next frame
    dst_data.scatter_data.clear();
    dst_data.scatter_data_size.clear();

    // Push the compressed packet structure to the scatter-gather lists
    dst_data.scatter_data.push_back((uint8_t*)&dst_data.packet);
    dst_data.scatter_data_size.push_back((int)sizeof(NDIlib_compressed_packet_t));

    // Push the actual frame data to the scatter-gather lists
    dst_data.scatter_data.push_back((uint8_t*)src_frame.p_data);
    dst_data.scatter_data_size.push_back((int)src_frame.data_size);

    // Push any required extra data to the scatter-gather lists
    if (src_frame.extra_size != 0) {
        dst_data.scatter_data.push_back((uint8_t*)src_frame.p_extra);
        dst_data.scatter_data_size.push_back((int)src_frame.extra_size);
    }

    // NULL terminate both lists
    dst_data.scatter_data.push_back(nullptr);
    dst_data.scatter_data_size.push_back(0);
}

void send_aac_audio(NDIlib_send_instance_t pNDI_send)
{
    // How many frames do we have total?
    const int num_frames = aac::num_audio_frames;

    // Initialize the packet structure which would be the header data for each audio frame
    NDIlib_compressed_packet_t packet = {};
    packet.version = sizeof(NDIlib_compressed_packet_t);
    packet.fourCC = NDIlib_compressed_FourCC_type_AAC;
    packet.flags = NDIlib_compressed_packet_t::flags_keyframe;

    // The following data is pretty much constant throughout the stream. Other fields will be calculated on
    // a per-frame basis.
    NDIlib_audio_frame_v3_t dst_frame = {};
    dst_frame.sample_rate = aac::audio_sample_rate;
    dst_frame.no_channels = aac::audio_num_channels;
    dst_frame.no_samples = aac::audio_num_samples;
    dst_frame.FourCC = (NDIlib_FourCC_audio_type_e)NDIlib_FourCC_audio_type_ex_AAC;

    std::vector<uint8_t*> scatter_data;
    std::vector<int>      scatter_data_size;

    // Keep sending frames until SIGINT arrives
    for (int frame_num = 0; !exit_loop; frame_num = (frame_num + 1) % num_frames) {
        const audio_frame& src_frame = aac::audio_frames[frame_num];

        // You should make sure that your timecode value is something "correct". Our display time-stamp is in
        // the 100ns format already so we can just use this as the timecode.
        dst_frame.timecode = src_frame.dts;

        // Initialize the non-const data of the packet header
        packet.pts = src_frame.pts;
        packet.dts = src_frame.dts;
        packet.data_size = src_frame.data_size;
        packet.extra_data_size = src_frame.extra_size;

        // Ensure the scatter-gather lists are cleared for the sending of the next frame
        scatter_data.clear();
        scatter_data_size.clear();

        // Push the compressed packet structure to the scatter-gather lists
        scatter_data.push_back((uint8_t*)&packet);
        scatter_data_size.push_back((int)sizeof(NDIlib_compressed_packet_t));

        // Push the actual frame data to the scatter-gather lists
        scatter_data.push_back((uint8_t*)src_frame.p_data);
        scatter_data_size.push_back((int)src_frame.data_size);

        // Push any required extra data to the scatter-gather lists
        if (src_frame.extra_size != 0) {
            scatter_data.push_back((uint8_t*)src_frame.p_extra);
            scatter_data_size.push_back((int)src_frame.extra_size);
        }

        // NULL terminate both lists
        scatter_data.push_back(nullptr);
        scatter_data_size.push_back(0);

        // These are the scatter-gather lists to use
        NDIlib_frame_scatter_t frame_scatter = {scatter_data.data(), scatter_data_size.data()};

        // Send the AAC audio frame
        NDIlib_send_send_audio_scatter(pNDI_send, &dst_frame, &frame_scatter);
    }
}

#include <fstream>
#include <iostream>
int iCurTimes = 0;
bool bGetKey2File = false;

//Sender *_sender = new Sender("senderDemo", nullptr);
void sendFrame(int frame_num, const stream_info_type& stream_info)
{
    const video_frame& src_frame = stream_info.p_frames[frame_num];
//    _sender->SendFrame(src_frame);
}

int senderStart()
{
    signal(SIGINT, sigint_handler);
    if (!NDIlib_initialize())
        return 0;
    NDIlib_send_create_t NDI_send_description;
    NDI_send_description.p_ndi_name = "HEVC HX Test";
    NDI_send_description.clock_video = true;
    NDI_send_description.clock_audio = true;
    NDIlib_send_instance_t pNDI_send = NDIlib_send_create_v2(&NDI_send_description, /* See note above */nullptr);
    if (!pNDI_send) return 0;
    std::thread audio_thread(send_aac_audio, pNDI_send);
    const int num_frames = video_highQ_info.num_frames;
    video_send_data highQ_data[2];
    init_video_data(highQ_data, 2, video_highQ_info, NDIlib_FourCC_video_type_ex_HEVC_highest_bandwidth);
    _opt = new OptFFmpeg(1920, 1080, nullptr);

    for (int frame_num = 0, send_frame = 0; !exit_loop; frame_num = (frame_num + 1) % num_frames) {
        if (NDIlib_send_is_keyframe_required(pNDI_send)) {
            frame_num = 0;
        }
        sendFrame(frame_num, video_highQ_info);

        if(iCurTimes == 30)
            break;

        if(bGetKey2File)
            iCurTimes++;

        uint32_t bKey = video_highQ_info.p_frames[frame_num].is_keyframe ? NDIlib_compressed_packet_t::flags_keyframe : NDIlib_compressed_packet_t::flags_none;

        if(bKey == 1 && frame_num > 300)
        {
            qDebug() << "send write frame num is " << frame_num;
            QByteArray f(reinterpret_cast<const char*>(video_highQ_info.p_frames[frame_num].p_data), video_highQ_info.p_frames[frame_num].data_size);
            _opt->Write1Frame2File(f.size(), f.data(), "sendFrame");
            bGetKey2File = true;
        }
        send_frame = (send_frame + 1) % 2;
    }
    NDIlib_send_send_video_scatter_async(pNDI_send, nullptr, nullptr);
    audio_thread.join();
    NDIlib_send_destroy(pNDI_send);
    NDIlib_destroy();
    if(_opt)
    {
        delete _opt;
        _opt = nullptr;
    }
    return 0;
}

int senderStart2()
{
    signal(SIGINT, sigint_handler);
    if (!NDIlib_initialize())
        return 0;
    NDIlib_send_create_t NDI_send_description;
    NDI_send_description.p_ndi_name = "HEVC HX Test";
    NDI_send_description.clock_video = true;
    NDI_send_description.clock_audio = true;
    NDIlib_send_instance_t pNDI_send = NDIlib_send_create_v2(&NDI_send_description, /* See note above */nullptr);
    if (!pNDI_send) return 0;
    std::thread audio_thread(send_aac_audio, pNDI_send);
    const int num_frames = video_highQ_info.num_frames;
    video_send_data highQ_data[2];
    init_video_data(highQ_data, 2, video_highQ_info, NDIlib_FourCC_video_type_ex_HEVC_highest_bandwidth);
    _opt = new OptFFmpeg(1920, 1080, nullptr);
    for (int frame_num = 0, send_frame = 0; !exit_loop; frame_num = (frame_num + 1) % num_frames) {
        if (NDIlib_send_is_keyframe_required(pNDI_send)) {
            frame_num = 0;
        }
        setup_frame(frame_num, video_highQ_info, highQ_data[send_frame]);

        NDIlib_video_frame_v2_t& highQ_frame = highQ_data[send_frame].frame;
        NDIlib_frame_scatter_t   highQ_scatter = {highQ_data[send_frame].scatter_data.data(), highQ_data[send_frame].scatter_data_size.data()};
        const int highQ_frame_size = NDIlib_send_get_target_frame_size(pNDI_send, &highQ_frame);
//        qDebug() << "sender demo key frame status:" << highQ_data[send_frame].packet.flags << ".\n";
        if(iCurTimes == 30)
            break;

        if(bGetKey2File)
            iCurTimes++;

        if(highQ_data[send_frame].packet.flags == 1 && frame_num > 300)
        {
            qDebug() << "send write frame num is " << frame_num << ", pts is " << highQ_data[send_frame].packet.pts;
            QByteArray f(reinterpret_cast<const char*>(highQ_scatter.p_data_blocks[1]), highQ_scatter.p_data_blocks_size[1]);
            _opt->Data2Cache(f);
            bGetKey2File = true;
        }
        NDIlib_send_send_video_scatter_async(pNDI_send, &highQ_frame, &highQ_scatter);
        send_frame = (send_frame + 1) % 2;
    }
    _opt->Cache2File("hqSendKeyFrame");
    if(_opt)
    {
        delete _opt;
        _opt = nullptr;
    }
    NDIlib_send_send_video_scatter_async(pNDI_send, nullptr, nullptr);
    audio_thread.join();
    NDIlib_send_destroy(pNDI_send);
    NDIlib_destroy();
    // Finished
    return 0;
}


int senderStart1()
{
    // Catch interrupt so that we can shut down gracefully
    signal(SIGINT, sigint_handler);

    // Not required, but "correct" (see the SDK documentation).
    if (!NDIlib_initialize())
        return 0;

    // We create the NDI sender properties. Please note that for this example we are going to allow the SDK
    // to clock the sending and receiving. It is likely that if you have a device which has it's own video
    // clock that delivers video at the correct rate that you would specify these as false.
    NDIlib_send_create_t NDI_send_description;
    NDI_send_description.p_ndi_name = "HEVC HX Test";
    NDI_send_description.clock_video = true;
    NDI_send_description.clock_audio = true;

    // Now create the sender. Note that for normal implementations you would specify your configuration
    // settings in the separate parameters per the SDK documentation. Most likely these would at very least
    // include your vendor-id which is needed for product use of the NDI Advanced SDK. You can however test
    // without out. This configuration allows all NDI settings to be configured as needed (UDP mode, etc...)
    // although if it is not present then it falls back to the default parameters in a configuration file.
    NDIlib_send_instance_t pNDI_send = NDIlib_send_create_v2(&NDI_send_description, /* See note above */nullptr);
    if (!pNDI_send) return 0;

    // Begin sending AAC audio on its own thread
    std::thread audio_thread(send_aac_audio, pNDI_send);

    // How many frames do we have total? In this example, the low quality and high quality structures should
    // have the same number of frames.
    const int num_frames = video_highQ_info.num_frames;

    // Initialize some video frames for each stream for asynchronous sending
    video_send_data highQ_data[2], lowQ_data[3];
    init_video_data(highQ_data, 2, video_highQ_info, NDIlib_FourCC_video_type_ex_HEVC_highest_bandwidth);
    init_video_data(lowQ_data, 3, video_lowQ_info, NDIlib_FourCC_video_type_ex_HEVC_lowest_bandwidth);

    // Keep sending frames until SIGINT arrives
    std::ofstream out_file("demosenddata.h265", std::ios::binary);
    if (!out_file.is_open()) {
        std::cerr << "无法打开输出文件" << std::endl;
    }

    for (int frame_num = 0, send_frame = 0; !exit_loop; frame_num = (frame_num + 1) % num_frames) {
        // In this test code, if a NDI sender instance needs a keyframe, we just go back to the animation. In
        // a real application this is NOT what you would do. Instead you will tell your compressor to
        // generate a new I-Frame as quickly as it can. You may insert I-Frames at regular intervals yourself
        // if you want, this function simply tells you when you MUST insert one very quickly (within <100ms).
        // This call will return true when there are new connections, there are connections that dropped
        // frames (e.g. we correctly detect UDP drops and request a new I-Frame to avoid having corrupted
        // video, etc...). We support any IBP frame sequences and support very long GOPs. Our general
        // recommendation however would be that it is best to use I and P frames to keep decode latency lower
        // (B frames add delay). You can write out multi-second (or even longer) GOPs if you want, allowing
        // the bandwidth to be very low indeed. NDI will request an I-Frame whenever it is truly needed for
        // the connection to correctly decode a stream.
        if (NDIlib_send_is_keyframe_required(pNDI_send)) {
            // Normally we would tell the compressor to insert a new I-Frame here. For our example we just
            // jump back to the beginning frame since we know this was an I-Frame. This is NOT what you would
            // do in production code, this is just an example that does not include support for a HW encoder.
            frame_num = 0;
        }

        // Prepare the next high quality frame to be sent out. This should be a full resolution frame at the
        // quality and format that your device supports and the user has selected. Dynamically changing video
        // formats are fully supported although you should of course insert an I-Frame when you change format.
        setup_frame(frame_num, video_highQ_info, highQ_data[send_frame]);
        NDIlib_video_frame_v2_t& highQ_frame = highQ_data[send_frame].frame;
        NDIlib_frame_scatter_t   highQ_scatter = {highQ_data[send_frame].scatter_data.data(), highQ_data[send_frame].scatter_data_size.data()};

        // Prepare the next low quality frame to be sent out. This MUST be a 640 wide preview resolution
        // stream. At 16:9 this would mean typically that the frame resolution is 640x360 if you assume
        // square pixels. We do not enforce that this is timed in sync with the high quality stream although
        // it is preferable. It is fine if you wish to run this stream at a lower frame rate (e.g. 30Hz
        // instead of 60Hz) although once again it is preferable if it is a full frame rate stream.
//        setup_frame(frame_num, video_lowQ_info, lowQ_data[send_frame]);
//        NDIlib_video_frame_v2_t& lowQ_frame = lowQ_data[send_frame].frame;
//        NDIlib_frame_scatter_t   lowQ_scatter = {lowQ_data[send_frame].scatter_data.data(), lowQ_data[send_frame].scatter_data_size.data()};

        // Estimate the current bit rate of the frames. Note that because this example does not include a
        // compressor we ignore it, however we keep this here since you most likely want to use it. This does
        // not take into account IBP frame sequences, etc... but instead gives you an average frame size that
        // respects what "typical NDI|HX devices use".
        const int highQ_frame_size = NDIlib_send_get_target_frame_size(pNDI_send, &highQ_frame);
//        const int lowQ_frame_size = NDIlib_send_get_target_frame_size(pNDI_send, &lowQ_frame);

        // Send both frames asynchronously so that we can begin preparing the next frames. We are using
        // asynchronous sending here which is defined in the SDK documentation. A call to the SDK will return
        // immediately and the frame data passed into the function should not be changed until the next call
        // to send_video_async. In other words, when the function return from NDIlib_send_send_video_async_v2
        // then all buffers from previous calls are no longer in use by the SDK.
        out_file.write(reinterpret_cast<const char*>(highQ_scatter.p_data_blocks[1]), highQ_scatter.p_data_blocks_size[1]);
        out_file.flush();
        if(frame_num % 10 == 0)
            qDebug() << "current send frame = " << frame_num;

        NDIlib_send_send_video_scatter_async(pNDI_send, &highQ_frame, &highQ_scatter);
//        NDIlib_send_send_video_scatter_async(pNDI_send, &lowQ_frame, &lowQ_scatter);

        // Advance the next frame buffer
        send_frame = (send_frame + 1) % 2;
        if(iCurTimes == 3 && frame_num == (num_frames - 1))
            exit_loop = true;
        if(frame_num == (num_frames - 1))
            iCurTimes++;

    }
    out_file.close();


    // Ensure all async operations have completed, especially before releasing the streams' memory. Note that
    // this ensures that there are no buffers in flight in the SDK.
    NDIlib_send_send_video_scatter_async(pNDI_send, nullptr, nullptr);

    // Wait for the audio thread to complete
    audio_thread.join();

    // Destroy the NDI sender
    NDIlib_send_destroy(pNDI_send);

    // Not required, but nice
    NDIlib_destroy();

    // Finished
    return 0;
}

bool bWriteOver= false;
int receiverStart()
{
    // Not required, but "correct" (see the SDK documentation).
    if (!NDIlib_initialize())
        return 0;

    // Create a finder
    NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2();
    if (!pNDI_find)
        return 0;

    // Wait until there is one source
    uint32_t no_sources = 0;
    const NDIlib_source_t* p_sources = NULL;
    while (!no_sources) {
        // Wait until the sources on the network have changed
        qDebug()<< "Looking for sources ...\n";
        NDIlib_find_wait_for_sources(pNDI_find, 5000/* One second */);
        p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);
    }

    // Setup the NDI receiver to pass through compressed audio and video frames.
    NDIlib_recv_create_v3_t NDI_recv_create_desc = {};
    NDI_recv_create_desc.color_format = (NDIlib_recv_color_format_e)NDIlib_recv_color_format_compressed_v4_with_audio;

    // We now have at least one source, so we create a receiver to look at it.
    NDIlib_recv_instance_t pNDI_recv = NDIlib_recv_create_v3(&NDI_recv_create_desc);
    if (!pNDI_recv)
        return 0;

    // Connect to our sources
    NDIlib_recv_connect(pNDI_recv, p_sources + 0);

    // Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
    NDIlib_find_destroy(pNDI_find);

    // Run for one minute
    using namespace std::chrono;

    uint8_t* pData;
    size_t iSize;
    _opt = new OptFFmpeg(1920, 1080, nullptr);
    for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < minutes(1);) {
        // The descriptors
        NDIlib_video_frame_v2_t video_frame;
        NDIlib_audio_frame_v2_t audio_frame;
        NDIlib_metadata_frame_t metadata_frame;
        switch (NDIlib_recv_capture_v2(pNDI_recv, &video_frame, &audio_frame, &metadata_frame, 100)) {
        // No data
        case NDIlib_frame_type_none:
        {
            if(!bWriteOver)
            {
                _opt->Cache2File("hqRecvKeyFrame");
                bWriteOver = true;
            }

            qDebug() << "No data received.\n";
            break;
        }
            // Video data
            case NDIlib_frame_type_video:
        {
                qDebug()<< "Video data received (" << video_frame.xres << video_frame.yres<<").\n";
                if (video_frame.p_metadata) {
                    // XML video metadata information.
                    qDebug() << "XML video metadata" << video_frame.p_metadata << ".\n";
                }
                if (video_frame.p_data) {
                    // Video frame buffer.
                    qDebug() << "video_frame received.\n";
                    // Check video_frame.FourCC for the pixel format of this buffer
                    NDIlib_compressed_packet_t* p_packet = (NDIlib_compressed_packet_t*)(video_frame.p_data);
                    if(p_packet && p_packet->flags == 1)
                    {
                        pData = (uint8_t*)(p_packet + 1);
                        iSize = p_packet->data_size;
                        qDebug() << "recv write frame pts is " << p_packet->pts;
                        QByteArray f(reinterpret_cast<const char*>(pData),iSize);
                        _opt->Data2Cache(f);
                    }
                }
                NDIlib_recv_free_video_v2(pNDI_recv, &video_frame);
                break;
        }
            // Audio data
            case NDIlib_frame_type_audio:
                printf("Audio data received (%d samples).\n", audio_frame.no_samples);
                if (audio_frame.p_data) {
                    // Audio frame buffer.
                }
                NDIlib_recv_free_audio_v2(pNDI_recv, &audio_frame);
                break;

            // Metadata
            case NDIlib_frame_type_metadata:
                if (metadata_frame.p_data) {
                    // XML metadata information
                    printf("Received metadata %s\n", metadata_frame.p_data);
                }
                NDIlib_recv_free_metadata(pNDI_recv, &metadata_frame);
                break;

            // There is a status change on the receiver
            case NDIlib_frame_type_status_change:
                printf("Receiver connection status changed.\n");
                break;
        }
    }
    if(_opt)
    {
        delete _opt;
        _opt = nullptr;
    }
    // Destroy the receiver
    NDIlib_recv_destroy(pNDI_recv);

    // Not required, but nice
    NDIlib_destroy();

    // Finished
    return 0;
}
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

#include <Processing.NDI.Advanced.h>

void check_error(int ret) {
    if (ret < 0) {
        char errbuf[256];
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Error: " << errbuf << std::endl;
        exit(1);
    }
}

int senderNdiStart()
{
    // 初始化NDI
       if (!NDIlib_initialize()) {
           std::cerr << "Cannot initialize NDI." << std::endl;
           return -1;
       }

       // 创建发送实例
       NDIlib_send_create_t send_create_desc = {};
       send_create_desc.p_ndi_name = "HEVC_NVENC_NDI_Sender";
       NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&send_create_desc);
       if (!pNDI_send) {
           std::cerr << "Failed to create NDI sender." << std::endl;
           return -1;
       }

       // 查找编码器
       const AVCodec* codec = avcodec_find_encoder_by_name("hevc_nvenc");
       if (!codec) {
           std::cerr << "Codec hevc_nvenc not found." << std::endl;
           return -1;
       }

       AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
       if (!codec_ctx) {
           std::cerr << "Could not allocate codec context." << std::endl;
           return -1;
       }

       // 设置编码参数（分辨率、帧率、像素格式）
       codec_ctx->width = 1280;
       codec_ctx->height = 720;
       codec_ctx->time_base = AVRational{1, 30};  // 30 fps
       codec_ctx->framerate = AVRational{30, 1};
       codec_ctx->pix_fmt = AV_PIX_FMT_NV12;      // NVENC推荐的格式
       codec_ctx->max_b_frames = 0;                // 低延迟
       codec_ctx->bit_rate = 4000000;              // 4 Mbps

       // 需要设置参数让 NVENC 输出封装好的 VPS/SPS/PPS（默认通常会输出）
       // 也可以设置 preset 等参数
       av_opt_set(codec_ctx->priv_data, "preset", "llhq", 0); // 低延迟高质量

       int ret = avcodec_open2(codec_ctx, codec, nullptr);
       check_error(ret);

       // 假设有一帧原始图像输入data，你要编码
       AVFrame* frame = av_frame_alloc();
       frame->format = codec_ctx->pix_fmt;
       frame->width = codec_ctx->width;
       frame->height = codec_ctx->height;
       ret = av_frame_get_buffer(frame, 32);
       check_error(ret);

       // 这里省略填充frame数据的代码（你需要转换你的RGB或屏幕采集到的图像填入frame）

       AVPacket* pkt = av_packet_alloc();

       int frame_index = 0;

       while (true) { // 假设循环获取图像帧编码发送
           // 1. 填充frame数据

           frame->pts = frame_index++;

           ret = avcodec_send_frame(codec_ctx, frame);
           if (ret < 0) {
               std::cerr << "Error sending frame to encoder." << std::endl;
               break;
           }

           while (ret >= 0) {
               ret = avcodec_receive_packet(codec_ctx, pkt);
               if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                   break;
               }
               check_error(ret);

               // 检查是否关键帧
               bool is_keyframe = (pkt->flags & AV_PKT_FLAG_KEY) != 0;

               // 发送压缩数据包给 NDI HX3
//               NDIlib_compressed_video_frame_v2_t ndi_frame = {};
//               ndi_frame.p_data = pkt->data;
//               ndi_frame.data_size = pkt->size;
//               ndi_frame.FourCC = NDIlib_FourCC_type_HEVC;  // HEVC编码
//               ndi_frame.frame_format_type = NDIlib_frame_format_type_progressive;
//               ndi_frame.timecode = NDIlib_send_timecode_synthesize; // 自动时间码
//               ndi_frame.frame_number = frame_index;
//               ndi_frame.frame_type = is_keyframe ? NDIlib_frame_type_keyframe : NDIlib_frame_type_video;

//               // 这里不发送extradata，依赖关键帧内包含VPS/SPS/PPS
//               ndi_frame.p_metadata = nullptr;
//               ndi_frame.metadata_size = 0;

//               bool send_ret = NDIlib_send_send_compressed_video_v2(pNDI_send, &ndi_frame);
//               if (!send_ret) {
//                   std::cerr << "NDI send failed." << std::endl;
//               }

               av_packet_unref(pkt);
           }

           // 这里你加终止条件或者继续
       }

       av_packet_free(&pkt);
       av_frame_free(&frame);
       avcodec_free_context(&codec_ctx);

       NDIlib_send_destroy(pNDI_send);
       NDIlib_destroy();

       return 0;
}
