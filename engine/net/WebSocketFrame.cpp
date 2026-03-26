#include "net/WebSocketFrame.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <algorithm>
#include <iterator>

std::vector<uint8_t> WebSocketFrame::serialize() const {
    std::vector<uint8_t> out_buf;

    uint8_t fin_rsv_opcode = ((fin ? 0x80 : 0x00) | ((rsv & 0x07) << 4) | (opcode & 0x0F));
    out_buf.push_back(fin_rsv_opcode);

    out_buf.push_back(len_code | (mask ? 0x80 : 0x00));

    std::vector<uint8_t> lenbuf;
    if (len_code == 126) {
        uint16_t net_order = htons((uint16_t)(len & 0xFFFF));
        lenbuf.resize(2);
        std::memcpy(lenbuf.data(), &net_order, sizeof(net_order));
    }
    else if (len_code == 127) {
        uint64_t net_order = host_to_net_64(len);
        lenbuf.resize(8);
        std::memcpy(lenbuf.data(), &net_order, sizeof(net_order));
    }
    out_buf.insert(out_buf.end(), lenbuf.begin(), lenbuf.end());

    if (mask) {
        std::vector<uint8_t> maskbuf(4);
        uint32_t mask_net = htonl(mask_key);
        std::memcpy(maskbuf.data(), &mask_net, sizeof(mask_net));
        out_buf.insert(out_buf.end(), maskbuf.begin(), maskbuf.end());
    }

    out_buf.reserve(out_buf.size() + data.size());

    if (mask) {
        auto key = mask_key;
        size_t idx = 0;
        std::transform(data.begin(), data.end(), std::back_inserter(out_buf), [&key, &idx](uint8_t c) {
            int shift = 24 - ((idx % 4) * 8);
            uint8_t mask_byte = static_cast<uint8_t>((key >> shift) & 0xFF);
            ++idx;
            return static_cast<uint8_t>(c ^ mask_byte);
        });
    }
    else {
        out_buf.insert(out_buf.end(), data.begin(), data.end());
    }

    return out_buf;
}
