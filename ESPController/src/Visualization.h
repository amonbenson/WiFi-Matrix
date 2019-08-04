#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "Settings.h"

#include <FastLED.h>
#include <arduinoFFT.h>

//#define VISUALIZATION_EMULATE_INPUT
#define VISUALIZATION_PALETTE_SIZE  5
#define NUM_VISUALIZATIONS          3

// Visualization update functions
void v_bars(CRGB*, CRGB*, CRGB*, double*, double, double);
void v_swirl(CRGB*, CRGB*, CRGB*, double*, double, double);
void v_heatmap(CRGB*, CRGB*, CRGB*, double*, double, double);

class Visualization {
    private:
        int currentVis;                 // Current visualization id
        void (*updateFuncs[NUM_VISUALIZATIONS])(CRGB*, CRGB*, CRGB*, double[], double, double) = {
            &v_bars, &v_swirl, &v_heatmap
        }; // Array of update functions

        arduinoFFT FFT;
        double fftReal[FFT_SAMPLES];    // Real part of the fft
        double fftImag[FFT_SAMPLES];    // Imaginary part of the fft
        double fftVal[MATRIX_WIDTH];    // Actual fft values
        double fftPeak;                 // Position of the peak (0 to 1)
        double fftPeakVal;              // Value of the peak (0 to MATRIX_HEIGHT)
        CRGB palette[VISUALIZATION_PALETTE_SIZE];       // Palette colors: Background, colA, colB, colC, colD
        CRGB colorBuf[MATRIX_WIDTH * MATRIX_HEIGHT];    // Color buffer used by some visualizations

    public:
        Visualization();

        double fftGain;

        void update(CRGB *leds);

        void nextVis();
        void prevVis();

        bool setPaletteColor(int index, CRGB color);
        CRGB getPaletteColor(int index);
        CRGB *getPalette();
};


#endif