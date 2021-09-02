 #ifndef RECORD_AV_QUEUE_H
#define RECORD_AV_QUEUE_H


#include "common.h"






 int packet_queue_put_private(PacketQueue *q, AVPacket *pkt,int64_t frame_pts);


 int packet_queue_put(PacketQueue *q, AVPacket *pkt,int64_t frame_pts);


 int packet_queue_put_nullpacket(PacketQueue *q, int stream_index);


/* packet queue handling */
 int packet_queue_init(PacketQueue *q);


 void packet_queue_flush(PacketQueue *q);


 void packet_queue_destroy(PacketQueue *q);


 void packet_queue_abort(PacketQueue *q);


 void packet_queue_start(PacketQueue *q);


/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
 int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block,int64_t *frame_pts);



#endif//RECORD_AV_QUEUE_H
