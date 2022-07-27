// Bresenham.cpp : Example app that draws circles on a tga

#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <cstdio>


static void writeTGAHeader(unsigned char * header, int width, int height, int bitsPerPixel)
{
    memset(header, 0, 18);

    header++; // id length - you can put a identification string (up to 255 chars) after the header, before the pixels
    header++; // color map. We don't need no stinking color map.
    *header++ = 2; // image type. 2 == uncompressed

    header += 5; // more color map stuff (that we don't need)
    header += 4; // X, Y origin always 0 (little endian btw)

    // little endian width, height
    *header++ = (width & 0x00FF);  *header++ = (width & 0xFF00) >> 8;  // width
    *header++ = (height & 0x00FF); *header++ = (height & 0xFF00) >> 8; // height

    *header++ = bitsPerPixel;
    *header = 0x20; // flags - 0x20 is the upside down bit
}

static void printBGRAToTGA(std::string filename, unsigned char const * image, int width, int height)
{
    std::FILE * fptr = std::fopen(filename.c_str(), "wb");
    if (!fptr) {
        const char * error = std::strerror(errno);
        return; // open can fail, for example on Windows 10 due to permissions, etc
    }

    unsigned char header[18];
    writeTGAHeader(header, width, height, 32);

    std::fwrite(header, 1, sizeof(header), fptr);
    std::fwrite(image, 1, 4 * width * height, fptr); // assume BGRA format (in order)
    std::fclose(fptr);
}

struct BGRA
{
    unsigned char b=0, g=0, r=0, a=0;
};

struct ImageBGRA
{
    BGRA * pixels = 0;
    int width = 0;
    int height = 0;

    ImageBGRA() = default;
    ImageBGRA(int width, int height) : width(width), height(height)
    {
        pixels = (BGRA *)malloc(width * height * 4);
    }
    ~ImageBGRA()
    {
        free(pixels);
    }

    bool inbounds(int x, int y) const
    {
        return 0 <= x && x < width
            && 0 <= y && y < height;
    }

    int clampX(int x) const { return x < 0 ? 0 : x >= width ? width - 1 : x; }
    int clampY(int y) const { return y < 0 ? 0 : y >= height? height - 1 : y; }

    void clampLine(int & x, int & length) const
    {
        if (length < 0) {
            length = -length;
            x = x - length + 1;
        }
        if (x < 0) {
            length += x;
            x = 0;
        }
        if (x + length > width) {
            length = width - x;
            if (length < 0)
                length = 0;
        }
    }

    BGRA * raw_address(int x, int y) { return pixels + y * width + x; }
    BGRA const * raw_address(int x, int y) const { return pixels + y * width + x; }

    void drawLine(int x, int y, int length, BGRA colour)
    {
        if (y < 0 || y >= height)
            return; // out of bounds
        clampLine(x, length);
        if (x > width)
            return; // out of bounds

        if (length)
        {
            BGRA * dst = raw_address(x, y);
            while (length--)
                *dst++ = colour;
        }
    }

    void set(int x, int y, BGRA c)
    {
        if (!inbounds(x, y))
            return;
        *raw_address(x, y) = c;
    }
};

void drawCircle(ImageBGRA & img, int cx, int cy, int r, BGRA colour)
{
    using draw_ptr_fn = void(int cx, int cy, int x, int y, BGRA colour, ImageBGRA & img);
    using draw_ptr_t = draw_ptr_fn *;

    draw_ptr_t drawStrips = [](int cx, int cy, int x, int y, BGRA colour, ImageBGRA & img)
    {
        // in memory order
        img.drawLine(cx - x, cy - y, 2*x+1, colour);
        img.drawLine(cx - y, cy - x, 2*y+1, colour);
        img.drawLine(cx - y, cy + x, 2*y+1, colour);
        img.drawLine(cx - x, cy + y, 2*x+1, colour);
    };
    draw_ptr_t drawOutlinePoint = [](int cx, int cy, int x, int y, BGRA colour, ImageBGRA & img)
    {
        BGRA c = { 255,0,0,255 };
        // in memory order
        img.set(cx - x, cy - y, c);
        img.set(cx + x, cy - y, c);
        img.set(cx - y, cy - x, c);
        img.set(cx + y, cy - x, c);
        img.set(cx - y, cy + x, c);
        img.set(cx + y, cy + x, c);
        img.set(cx - x, cy + y, c);
        img.set(cx + x, cy + y, c);
    };

    auto draw = drawStrips;

    int twice = 2;
    while (twice--) // fill, then outline
    {
        int d = 3 - 2 * r;
        int x = 0;
        int y = r;

        draw(cx, cy, x, y, colour, img);
        while (x < y)
        {
            if (d <= 0)
            {
                d = d + (4 * x) + 6;
                x++;
                draw(cx, cy, x, y, colour, img);
            }
            else
            {
                d = d + (4 * (x - y)) + 10;
                x++;
                y--;
                draw(cx, cy, x, y, colour, img);
            }
        }
        draw = drawOutlinePoint;
    }

    // center
    *img.raw_address(cx, cy) = { 255,0,255,255 };
}

static void printBGRAToTGA(std::string filename, ImageBGRA const & img)
{
    printBGRAToTGA(filename, (unsigned char *)img.pixels, img.width, img.height);
}


int main(int argc, char * argv[])
{
/*
    std::cout << "Hello World!\n";

    for (int i = 0; i < argc; i++)
    {
        std::cout << argv[i] << '\n';
    }
    if (argc != 2) // 1st is program name, second is file to save image to
    {
        std::cout << "Usage:\n";
        std::cout << argv[0] << " <filename>\n";
        std::cout << "where <filename> is where the image will be saved";
        return -1;
    }
*/

    std::string filename = argc == 2 ? argv[1] : "C:/Code/bresenham2.tga";

    ImageBGRA img(640, 480);

    BGRA red = { 0, 0, 255, 255 };
    BGRA green = { 0, 255, 0, 255 };
    drawCircle(img, 20, 50, 0, green);
    drawCircle(img, 30, 50, 1, red);
    drawCircle(img, 40, 50, 2, green);
    drawCircle(img, 50, 50, 3, red);
    drawCircle(img, 60, 50, 4, green);
    drawCircle(img, 70, 50, 5, red);

    printBGRAToTGA(filename, img);
}
