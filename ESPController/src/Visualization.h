#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "Settings.h"

#include <FastLED.h>
#include <arduinoFFT.h>

#define VISUALIZATION_NUM_COLORS        4
#define NUM_VISUALIZATIONS              1

#define FFT_SAMPLES                     32
#define FFT_DC_OFFSET                   679
#define FFT_NOISE_REDUCTION             3
#define FFT_ATTACK                      0.2
#define FFT_RELEASE                     0.8

class Visualization {
    private:
        int currentVis;

        arduinoFFT FFT;
        double fftReal[FFT_SAMPLES];  // Real part of the fft
        double fftImag[FFT_SAMPLES];  // Imaginary part of the fft
        double fftVal[MATRIX_WIDTH];  // Actual fft values

        void v_bars(CRGB leds[]);

        CRGB v_swirl_colorBuf[MATRIX_WIDTH * MATRIX_HEIGHT / 2];
        void v_swirl(CRGB leds[]);

    public:
        Visualization();

        CRGB colors[VISUALIZATION_NUM_COLORS];
        double fftGain;

        void init();
        void update(CRGB *leds);

        void nextVis();
        void prevVis();
};


#endif