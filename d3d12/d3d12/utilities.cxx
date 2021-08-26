#include <stdint.h>
#include <vector>

#include "utilities.h"

uint32_t
get_pixel_value(size_t x, size_t y, bool direction, uint32_t shift = 0)
{
    if (direction)
        if ((x >> shift) % 3 == 0)
            return 0xff0000ff;
        else if ((x >> shift) %3 == 1)
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

    for (size_t i = 0; i < height; ++i)
        for (size_t j = 0; j < width; ++j)
            image.push_back(get_pixel_value(j, i, direction));

    return image;
}
