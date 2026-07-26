#include "epoch/render/gl/image.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <mutex>
#elif defined(__linux__)
#include <png.h>
#endif

namespace epoch::render::gl {
namespace {
std::string token(std::istream& stream) {
    std::string result;
    char c{};
    while (stream.get(c)) {
        if (c == '#') { std::string ignored; std::getline(stream, ignored); continue; }
        if (!std::isspace(static_cast<unsigned char>(c))) { result.push_back(c); break; }
    }
    while (stream.get(c) && !std::isspace(static_cast<unsigned char>(c))) result.push_back(c);
    if (result.empty()) throw std::runtime_error("Malformed Netpbm header");
    return result;
}

void flip_rows(Image& image) {
    if (image.height <= 1 || image.width <= 0 || image.channels <= 0) return;
    const std::size_t row_bytes = static_cast<std::size_t>(image.width) * image.channels;
    std::vector<std::uint8_t> temporary(row_bytes);
    for (int y = 0; y < image.height / 2; ++y) {
        auto* top = image.pixels.data() + static_cast<std::size_t>(y) * row_bytes;
        auto* bottom = image.pixels.data() + static_cast<std::size_t>(image.height - 1 - y) * row_bytes;
        std::copy_n(top, row_bytes, temporary.data());
        std::copy_n(bottom, row_bytes, top);
        std::copy_n(temporary.data(), row_bytes, bottom);
    }
}

#if defined(_WIN32)
using Microsoft::WRL::ComPtr;
IWICImagingFactory* wic_factory() {
    static std::once_flag once;
    static ComPtr<IWICImagingFactory> factory;
    static HRESULT initialization_result = E_FAIL;
    std::call_once(once, [] {
        const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(com) && com != RPC_E_CHANGED_MODE) { initialization_result = com; return; }
        initialization_result = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(factory.GetAddressOf()));
    });
    if (FAILED(initialization_result) || !factory)
        throw std::runtime_error("Windows Imaging Component initialization failed");
    return factory.Get();
}

Image load_native_image(const std::filesystem::path& path, bool flip_y) {
    IWICImagingFactory* factory = wic_factory();
    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = factory->CreateDecoderFromFilename(
        path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("WIC cannot decode texture: " + path.string());
    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("WIC cannot read texture frame: " + path.string());
    UINT width{}, height{};
    hr = frame->GetSize(&width, &height);
    if (FAILED(hr) || width == 0 || height == 0)
        throw std::runtime_error("WIC returned invalid texture dimensions: " + path.string());
    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(hr)) throw std::runtime_error("WIC format converter creation failed");
    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) throw std::runtime_error("WIC cannot convert texture to RGBA8: " + path.string());
    const std::size_t stride = static_cast<std::size_t>(width) * 4u;
    const std::size_t bytes = stride * height;
    if (bytes > static_cast<std::size_t>(std::numeric_limits<UINT>::max()))
        throw std::runtime_error("Texture is too large for WIC copy: " + path.string());
    Image image{static_cast<int>(width), static_cast<int>(height), 4, std::vector<std::uint8_t>(bytes)};
    hr = converter->CopyPixels(nullptr, static_cast<UINT>(stride), static_cast<UINT>(bytes), image.pixels.data());
    if (FAILED(hr)) throw std::runtime_error("WIC texture copy failed: " + path.string());
    if (flip_y) flip_rows(image);
    return image;
}
#elif defined(__linux__)
Image load_native_image(const std::filesystem::path& path, bool flip_y) {
    std::FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) throw std::runtime_error("Cannot open PNG texture: " + path.string());
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) { std::fclose(file); throw std::runtime_error("libpng read structure allocation failed"); }
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); std::fclose(file); throw std::runtime_error("libpng info allocation failed"); }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        std::fclose(file);
        throw std::runtime_error("libpng cannot decode texture: " + path.string());
    }
    png_init_io(png, file);
    png_read_info(png, info);
    const png_uint_32 width = png_get_image_width(png, info);
    const png_uint_32 height = png_get_image_height(png, info);
    const int color_type = png_get_color_type(png, info);
    const int bit_depth = png_get_bit_depth(png, info);
    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
    png_read_update_info(png, info);
    Image image{static_cast<int>(width), static_cast<int>(height), 4,
        std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height * 4u)};
    std::vector<png_bytep> rows(height);
    for (png_uint_32 y = 0; y < height; ++y)
        rows[y] = image.pixels.data() + static_cast<std::size_t>(y) * width * 4u;
    png_read_image(png, rows.data());
    png_read_end(png, nullptr);
    png_destroy_read_struct(&png, &info, nullptr);
    std::fclose(file);
    if (flip_y) flip_rows(image);
    return image;
}
#else
Image load_native_image(const std::filesystem::path& path, bool) {
    throw std::runtime_error("No native image decoder is configured for: " + path.string());
}
#endif
}

Image load_netpbm(const std::filesystem::path& path, bool flip_y) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open texture: " + path.string());
    const std::string magic = token(file);
    const int channels = magic == "P6" ? 3 : magic == "P5" ? 1 : 0;
    if (!channels) throw std::runtime_error("Only binary PPM/PGM is supported: " + path.string());
    const int width = std::stoi(token(file));
    const int height = std::stoi(token(file));
    const int maximum = std::stoi(token(file));
    if (width <= 0 || height <= 0 || maximum != 255)
        throw std::runtime_error("Unsupported Netpbm dimensions/range");
    Image image{width, height, channels,
        std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height * channels)};
    file.read(reinterpret_cast<char*>(image.pixels.data()), static_cast<std::streamsize>(image.pixels.size()));
    if (file.gcount() != static_cast<std::streamsize>(image.pixels.size()))
        throw std::runtime_error("Truncated texture: " + path.string());
    if (flip_y) flip_rows(image);
    return image;
}

Image load_image(const std::filesystem::path& path, bool flip_y) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    if (extension == ".ppm" || extension == ".pgm" || extension == ".pnm")
        return load_netpbm(path, flip_y);
#if defined(__linux__)
    if (extension != ".png")
        throw std::runtime_error("Linux build currently accepts PNG and Netpbm textures: " + path.string());
#endif
    return load_native_image(path, flip_y);
}

} // namespace epoch::render::gl
