#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace epoch::render::gl {

struct Image {
    int width{};
    int height{};
    int channels{};
    std::vector<std::uint8_t> pixels;
};

// Native Windows loader: WIC for PNG/JPEG/BMP/TIFF/GIF plus Netpbm fallback.
// Images are returned as tightly-packed RGBA8 or R8 and may be flipped for GL UVs.
[[nodiscard]] Image load_image(const std::filesystem::path& path, bool flip_y = true);
[[nodiscard]] Image load_netpbm(const std::filesystem::path& path, bool flip_y = true);

} // namespace epoch::render::gl
