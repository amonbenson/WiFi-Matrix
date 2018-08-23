/*
 * ESP8266 WiFI RGB LED Matrix main file.
 * 
 * This file contains the full code neccessary to drive the RGB Matrix, except for the gif decoding
 * (which can be found in GifDecoder_Impl.h).
 * 
 * Written by: Amon Benson
 * 
 * Copyright (c) 2018 Amon Benson
 * 
 * This file is part of WiFi-Matrix.
 *
 * WiFi-Matrix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * WiFi-Matrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with WiFi-Matrix. If not, see <http://www.gnu.org/licenses/>.
 */


// INCLUDES


#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>

// Include NeoPixelBus
#include <NeoPixelBus.h>

// Include the GIF file decoder and settings files
#include "GifDecoder.h"
#include "Settings.h"


// DEFINES AND GLOBAL VARIABLES


#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT) // Number of leds on the strip
#define LZW_MAX_BITS 10 // 10 - 12. Should be kept as low as possible

// GIF decoder object
GifDecoder<MATRIX_WIDTH, MATRIX_HEIGHT, LZW_MAX_BITS> decoder;

// Current gif file object
int currentFileIndex = 0, numFiles = 0;
File file;

// next animation change time
bool cycleAnimations = true;
unsigned long timeAnimationChange;

// led strip object (and gamma corrector)
NeoGamma<NeoGammaTableMethod> colorGamma;
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUM_LEDS);

// HTTP server
ESP8266WebServer server(80);


// GLOBALLY ACCESSABLE HELPER METHODES


void playAnimationCycle() {
    timeAnimationChange = millis() + ANIMATION_CHANGE_INTERVAL;
    cycleAnimations = true;
}

void pauseAnimationCycle() {
    cycleAnimations = false;
}

void selectNextAnimationFile() {
    currentFileIndex++;
    if (currentFileIndex > numFiles - 1) currentFileIndex = 0;
}

void selectPrevAnimationFile() {
    currentFileIndex--;
    if (currentFileIndex < 0) currentFileIndex = numFiles - 1;
}

void updateNumFiles() {
    // Reset and count all files in the animation directory
    numFiles = 0;
    Dir dir = SPIFFS.openDir(D_ANIMATIONS);
    while (dir.next()) {
        numFiles++;
    }
}

bool loadCurrentAnimationFile() {
    Serial.print("Loading animation: ");
    Serial.println(currentFileIndex);

    // Close up the old animation file
    if (file) file.close();

    // Store the current file index in the direction
    int i = 0;

    // Loop through the directory. If the file with the current index is found, open it and start the decoder.
    Dir dir = SPIFFS.openDir(D_ANIMATIONS);
    while (dir.next()) {
        if (i == currentFileIndex) {
            file = dir.openFile("r");
            decoder.startDecoding();
            return true;
        }
        i++;
    }

    // File wasn't found
    return false;
}


// GIF DECODER CALLBACKS


void screenClearCallback(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.SetPixelColor(i, RgbColor(0, 0, 0));
    }
}

void updateScreenCallback(void) {
    strip.Show();
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    if (x > MATRIX_WIDTH - 1) return;
    if (y > MATRIX_WIDTH - 1) return;

    int pos = 0;
    if (y % 2 == 0) pos = y * MATRIX_WIDTH + x;
    else pos = y * MATRIX_WIDTH + 9 - x;

    strip.SetPixelColor(pos, colorGamma.Correct(RgbColor(red, green, blue)));
}

bool fileSeekCallback(unsigned long position) {
    if (!file) {
        Serial.println("Could not seek: File is not available");
        return false;
    }
    return file.seek(position);
}

unsigned long filePositionCallback(void) {
    if (!file) {
        Serial.println("Could not get position: File is not available");
        return 0;
    }
    return file.position();
}

int fileReadCallback(void) {
    if (!file) {
        Serial.println("Could not read: File is not available");
        return -1;
    }
    return file.read();
}

int fileReadBlockCallback(void * buffer, int numberOfBytes) {
    if (!file) {
        Serial.println("Could not read block: File is not available");
        return -1;
    }
    return file.readBytes((char*) buffer, numberOfBytes);
}

void displaySplash() {
    for (int x = 0; x < MATRIX_WIDTH; x++) {
        for (int y = 0; y < MATRIX_HEIGHT; y++) {
            int red = x * 255 / MATRIX_WIDTH;
            int green = y * 255 / MATRIX_HEIGHT;
            int blue = 255;
            drawPixelCallback(x, y, red, green, blue);
        }
    }
}


// HTTP SERVER CALLBACKS AND WIFI CONNECTION


void handleRootCallback() {
    File file = SPIFFS.open(F_WEBINTERFACE, "r");
    if (!file) {
        Serial.println("Could not read webinterface html!");
        return;
    }

    server.streamFile(file, "text/html");
}

void handlePlayCallback() {
    playAnimationCycle();

    server.send(200, "text/plain", "success");
}

void handlePauseCallback() {
    pauseAnimationCycle();

    server.send(200, "text/plain", "success");
}

void handleNextCallback() {
    selectNextAnimationFile();
    loadCurrentAnimationFile();

    server.send(200, "text/plain", "success");
}

void handlePrevCallback() {
    selectPrevAnimationFile();
    loadCurrentAnimationFile();

    server.send(200, "text/plain", "success");
}

void connectWiFi() {
    // Start WiFi
    WiFi.mode(WIFI_STA);
    WiFi.hostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PSK);
    Serial.print("  Connecting ");

    // Wait for a connection
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    MDNS.begin(WIFI_HOSTNAME);

    // Start the http server
    server.begin();

    Serial.println(" done");
    Serial.print("  IP-Address: ");
    Serial.println(WiFi.localIP());
}


// SETUP AND LOOP


void setup() {
    // Wait for Serial to become available and init the logger
#if INITIAL_DELAY == 1
    delay(3000);
#endif
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println("\n\n\nWiFi-Matrix");
    Serial.println("Copyright (c) 2018 Amon Benson\n");
    Serial.println("This program comes with ABSOLUTELY NO WARRANTY");
    Serial.println("This is free software, and you are welcome to redistribute it");
    Serial.println("under certain conditions\n\n\n");

    // Init the led strip and display a litte splash screen
    Serial.println("Initializing: leds");
    strip.Begin();
    displaySplash();
    strip.Show();

    // Link the callback methodes
    Serial.println("Initializing: GIF decoder");
    decoder.setScreenClearCallback(screenClearCallback);
    decoder.setUpdateScreenCallback(updateScreenCallback);
    decoder.setDrawPixelCallback(drawPixelCallback);

    decoder.setFileSeekCallback(fileSeekCallback);
    decoder.setFilePositionCallback(filePositionCallback);
    decoder.setFileReadCallback(fileReadCallback);
    decoder.setFileReadBlockCallback(fileReadBlockCallback);

    // Init the spiffs
    Serial.println("Initializing: File system");
    SPIFFS.begin();

    // Init the http server
    server.on("/", handleRootCallback);
    server.on("/control/play", handlePlayCallback);
    server.on("/control/pause", handlePauseCallback);
    server.on("/control/next", handleNextCallback);
    server.on("/control/prev", handlePrevCallback);
    server.onNotFound(handleRootCallback);

    // Connect to the WiFi
    Serial.println("Initializing: WiFi-Connection ...");
    connectWiFi();
    
    Serial.println("Initializing: Animation player");

    // Initially count the number of files and load the first aninmation
    updateNumFiles();

    timeAnimationChange = millis() + ANIMATION_CHANGE_INTERVAL;
    loadCurrentAnimationFile();

    // Init done
    Serial.println("Initialization done!");
}

void loop() {
    // Reconnect if WiFi connection got lost
    if (WiFi.status() != WL_CONNECTED) connectWiFi();

    // Handle any http clients
    server.handleClient();

    // Load in the next file (if cycling)
    if (millis() >= timeAnimationChange && cycleAnimations) {
        timeAnimationChange = millis() + ANIMATION_CHANGE_INTERVAL;

        selectNextAnimationFile();
        loadCurrentAnimationFile();
    }

    // Decode a single frame
    if (file) decoder.decodeFrame();
}