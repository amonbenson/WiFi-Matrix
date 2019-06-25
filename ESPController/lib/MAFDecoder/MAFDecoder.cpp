#include "MAFDecoder.h"
#include <stdio.h>

MAFDecoder::MAFDecoder(uint8_t matrix_width, uint8_t matrix_height) {
    m_matrix_width = matrix_width;
    m_matrix_height = matrix_height;
}

void MAFDecoder::setFileSeekCallback(file_seek_callback c) {
    fileSeekCallback = c;
}

void MAFDecoder::setFileReadCallback(file_read_callback c) {
    fileReadCallback = c;
}

void MAFDecoder::setFileReadBlockCallback(file_read_block_callback c) {
    fileReadBlockCallback = c;
}

void MAFDecoder::setDrawPixelCallback(draw_pixel_callback c) {
    drawPixelCallback = c;
}

void MAFDecoder::setUpdateScreenCallback(update_screen_callback c) {
    updateScreenCallback = c;
}

void MAFDecoder::initDecoder(void) {
    // Read the animation width and height and check if they are correct.
    uint8_t animationWidth = fileReadCallback();
    uint8_t animationHeight= fileReadCallback();
    if (animationWidth != m_matrix_width || animationHeight != m_matrix_height) {
        #ifdef MAF_DEBUG
            printf("MAF: Animation size is different from matrix size!\n");
        #endif
        return;
    }

    // Read the animation frame count
    m_frame_count = fileReadCallback();
    #ifdef MAF_DEBUG
        printf("MAF: Frame count: %d\n", m_frame_count);
    #endif

    // Read the palette size
    int paletteSize = (int) fileReadCallback() + 1;
    #ifdef MAF_DEBUG
        printf("MAF: Palette size: %d\n", paletteSize);
    #endif

    // Read the palette
    fileReadBlockCallback(&m_palette, paletteSize * 3);
    #ifdef MAF_DEBUG
        printf("Palette: ");
        for (int i = 0; i < paletteSize * 3; i++) printf("%d ", (uint8_t) m_palette[i]);
        printf("\n");
    #endif

    // Set the frame offset
    m_frame_offset = 4 + paletteSize * 3;
    m_currrent_frame = 0;
}

void MAFDecoder::decodeFrame(void) {
    // Seek to the current position
    fileSeekCallback(m_frame_offset + (int) m_matrix_width * (int) m_matrix_height * (int) m_currrent_frame);

    // Read the frame data one-by-one and output it to the draw pixel callback
    for (uint8_t y = 0; y < m_matrix_height; y++) {
        for (uint8_t x = 0; x < m_matrix_width; x++) {
            // Get the palette color and convert it to rgb
            const uint8_t color = fileReadCallback();
            const uint8_t red = m_palette[(int) color * 3];
            const uint8_t green = m_palette[(int) color * 3 + 1];
            const uint8_t blue = m_palette[(int) color * 3 + 2];

            drawPixelCallback(x, y, red, green, blue);
        }
    }

    m_currrent_frame++;
    if (m_currrent_frame >= m_frame_count) m_currrent_frame = 0;
    updateScreenCallback();
}