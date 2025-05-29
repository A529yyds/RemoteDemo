#ifndef RECEIVER_H
#define RECEIVER_H
#include <QObject>
#include <cstddef>
#include <Processing.NDI.Advanced.h>


class Receiver: public QObject
{
    Q_OBJECT
public:
    Receiver(QObject *obj = nullptr);
    ~Receiver();
    void GetFrame(NDIlib_video_frame_v2_t &frame);
    void GetCompressedPacket();

private:
    NDIlib_recv_instance_t _ndiRecv;
};

#endif // RECEIVER_H
