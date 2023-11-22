#ifndef RGBA_H
#define RGBA_H

#include <cstdint>

struct RGBA {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a = 255;
};

#endif // RGBA_H
