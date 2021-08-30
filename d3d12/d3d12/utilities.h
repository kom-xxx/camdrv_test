#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdint.h>
#include <vector>

std::vector<uint32_t> create_texture_image(size_t, size_t, bool);
void *alloc_texture_image(size_t, size_t, bool);

#endif  /* !UTILITIES_H */
