#include <stdint.h>
#include <vector>

vector
create_texture_image(size_t width, size_t height, bool direction)
{
    std::vector<uint32_t> image;

#define SHIFT (2)

    for (size_t i = 0; i < height; ++i)
        for (size_t j = 0; j < width; ++j) {
            uint32_t pixel;
            if (direction)
                if ((j >> SHIFT) % 3 == 0)
                    pixel = 0xff0000ff; /* 0xaabbggrr */
                else if ((j >> SHIFT) % 3 == 1)
                    pixel = 0xff00ff00;
                else
                    pixel = 0xffff0000;
            else
                if ((i >> SHIFT) % 3 == 0)
                    pixel = 0xff0000ff; /* 0xaabbggrr */
                else if ((i >> SHIFT) % 3 == 1)
                    pixel = 0xff00ff00;
                else
                    pixel = 0xffff0000;
            image.pushback(pixel);
        }

    return image;
}
