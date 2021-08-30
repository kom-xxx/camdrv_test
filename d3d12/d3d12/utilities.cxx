#include <stdint.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <windows.h>

#include "utilities.h"

uint32_t
get_pixel_value(size_t x, size_t y, bool direction, uint32_t shift = 0)
{
    if (direction)
        if ((x >> shift) % 3 == 0)
            return 0xff0000ff;
        else if ((x >> shift) % 3 == 1)
            return 0xff00ff00;
        else
            return 0xffff0000;
    else
        if ((y >> shift) % 3 == 0)
            return 0xff0000ff;
        else if ((y >> shift) % 3 == 1)
            return 0xff00ff00;
        else
            return 0xffff0000;
}

std::vector<uint32_t>
create_texture_image(size_t width, size_t height, bool direction)
{
    std::vector<uint32_t> image;
    image.resize(width * height);

    for (size_t i = 0; i < height; ++i)
        for (size_t j = 0; j < width; ++j)
            if (i < 64 && j < 64)
                image[width * i + j] = get_pixel_value(j, i, direction);
            else
                image[width * i + j] = 0xff010101;

    return image;
}

void *
alloc_texture_image(size_t width, size_t height, bool direction)
{
    size_t size = width * height * sizeof (uint32_t);
    uint32_t *mem =
        (uint32_t *)VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE);
    if (!mem) {
        uint32_t error = GetLastError();
        std::stringstream ss;
        ss << __func__ << ":0x" << std::hex << error;
        throw std::runtime_error(ss.str());
    }

    for (size_t i = 0; i < height; ++i)
        for (size_t j = 0; j < width; ++j)
            if (i < 2 || 253 <= i || j < 2 || 253 <= j)
                mem[i * width + j] = 0xff000000;
            else
                mem[i * width + j] = get_pixel_value(j, i, direction);

    return (void *)mem;
}
