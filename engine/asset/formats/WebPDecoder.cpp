#include "asset/ImageDecoder.h"

#include <webp/decode.h>
#include <webp/demux.h>

#include <cstring>

class WebPImageDecoder : public ImageDecoder {
  public:
    std::vector<std::string> extensions() const override { return {"webp"}; }

    std::vector<ImageFrame> decode(const uint8_t* data, size_t size) const override {
        std::vector<ImageFrame> frames;

        WebPData webp_data = {data, size};
        WebPDemuxer* demux = WebPDemux(&webp_data);
        if (!demux)
            return frames;

        uint32_t canvas_w = WebPDemuxGetI(demux, WEBP_FF_CANVAS_WIDTH);
        uint32_t canvas_h = WebPDemuxGetI(demux, WEBP_FF_CANVAS_HEIGHT);
        WebPIterator iter;

        if (WebPDemuxGetFrame(demux, 1, &iter)) {
            do {
                int w, h;
                uint8_t* rgba = WebPDecodeRGBA(iter.fragment.bytes, iter.fragment.size, &w, &h);
                if (rgba && w > 0 && h > 0) {
                    ImageFrame f;
                    f.width = (int)canvas_w;
                    f.height = (int)canvas_h;
                    f.duration_ms = iter.duration > 0 ? iter.duration : 100;
                    size_t row_bytes = (size_t)canvas_w * 4;
                    f.pixels.resize(row_bytes * canvas_h, 0);

                    for (int y = 0; y < h && (iter.y_offset + y) < (int)canvas_h; y++) {
                        int dst_y = (int)canvas_h - 1 - (iter.y_offset + y);
                        int dst_x = iter.x_offset;
                        int copy_w = w;
                        if (dst_x + copy_w > (int)canvas_w)
                            copy_w = (int)canvas_w - dst_x;
                        std::memcpy(f.pixels.data() + dst_y * row_bytes + dst_x * 4,
                                    rgba + y * w * 4, (size_t)copy_w * 4);
                    }
                    WebPFree(rgba);
                    frames.push_back(std::move(f));
                }
            } while (WebPDemuxNextFrame(&iter));
            WebPDemuxReleaseIterator(&iter);
        }

        WebPDemuxDelete(demux);
        return frames;
    }
};

std::unique_ptr<ImageDecoder> create_webp_decoder() {
    return std::make_unique<WebPImageDecoder>();
}
