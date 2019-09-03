#ifndef RXPC_PROTO_H
#define RXPC_PROTO_H

#define RXPC_MSG_VERSION 1
#define RXPC_MSG_MAGIC 0x29B00B92

struct rxpc_msg_header {
    uint32_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t flags;
    uint64_t length;
    uint64_t msg_id;
};

// RXPC_TYPE_HELLO - first command sent on a stream, some sort of an early handshake.
#define RXPC_TYPE_HELLO 0

// RXPC_FLAG_REPLY_CHANNEL - for use with RXPC_TYPE_HELLO: specifies the channel opened is a reply channel
#define RXPC_FLAG_REPLY_CHANNEL 0x40


#endif //RXPC_PROTO_H
