#include "Visualization.h"

int toLedPos(int x, int y) {
    return x + y * MATRIX_WIDTH;
}

Visualization::Visualization() {
    currentVis = 2;

    FFT = arduinoFFT();
    fftGain = 0.0005;
}

void Visualization::update(CRGB *leds) {
    // Store the dc offset of the captured signal
    int dcOffset = 0;

    // Capture some samples
    for (int i = 0; i < FFT_SAMPLES; i++) {
        // Get the current mic input value
        #ifdef VISUALIZATION_EMULATE_INPUT
            int val = random(400);
        #else
            int val = analogRead(PIN_MICROPHONE);
        #endif

        // Set the real and imaginary part and update the dc offset
        fftReal[i] = val;
        fftImag[i] = 0;
        dcOffset += val;

        // Short delay to make equidistant captures
        delayMicroseconds(80);
    }

    // Remove the dc offset by subtracting it from all values
    dcOffset /= FFT_SAMPLES;
    for (int i = 0; i < FFT_SAMPLES; i++) fftReal[i] -= dcOffset;

    // Calculate the FFT
    FFT.Windowing(fftReal, FFT_SAMPLES, FFT_WIN_TYP_BLACKMAN_NUTTALL, FFT_FORWARD);
    FFT.Compute(fftReal, fftImag, FFT_SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(fftReal, fftImag, FFT_SAMPLES);

    // Scale the frequency and amplitude and smooth out the fft
    fftPeak = 0;
    fftPeakVal = 0;
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        int sum = 0;
        double val = 0;

        // Average all nearby frequency bands to scale the frequency range to MATRIX_WIDTH
        double l = FFT_SAMPLES / (double) MATRIX_WIDTH / 2;
        for (int i = (int) (x * l); i < (x + 1) * l; i++) {
            val += fftReal[i];
            sum++;
        }

        // Scale the fft
        val *= fftGain / sum;
        val *= exp(x / (double) MATRIX_WIDTH);

        //val *= sqrt(val);

        // Keep the value in bounds and apply the attack and decay
        if (val < 0) val = 0;
        if (val > 1) val = 1;

        if (val > fftVal[x]) fftVal[x] = fftVal[x] * FFT_ATTACK + val * (1 - FFT_ATTACK);
        else fftVal[x] = fftVal[x] * FFT_RELEASE + val * (1 - FFT_RELEASE);
        if (fftVal[x] > fftPeakVal) {
            fftPeakVal = fftVal[x];
            fftPeak = x / (double) (MATRIX_WIDTH - 1);
        }
    }

    // Update the selected visualization
    updateFuncs[currentVis](leds, colorBuf, palette, fftVal, fftPeak, fftPeakVal);
}

void Visualization::nextVis() {
    // Increase the visualization index
    currentVis++;
    if (currentVis >= NUM_VISUALIZATIONS) currentVis = 0;
}

void Visualization::prevVis() {
    // Decrease the visualization index
    currentVis--;
    if (currentVis < 0) currentVis = NUM_VISUALIZATIONS - 1;
}

// Getters and setters for the palette
bool Visualization::setPaletteColor(int index, CRGB color) {
    if (index < 0 || index >= VISUALIZATION_PALETTE_SIZE) return false;
    palette[index] = color;
    return true;
}
CRGB Visualization::getPaletteColor(int index) {
    if (index < 0 || index >= VISUALIZATION_PALETTE_SIZE) return NULL;
    return palette[index];
}
CRGB *Visualization::getPalette() {
    return palette;
}

// BARS renders a spectrum style bargraph
void v_bars(CRGB leds[], CRGB colorBuf[], CRGB palette[], double fftVal[], double fftPeak, double fftPeakVal) {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        // Calc the threshold values
        double thresh_d = MATRIX_HEIGHT - fftVal[x] * MATRIX_HEIGHT;
        int thresh = (int) thresh_d;
        fract8 factor = (fract8) ((1 + thresh - thresh_d) * 255);

        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            CRGB color;
            CRGB color_fg = blend(
                blend(
                    palette[4],
                    palette[3],
                    x * 255 / (MATRIX_WIDTH - 1)
                ),
                blend(
                    palette[2],
                    palette[1],
                    x * 255 / (MATRIX_WIDTH - 1)
                ),
                y * 255 / (MATRIX_HEIGHT - 1)
            );

            if (y > thresh) {
                // Foreground
                color = color_fg;
            } else if (y == thresh) {
                // Blend
                color = blend(palette[0], color_fg, factor);
            } else {
                // Background
                color = palette[0];
            }
            leds[x + y * MATRIX_WIDTH] = color;
        }
    }
}

// SWIRL renders a rotating swirl with colors mapped to the peak frequency and brightness mapped to the amplitude
void v_swirl(CRGB leds[], CRGB colorBuf[], CRGB palette[], double fftVal[], double fftPeak, double fftPeakVal) {
    // Get the new color
    /* int a = fftVal[0] * 255 / MATRIX_HEIGHT;
    int b = max(0, (int) (a - fftVal[MATRIX_WIDTH / 2] * 255 / MATRIX_HEIGHT));
    int c = max(0, (int) (a - fftVal[MATRIX_WIDTH - 1] * 255 / MATRIX_HEIGHT)); */
    
    //CRGB col = CHSV((int) (fftPeak * 255), 255, (int) (fftPeakVal * 128) + 127);
    CRGB col = blend(
        palette[0],
        blend(
            blend(
                palette[1],
                palette[2],
                fftVal[0] * 255
            ),
            blend(
                palette[3],
                palette[4],
                fftVal[MATRIX_WIDTH / 2] * 255
            ),
            fftVal[MATRIX_WIDTH - 1] * 255
        ),
        fftPeakVal * 255
    );

    // Shift it into the color buffer array
    for (int i = MATRIX_WIDTH * MATRIX_HEIGHT / 2 - 1; i > 0; i--)
        colorBuf[i] = colorBuf[i - 1];
    colorBuf[0] = col;

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

        leds[toLedPos(px, py)] = colorBuf[i];
        leds[toLedPos(MATRIX_WIDTH - px - 1, MATRIX_HEIGHT - py - 1)] = colorBuf[i];

        /*Serial.print("p=");
        Serial.print(px);
        Serial.print(",");
        Serial.println(py);*/

        l++;
    }
}

// HEATMAP renders a top to bottom heatmap
void v_heatmap(CRGB leds[], CRGB colorBuf[], CRGB palette[], double fftVal[], double fftPeak, double fftPeakVal) {
    // Shift all columns to the left
    for (int x = 0; x < MATRIX_WIDTH - 1; x++)
        for (int y = 0; y < MATRIX_HEIGHT; y++)
            leds[x + y * MATRIX_WIDTH] = CRGB(leds[x + 1 + y * MATRIX_WIDTH]);

    // Set the right led column to the new color
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        CRGB color;
        double v = fftVal[y * MATRIX_WIDTH / MATRIX_HEIGHT];
        if (v < 0.25) color = blend(palette[0], palette[1], v * 4 * 255);
        else if (v < 0.5) color = blend(palette[1], palette[2], (v * 4 - 1) * 255);
        else if (v < 0.75) color = blend(palette[2], palette[3], (v * 4 - 2) * 255);
        else color = blend(palette[3], palette[4], v * 4 - 3);
        leds[y * MATRIX_WIDTH + MATRIX_WIDTH - 1] = color;
    }
}