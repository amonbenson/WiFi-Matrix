#include "Visualization.h"

int toLedPos(int x, int y) {
    return x + y * MATRIX_WIDTH;
}

Visualization::Visualization() {
    currentVis = 0;

    FFT = arduinoFFT();
    fftGain = 0.6;
}

void Visualization::update(CRGB *leds) {
    // Make a capture on the microphone pin
    for (int i = 0; i < FFT_SAMPLES; i++) {
        #ifdef FFT_EMULATE_INPUT
            fftReal[i] = random(6) - 3;
        #else
            fftReal[i] = analogRead(PIN_MICROPHONE) - FFT_DC_OFFSET;
        #endif
        fftImag[i] = 0;
    }

    // Calculate the FFT
    FFT.Windowing(fftReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(fftReal, fftImag, FFT_SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(fftReal, fftImag, FFT_SAMPLES);

    // Calculate the smoothed fft value
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        // Scale the value to MATRIX_WIDTH
        double val = 0;
        double l = FFT_SAMPLES / (double) MATRIX_WIDTH / 2;
        for (int i = (int) (x * l); i < (x + 1) * l; i++) {
            val += fftReal[i];
        }
        val *= fftGain / l;

        // Cut off noise and apply the attack and decay
        val -= FFT_NOISE_REDUCTION;
        if (val < 0) val = 0;
        if (val >= MATRIX_HEIGHT) val = MATRIX_HEIGHT - 1;

        if (val > fftVal[x]) fftVal[x] = fftVal[x] * FFT_ATTACK + val * (1 - FFT_ATTACK);
        else fftVal[x] = fftVal[x] * FFT_RELEASE + val * (1 - FFT_RELEASE);
    }

    // Use the selected visualization
    if (currentVis == 0) v_bars(leds);
    if (currentVis == 1) v_swirl(leds);
}

void Visualization::v_bars(CRGB leds[]) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            if (y > MATRIX_HEIGHT - fftVal[x] - 1.5) leds[x + y * MATRIX_WIDTH] = CRGB(x * 255 / (MATRIX_WIDTH - 1), y * 255 / (MATRIX_HEIGHT - 1), 255);
            else leds[toLedPos(x, y)] = CRGB::Black;
        }
    }
}

void Visualization::v_swirl(CRGB leds[]) {
    // Get the new color
    int a = fftVal[0] * 255 / MATRIX_HEIGHT;
    int b = max(0, (int) (a - fftVal[MATRIX_WIDTH / 2] * 255 / MATRIX_HEIGHT));
    int c = max(0, (int) (a - fftVal[MATRIX_WIDTH - 1] * 255 / MATRIX_HEIGHT));
    
    CRGB col = CRGB(a, b, c);

    // Shift it into the buffer array
    for (int i = MATRIX_WIDTH * MATRIX_HEIGHT / 2 - 1; i > 0; i--)
        v_swirl_colorBuf[i] = v_swirl_colorBuf[i - 1];
    v_swirl_colorBuf[0] = col;

    int wMax = MATRIX_WIDTH - 1;
    int hMax = MATRIX_HEIGHT - 2;
    int direction = 0;

    int px = -1;
    int py = 0;
    int l = 0;

    for (int i = 0; i < MATRIX_WIDTH * MATRIX_HEIGHT / 2; i++) {
        /*Serial.print(i);
        Serial.print(": ");*/
        if (direction == 0 && l > wMax) {
            l = 0;
            wMax--;
            hMax--;
            direction = 1;
            /*Serial.print("reset l=0, wMax=");
            Serial.print(wMax);
            Serial.print(", hMax=");
            Serial.print(hMax);
            Serial.print(", direction=");
            Serial.print(direction);*/
        } else if (direction == 1 && l > hMax) {
            l = 0;
            wMax--;
            hMax--;
            direction = 2;
            /*Serial.print("reset l=0, wMax=");
            Serial.print(wMax);
            Serial.print(", hMax=");
            Serial.print(hMax);
            Serial.print(", direction=");
            Serial.print(direction);*/
        } else if (direction == 2 && l > wMax) {
            l = 0;
            wMax--;
            hMax--;
            direction = 3;
            /*Serial.print("reset l=0, wMax=");
            Serial.print(wMax);
            Serial.print(", hMax=");
            Serial.print(hMax);
            Serial.print(", direction=");
            Serial.print(direction);*/
        } else if (direction == 3 && l > hMax) {
            l = 0;
            wMax--;
            hMax--;
            direction = 0;
            /*Serial.print("reset l=0, wMax=");
            Serial.print(wMax);
            Serial.print(", hMax=");
            Serial.print(hMax);
            Serial.print(", direction=");
            Serial.print(direction);*/
        }

        if (direction == 0) px++;
        if (direction == 1) py++;
        if (direction == 2) px--;
        if (direction == 3) py--;

        leds[toLedPos(px, py)] = v_swirl_colorBuf[i];
        leds[toLedPos(MATRIX_WIDTH - px - 1, MATRIX_HEIGHT - py - 1)] = v_swirl_colorBuf[i];

        /*Serial.print("p=");
        Serial.print(px);
        Serial.print(",");
        Serial.println(py);*/

        l++;
    }
}