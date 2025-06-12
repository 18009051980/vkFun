#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


unsigned char* ImageLoader::loadImage(const char* filename, int* width, int* height, int* channels)
{
    unsigned char* pixels = stbi_load(filename, width, height, channels, STBI_rgb_alpha);
    if (!pixels) {
        return nullptr;
    }
    return pixels;
}

void ImageLoader::freeImage(unsigned char* data)
{
    stbi_image_free(data);
}