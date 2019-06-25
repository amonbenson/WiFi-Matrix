#include <stdio.h>

#include "MAFDecoder.h"
#include <string.h>

//#define VFILE_DEBUG

/*
 * MAF (Matrix Animation File) Format:
 * 
 * bytesize:    4 + paletteColorCount * 3 + width * height * frameCount
 * 
 * OFFSET   MEANING
 * 0x00     width of the animation
 * 0x01     height of the animation
 * 0x02     number of frames / animation length
 * 
 * 0x03     number of colors in the palette minus on (0: 1 color, 1: two colors, ..., 255: 256 colors)
 * 0x04...  color0 red, color0 green, color0 blue, color1 red, color1 green, color1 blue, ...
 * 
 * 0xn0...  frame 0 color id at (0, 0), frame 0 color id at (1, 0), frame 0 color id at (2, 0), ...
 * 0xm0...  frame 1 color id at (0, 0), ...
 * 
 * ...      ...
 */

unsigned char file[] = {
    2,      // Animation width
    2,      // Animation height
    3,      // Number of frames
    
    1,      // Number of colors in the palette (1 => 2 different colors)
    255,    // Color0: Magenta
    0,
    255,
    0,      // Color1: Green
    255,
    0,

    1,      // Frame0
    0,      //   GM
    0,      //   MM
    0,
    1,      // Frame1
    0,      //   GM
    0,      //   MG
    1,
    0,      // Frame2
    1,      //   MG
    1,      //   GM
    0
};
int fileIndex = 0;

bool mafFileSeek(unsigned long position) {
    #ifdef VFILE_DEBUG
        printf("FILE: Seeking to position %lu\n", position);
    #endif

    fileIndex = position;
    return true;
}

int mafFileRead(void) {
    #ifdef VFILE_DEBUG
        printf("FILE: Reading byte from position %d\n", fileIndex);
    #endif

    return file[fileIndex++];
}

int mafFileReadBlock(void *buffer, int numberOfBytes) {
    #ifdef VFILE_DEBUG
        printf("FILE: Reading %d bytes from position %d\n", numberOfBytes, fileIndex);
    #endif

    memcpy(buffer, &file[fileIndex], numberOfBytes);
    fileIndex += numberOfBytes;

    return 0;
}

void mafDrawPixel(uint8_t x, uint8_t y, uint8_t red, uint8_t green, uint8_t blue) {
    printf("Pixel at (%d, %d) is rgb(%d, %d, %d)\n", x, y, red, green, blue);
}

void mafUpdateScreen(void) {
    printf("RENDER!\n");
}


MAFDecoder decoder = MAFDecoder(2, 2);


int main() {
    printf("MAF Decoder Library Test\n");

    decoder.setFileSeekCallback(mafFileSeek);
    decoder.setFileReadCallback(mafFileRead);
    decoder.setFileReadBlockCallback(mafFileReadBlock);

    decoder.setDrawPixelCallback(mafDrawPixel);
    decoder.setUpdateScreenCallback(mafUpdateScreen);

    decoder.initDecoder();

    decoder.decodeFrame();

    return 0;
}