#include "asset/ApngDecoder.h"

#include "utils/BlendOps.h"
#include "utils/ImageOps.h"
#include "utils/Log.h"

#include "stb_image.h"

#include <algorithm>
#include <cstring>

static uint32_t read_u32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}

static uint16_t read_u16(const uint8_t* p) {
    return (uint16_t(p[0]) << 8) | p[1];
}

static void write_u32(uint8_t* p, uint32_t v) {
    p[0] = (v >> 24) & 0xFF;
    p[1] = (v >> 16) & 0xFF;
    p[2] = (v >> 8) & 0xFF;
    p[3] = v & 0xFF;
}

static bool tag_eq(const uint8_t* p, const char* tag) {
    return p[0] == tag[0] && p[1] == tag[1] && p[2] == tag[2] && p[3] == tag[3];
}

enum { DISPOSE_NONE = 0, DISPOSE_BACKGROUND = 1, DISPOSE_PREVIOUS = 2 };
enum { BLEND_SOURCE = 0, BLEND_OVER = 1 };

struct ChunkRef {
    uint32_t length;
    const uint8_t* type;
    const uint8_t* body;
};

struct FcTL {
    uint32_t width, height;
    uint32_t x_offset, y_offset;
    uint16_t delay_num, delay_den;
    uint8_t dispose_op, blend_op;
};

static FcTL parse_fctl(const uint8_t* body) {
    FcTL f;
    f.width = read_u32(body + 4);
    f.height = read_u32(body + 8);
    f.x_offset = read_u32(body + 12);
    f.y_offset = read_u32(body + 16);
    f.delay_num = read_u16(body + 20);
    f.delay_den = read_u16(body + 22);
    f.dispose_op = body[24];
    f.blend_op = body[25];
    return f;
}

static int fctl_duration_ms(const FcTL& f) {
    uint16_t den = f.delay_den == 0 ? 100 : f.delay_den;
    return static_cast<int>(f.delay_num * 1000 / den);
}

/// Build a minimal valid PNG from IHDR (with modified dims) + ancillary chunks + IDAT data.
/// The ancillary chunks (PLTE, tRNS, etc.) are needed for paletted PNGs.
static std::vector<uint8_t> build_png(const uint8_t* ihdr_body, uint32_t ihdr_len, uint32_t frame_width,
                                      uint32_t frame_height, const std::vector<ChunkRef>& ancillary_chunks,
                                      const std::vector<std::vector<uint8_t>>& idat_bodies) {
    static const uint8_t png_sig[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    static const uint8_t iend[] = {0, 0, 0, 0, 'I', 'E', 'N', 'D', 0xAE, 0x42, 0x60, 0x82};

    std::vector<uint8_t> ihdr(ihdr_body, ihdr_body + ihdr_len);
    write_u32(ihdr.data() + 0, frame_width);
    write_u32(ihdr.data() + 4, frame_height);

    std::vector<uint8_t> png;
    png.reserve(4096);

    png.insert(png.end(), png_sig, png_sig + 8);

    auto write_chunk = [&](const char* type, const uint8_t* data, uint32_t len) {
        uint8_t buf[4];
        write_u32(buf, len);
        png.insert(png.end(), buf, buf + 4);
        png.insert(png.end(), (const uint8_t*)type, (const uint8_t*)type + 4);
        png.insert(png.end(), data, data + len);
        uint8_t zero_crc[4] = {0, 0, 0, 0};
        png.insert(png.end(), zero_crc, zero_crc + 4);
    };

    write_chunk("IHDR", ihdr.data(), (uint32_t)ihdr.size());

    // Write ancillary chunks (PLTE, tRNS, etc.) — required for paletted PNGs
    for (auto& c : ancillary_chunks) {
        write_chunk((const char*)c.type, c.body, c.length);
    }

    for (auto& body : idat_bodies) {
        write_chunk("IDAT", body.data(), (uint32_t)body.size());
    }
    png.insert(png.end(), iend, iend + sizeof(iend));

    return png;
}

struct FrameInfo {
    FcTL fctl;
    std::vector<std::vector<uint8_t>> data;
};

static std::vector<ChunkRef> parse_chunks(const uint8_t* data, size_t size) {
    std::vector<ChunkRef> chunks;
    size_t pos = 8;
    while (pos + 8 <= size) {
        uint32_t len = read_u32(data + pos);
        if (pos + 12 + len > size)
            break;

        ChunkRef c;
        c.length = len;
        c.type = data + pos + 4;
        c.body = data + pos + 8;
        chunks.push_back(c);

        pos += 12 + len;
    }
    return chunks;
}

static std::vector<FrameInfo> collect_frames(const std::vector<ChunkRef>& chunks, std::vector<ChunkRef>& ancillary) {
    std::vector<FrameInfo> frame_infos;
    int active_frame = -1;

    // Collect ancillary chunks that appear before IDAT (PLTE, tRNS, etc.)
    // These are needed for paletted PNGs and must be included in synthetic PNGs.
    std::vector<std::vector<uint8_t>> all_idats;

    for (size_t i = 1; i < chunks.size(); i++) {
        auto& c = chunks[i];

        if (tag_eq(c.type, "fcTL") && c.length >= 26) {
            frame_infos.emplace_back();
            active_frame = (int)frame_infos.size() - 1;
            frame_infos.back().fctl = parse_fctl(c.body);
        }
        else if (tag_eq(c.type, "IDAT")) {
            all_idats.emplace_back(c.body, c.body + c.length);
        }
        else if (tag_eq(c.type, "fdAT") && c.length > 4) {
            if (active_frame >= 0) {
                frame_infos[active_frame].data.emplace_back(c.body + 4, c.body + c.length);
            }
        }
        else if (!tag_eq(c.type, "acTL") && !tag_eq(c.type, "IEND") && !tag_eq(c.type, "tEXt") &&
                 !tag_eq(c.type, "iTXt") && !tag_eq(c.type, "zTXt") && all_idats.empty()) {
            // Ancillary chunk before any IDAT — keep for synthetic PNGs
            ancillary.push_back(c);
        }
    }

    if (!frame_infos.empty() && frame_infos[0].data.empty()) {
        frame_infos[0].data = all_idats;
    }

    Log::log_print(VERBOSE, "APNG: parsed %zu frame_infos, %zu IDAT chunks, %zu ancillary chunks", frame_infos.size(),
                   all_idats.size(), ancillary.size());

    return frame_infos;
}

static std::optional<DecodedFrame> decode_frame(const FrameInfo& fi, size_t fi_idx, const uint8_t* ihdr_body,
                                                uint32_t ihdr_len, const std::vector<ChunkRef>& ancillary,
                                                std::vector<uint8_t>& canvas, uint32_t canvas_w, uint32_t canvas_h,
                                                bool flip_y) {
    auto& fctl = fi.fctl;
    int duration_ms = fctl_duration_ms(fctl);

    auto png_data = build_png(ihdr_body, ihdr_len, fctl.width, fctl.height, ancillary, fi.data);

    // Decode sub-frame WITHOUT flip — compositing is done in top-down canvas
    // coordinates. The final canvas is flipped once after compositing.
    int w, h, ch;
    uint8_t* pixels = stbi_load_from_memory(png_data.data(), (int)png_data.size(), &w, &h, &ch, 4);

    if (!pixels) {
        const char* reason = stbi_failure_reason();
        Log::log_print(WARNING, "APNG: stbi failed to decode frame %zu (png_size=%zu, frame=%ux%u): %s", fi_idx,
                       png_data.size(), fctl.width, fctl.height, reason ? reason : "unknown");
        return std::nullopt;
    }

    std::vector<uint8_t> prev_canvas;
    if (fctl.dispose_op == DISPOSE_PREVIOUS) {
        prev_canvas = canvas;
    }

    // Composite onto canvas (top-down coordinates, no flip)
    uint32_t fx = fctl.x_offset;
    uint32_t fy = fctl.y_offset;
    uint32_t fw = (uint32_t)w;
    uint32_t fh = (uint32_t)h;

    // NB: clang-tidy thinks this is an infinite loop because fx/fy don't change. It is wrong.
    // NOLINTBEGIN(bugprone-infinite-loop)
    for (uint32_t row = 0; row < fh && (fy + row) < canvas_h; row++) {
        for (uint32_t col = 0; col < fw && (fx + col) < canvas_w; col++) {
            size_t src_idx = (row * fw + col) * 4;
            size_t dst_idx = ((fy + row) * canvas_w + (fx + col)) * 4;

            if (fctl.blend_op == BLEND_OVER) {
                BlendOps::blend_over(&canvas[dst_idx], &pixels[src_idx]);
            }
            else {
                std::memcpy(&canvas[dst_idx], &pixels[src_idx], 4);
            }
        }
    }
    // NOLINTEND(bugprone-infinite-loop)

    stbi_image_free(pixels);

    // Snapshot canvas, then flip vertically for GL if requested
    DecodedFrame frame;
    frame.width = (int)canvas_w;
    frame.height = (int)canvas_h;
    frame.duration_ms = duration_ms > 0 ? duration_ms : 100;

    frame.pixels = canvas;
    if (flip_y)
        flip_vertical_rgba(frame.pixels.data(), (int)canvas_w, (int)canvas_h);

    // Dispose (operates on the top-down canvas, not the flipped frame)
    if (fctl.dispose_op == DISPOSE_BACKGROUND) {
        for (uint32_t row = 0; row < fh && (fy + row) < canvas_h; row++) {
            for (uint32_t col = 0; col < fw && (fx + col) < canvas_w; col++) {
                size_t dst_idx = ((fy + row) * canvas_w + (fx + col)) * 4;
                std::memset(&canvas[dst_idx], 0, 4);
            }
        }
    }
    else if (fctl.dispose_op == DISPOSE_PREVIOUS) {
        canvas = prev_canvas;
    }

    return frame;
}

namespace ApngDecoder {

std::optional<std::vector<DecodedFrame>> decode(const uint8_t* data, size_t size, bool flip_y) {
    static const uint8_t png_sig[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (size < 8 || std::memcmp(data, png_sig, 8) != 0) {
        return std::nullopt;
    }

    auto chunks = parse_chunks(data, size);
    if (chunks.empty())
        return std::nullopt;

    if (!tag_eq(chunks[0].type, "IHDR") || chunks[0].length < 13) {
        return std::nullopt;
    }
    const uint8_t* ihdr_body = chunks[0].body;
    uint32_t canvas_w = read_u32(ihdr_body);
    uint32_t canvas_h = read_u32(ihdr_body + 4);

    // Find acTL
    bool is_apng = false;
    uint32_t num_frames = 1;
    for (auto& c : chunks) {
        if (tag_eq(c.type, "acTL") && c.length >= 8) {
            num_frames = read_u32(c.body);
            is_apng = true;
            break;
        }
    }

    Log::log_print(VERBOSE, "APNG: canvas=%ux%u is_apng=%d num_frames=%u chunks=%zu", canvas_w, canvas_h, is_apng,
                   num_frames, chunks.size());

    // Plain PNG
    if (!is_apng || num_frames <= 1) {
        int w, h, ch;
        uint8_t* pixels = stbi_load_from_memory(data, (int)size, &w, &h, &ch, 4);
        if (!pixels)
            return std::nullopt;
        if (flip_y)
            flip_vertical_rgba(pixels, w, h);

        DecodedFrame f;
        f.width = w;
        f.height = h;
        f.duration_ms = 0;
        f.pixels.assign(pixels, pixels + w * h * 4);
        stbi_image_free(pixels);

        return std::vector<DecodedFrame>{std::move(f)};
    }

    // ---- APNG multi-frame decoding ----
    std::vector<ChunkRef> ancillary;
    auto frame_infos = collect_frames(chunks, ancillary);

    std::vector<uint8_t> canvas(canvas_w * canvas_h * 4, 0);
    std::vector<DecodedFrame> frames;
    frames.reserve(frame_infos.size());

    for (size_t fi_idx = 0; fi_idx < frame_infos.size(); fi_idx++) {
        auto& fi = frame_infos[fi_idx];
        if (fi.data.empty()) {
            Log::log_print(WARNING, "APNG: frame %zu has no data, skipping", fi_idx);
            continue;
        }

        auto result =
            decode_frame(fi, fi_idx, ihdr_body, chunks[0].length, ancillary, canvas, canvas_w, canvas_h, flip_y);
        if (result) {
            frames.push_back(std::move(*result));
        }
    }

    Log::log_print(VERBOSE, "APNG: decoded %zu frames total", frames.size());

    if (frames.empty())
        return std::nullopt;
    return frames;
}

} // namespace ApngDecoder

// ---------------------------------------------------------------------------
// ImageDecoder interface for PNG/APNG
// ---------------------------------------------------------------------------

#include "asset/ImageDecoder.h"

class ApngImageDecoder : public ImageDecoder {
  public:
    std::vector<std::string> extensions() const override {
        return {"apng", "png"};
    }

    std::vector<DecodedFrame> decode(const uint8_t* data, size_t size) const override {
        if (!data || size == 0)
            return {};
        auto apng_frames = ApngDecoder::decode(data, size, true);
        if (apng_frames && !apng_frames->empty())
            return std::move(*apng_frames);

        // Fallback: plain PNG via stb_image.
        // Only attempt if data has a valid PNG signature to avoid stbi crashes
        // on completely non-PNG data (observed on macOS).
        static const uint8_t png_sig[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
        if (size < 8 || memcmp(data, png_sig, 8) != 0)
            return {};

        int width, height, channels;
        uint8_t* pixels = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 4);

        std::vector<DecodedFrame> frames;
        if (!pixels)
            return frames;

        flip_vertical_rgba(pixels, width, height);

        DecodedFrame f;
        f.width = width;
        f.height = height;
        f.duration_ms = 0;
        f.pixels.assign(pixels, pixels + width * height * 4);
        stbi_image_free(pixels);
        frames.push_back(std::move(f));
        return frames;
    }
};

std::unique_ptr<ImageDecoder> create_apng_decoder() {
    return std::make_unique<ApngImageDecoder>();
}
