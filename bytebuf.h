#include <string.h>
#include <stdint.h>


class ByteBuf {
public:
    ByteBuf() {
        _len = 0;
    }

    inline void appendBytes(void *bytes, size_t len) {
        memcpy(_bytes + _len, bytes, len);
        _len += len;
    }

    inline uint16_t len() { return _len; }
    inline uint8_t* buf() { return _bytes; }

private:
    uint16_t _len;
    uint8_t _bytes[65536];
};
