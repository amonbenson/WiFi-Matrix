#ifndef MAF_DECODER_H
#define MAF_DECODER_H

#include <stdint.h>

//#define MAF_DEBUG

typedef bool (*file_seek_callback)(unsigned long position);
typedef int (*file_read_callback)(void);
typedef int (*file_read_block_callback)(void *buffer, int numberOfBytes);

typedef void (*draw_pixel_callback)(uint8_t x, uint8_t y, uint8_t red, uint8_t green, uint8_t blue);
typedef void (*update_screen_callback)(void);

class MAFDecoder {
    private:
        int8_t m_matrix_width;
        int8_t m_matrix_height;

        int8_t m_frame_count, m_currrent_frame;

        int8_t m_palette[256 * 3];
        int m_frame_offset;

        file_seek_callback fileSeekCallback;
        file_read_callback fileReadCallback;
        file_read_block_callback fileReadBlockCallback;

        draw_pixel_callback drawPixelCallback;
        update_screen_callback updateScreenCallback;

    public:
        MAFDecoder(uint8_t matrix_width, uint8_t matrix_height);

        void setFileSeekCallback(file_seek_callback c);
        void setFileReadCallback(file_read_callback c);
        void setFileReadBlockCallback(file_read_block_callback c);

        void setDrawPixelCallback(draw_pixel_callback c);
        void setUpdateScreenCallback(update_screen_callback c);

        void initDecoder(void);
        void decodeFrame(void);
};

#endif