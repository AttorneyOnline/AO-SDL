#include "asset/ImageDecoder.h"

#include <webp/decode.h>
#include <webp/demux.h>

#include <cstring>

class WebPImageDecoder : public ImageDecoder {
  public:
    std::vector<std::string> extensions() const override {
        return {"webp"};
    }

    std::vector<ImageFrame> decode(const uint8_t* data, size_t size) const override {
        if (!data || size == 0)
            return {};

        std::vector<ImageFrame> frames;

        WebPData webp_data = {data, size};
        WebPDemuxer* demux = WebPDemux(&webp_data);
        if (!demux)
            return frames;

        uint32_t canvas_w = WebPDemuxGetI(demux, WEBP_FF_CANVAS_WIDTH);
        uint32_t canvas_h = WebPDemuxGetI(demux, WEBP_FF_CANVAS_HEIGHT);
        size_t row_bytes = (size_t)canvas_w * 4;
        std::vector<uint8_t> canvas(row_bytes * canvas_h, 0);
        WebPIterator iter;

        if (WebPDemuxGetFrame(demux, 1, &iter)) {
            do {
                int w, h;
                uint8_t* rgba = WebPDecodeRGBA(iter.fragment.bytes, iter.fragment.size, &w, &h);
                if (!rgba || w <= 0 || h <= 0)
                    continue;

                // Save canvas for DISPOSE_PREVIOUS
                std::vector<uint8_t> prev_canvas;
                if (iter.dispose_method == WEBP_MUX_DISPOSE_BACKGROUND)
                    prev_canvas = canvas; // will clear after snapshot
                else if (iter.dispose_method == WEBP_MUX_DISPOSE_NONE)
                    ; // keep canvas as-is for next frame

                // Composite frame onto canvas
                for (int y = 0; y < h && (iter.y_offset + y) < (int)canvas_h; y++) {
                    for (int x = 0; x < w && (iter.x_offset + x) < (int)canvas_w; x++) {
                        size_t src_idx = ((size_t)y * w + x) * 4;
                        int dst_y_flipped = (int)canvas_h - 1 - (iter.y_offset + y);
                        size_t dst_idx = ((size_t)dst_y_flipped * canvas_w + iter.x_offset + x) * 4;

                        if (iter.blend_method == WEBP_MUX_NO_BLEND) {
                            std::memcpy(&canvas[dst_idx], &rgba[src_idx], 4);
                        }
                        else {
                            // Alpha-blend (WEBP_MUX_BLEND)
                            uint8_t sa = rgba[src_idx + 3];
                            if (sa == 255) {
                                std::memcpy(&canvas[dst_idx], &rgba[src_idx], 4);
                            }
                            else if (sa > 0) {
                                uint8_t da = canvas[dst_idx + 3];
                                uint16_t oa = sa + da * (255 - sa) / 255;
                                if (oa > 0) {
                                    canvas[dst_idx + 0] = (uint8_t)((rgba[src_idx + 0] * sa +
                                                                     canvas[dst_idx + 0] * da * (255 - sa) / 255) /
                                                                    oa);
                                    canvas[dst_idx + 1] = (uint8_t)((rgba[src_idx + 1] * sa +
                                                                     canvas[dst_idx + 1] * da * (255 - sa) / 255) /
                                                                    oa);
                                    canvas[dst_idx + 2] = (uint8_t)((rgba[src_idx + 2] * sa +
                                                                     canvas[dst_idx + 2] * da * (255 - sa) / 255) /
                                                                    oa);
                                    canvas[dst_idx + 3] = (uint8_t)oa;
                                }
                            }
                        }
                    }
                }
                WebPFree(rgba);

                // Snapshot canvas as frame
                ImageFrame f;
                f.width = (int)canvas_w;
                f.height = (int)canvas_h;
                f.duration_ms = iter.duration > 0 ? iter.duration : 100;
                f.pixels = canvas;
                frames.push_back(std::move(f));

                // Dispose
                if (iter.dispose_method == WEBP_MUX_DISPOSE_BACKGROUND) {
                    for (int y = 0; y < h && (iter.y_offset + y) < (int)canvas_h; y++) {
                        int dst_y_flipped = (int)canvas_h - 1 - (iter.y_offset + y);
                        size_t dst_start = ((size_t)dst_y_flipped * canvas_w + iter.x_offset) * 4;
                        int clear_w = w;
                        if (iter.x_offset + clear_w > (int)canvas_w)
                            clear_w = (int)canvas_w - iter.x_offset;
                        std::memset(&canvas[dst_start], 0, (size_t)clear_w * 4);
                    }
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
