#pragma once

class ImageLoader
{
public:
    ImageLoader() = default;
    ~ImageLoader() = default;

    static unsigned char* loadImage(const char* filename, int* width, int* height, int* channels);
    static void freeImage(unsigned char* data);
};