#define FASTLED_ESP8266_RAW_PIN_ORDER   // Makes WS2812B work on the ESP

#include "Settings.h"                   // Settings file
#include "Log.h"                        // Logging
#include <Arduino.h>                    // Standard Arduino libraries
#include <FS.h>                         // SPIFFS
#include <ESP8266WiFi.h>                // WiFi interfaces
#include <ESP8266mDNS.h>                // mDNS controller
#include <ESP8266WebServer.h>           // WebServer for http request handling
#include <FastLED.h>                    // FastLED for controlling WS2812B
#include "FileIO.h"                     // Handles SPIFFS input / output
#include "GifDecoder.h"                 // Custom lib for decoding gif files
#include "Visualization.h"              // Handles the fft visualizations

FASTLED_USING_NAMESPACE



// HTTP Method strings for debugging
#ifdef SERIAL_DEBUG
    const char* httpMethodStrings[] = { "ANY", "GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS" };
#endif

// Available modes
#define MODE_ANI    0
#define MODE_VIS    1


// WiFi conencted handler
WiFiEventHandler onConnectedHandler;

// HTTP WebServer
ESP8266WebServer webserver(WEBSERVER_PORT);
File currentUploadFile;

// Gif format decoder
GifDecoder<MATRIX_WIDTH, MATRIX_HEIGHT, 12> gifDecoder;

// Visualization handler
Visualization visualization;

// FastLED array represents the led strip
#define NUM_LEDS MATRIX_WIDTH * MATRIX_HEIGHT
CRGB leds[NUM_LEDS];

// Current mode and automatic cycling through all animations
unsigned int mode;
unsigned int cycleDelay;
unsigned long nextCycle;




/************************************
 *    HELPER AND CONTROL METHODS    *
 ************************************/

String generateAnimationFileName() {
    String prefix = String(DIR_ANIMATIONS) + "/";
    for (int i = 0; i < MAX_NUM_ANIMATIONS; i++) {

        String fileName = prefix + i + ".gif";
        if (!SPIFFS.exists(fileName)) {
            return fileName;
        }
    }

    FATAL("Could not get an empty filename.");
    return String();
}

void resetNextCycle() {
    nextCycle = millis() + cycleDelay * 1000;
}

void loadNextAnimation() {
    // Load the new file
    FileIO::nextGifFile();

    // Start the decoder
    gifDecoder.startDecoding();

    // Reset the delay for the next cycle
    resetNextCycle();
}

void loadPrevAnimation() {
    // Load the new file
    FileIO::prevGifFile();

    // Start the decoder
    gifDecoder.startDecoding();

    // Reset the delay for the next cycle
    resetNextCycle();
}




/*********************************
 *    MATRIX RENDER CALLBACKS    *
 *********************************/

void onGifScreenClear() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void onGifUpdateScreen() {
    // Nothing to do here, we update the leds from the loop method.
}

void onGifDrawPixel(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
    if (x < 0) return;
    if (x >= MATRIX_WIDTH) return;
    if (y < 0) return;
    if (y >= MATRIX_HEIGHT) return;

    leds[x + y * MATRIX_WIDTH] = CRGB(red, green, blue);
}


/**************************************
 *    HTTP REQUEST AND WIFI EVENTS    *
 **************************************/

void onAnimationFileUpload() {
    // Get the http upload
    HTTPUpload& upload = webserver.upload();

    if (upload.status == UPLOAD_FILE_START) {
        // Create a new upload file and open it
        String fileName = generateAnimationFileName();
        DEBUGF("New gif file: %s\n", fileName.c_str());
        currentUploadFile = SPIFFS.open(fileName, "w");
    } else if (upload.status == UPLOAD_FILE_WRITE && currentUploadFile) {
        // Write a chunk
        currentUploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END && currentUploadFile) {
        // Done. Close the file and send an OK message.
        currentUploadFile.close();
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        // Aborted. Close the file and remove it.
        if (currentUploadFile) {
            currentUploadFile.close();
            if (!SPIFFS.remove(currentUploadFile.name())) {
                WARN("Canceled file could not be removed")
                return;
            }
        }
    } else {
        // Something went wrong. Send an error message
        WARN("File Upload failed.")
        return;
    }
}

bool onApiRequest(String path, HTTPMethod method) {
    // GET number of animations
    if (method == HTTP_GET && path.equals("/api/animations")) {

        // Get the number of gif files and send it over
        int num = FileIO::getNumGifFiles();
        webserver.send(200, "text/plain", String(num));

        return true;
    }

    // GET animation as GIF file
    if (method == HTTP_GET && path.startsWith("/api/animations/")) {

        // Get the file index
        path.replace("/api/animations/", "");
        int fileIndex = path.toInt();

        // Send over the file if it exists
        String fileName = FileIO::getNthGifFileName(fileIndex);
        File file = SPIFFS.open(fileName, "r");

        if (file) {
            webserver.streamFile(file, "image/gif");
        } else {
            webserver.send(404, "text/plain", "Gif file not found.");
        }

        return true;
    }

    // POST new gif file
    if (method == HTTP_POST && path.equals("/api/animations")) {

        // Send 200, the upload will be handled from onAnimationFileUpload()
        webserver.send(200);
        return true;
    }

    // DELETE existing gif file
    if (method == HTTP_DELETE && path.startsWith("/api/animations/")) {

        // Get the file index
        path.replace("/api/animations/", "");
        int fileIndex = path.toInt();

        // Send over the file
        String fileName = FileIO::getNthGifFileName(fileIndex);

        // Check if the file exists
        if (!SPIFFS.exists(fileName)) {
            webserver.send(404, "text/plain", "Gif file not found.");
        }

        // Try to remove
        if (SPIFFS.remove(fileName)) {
            webserver.send(200);
        } else {
            webserver.send(500, "text/plain", "Could not delete gif file.");
        }

        return true;
    }

    // Load next animation
    if (method == HTTP_POST && path.equals("/api/control/next")) {
        // Load the next gif file and send an ok status
        loadNextAnimation();
        webserver.send(200);
    }

    // Load previous animation
    if (method == HTTP_POST && path.equals("/api/control/prev")) {
        // Load the previous gif file and send an ok status
        loadPrevAnimation();
        webserver.send(200);
    }

    // Get the cycle delay
    if (method == HTTP_GET && path.equals("/api/control/cycle")) {
        webserver.send(200, "text/plain", String(cycleDelay));
    }

    // Set the cycle delay
    if (method == HTTP_POST && path.equals("/api/control/cycle")) {
        // Get the value
        int value = webserver.arg("plain").toInt();

        // Value must be larger than or equal to 0
        if (value < 0) {
            webserver.send(400, "text/plain", "Invalid value for delay.");
            return true;
        }

        // Finally, set the new value and reset the cycle delay
        // (The animation would switch immediately when changing from 0 or a high number to a lower one)
        cycleDelay = value;
        resetNextCycle();

        webserver.send(200);
        return true;
    }

    // Get the current mode
    if (method == HTTP_GET && path.equals("/api/control/mode")) {
        webserver.send(200, "text/plain", String(mode));
    }

    // Set the current mode
    if (method == HTTP_POST && path.equals("/api/control/mode")) {
        int newMode = webserver.arg("plain").toInt();

        if (newMode == MODE_ANI || newMode == MODE_VIS) {
            mode = newMode;
            webserver.send(200);
        } else {
            webserver.send(400, "text/plain", "Invalid value for mode");
        }

        return true;
    }

    // We got an invalid path, returning false will send a 404 Not Found response
    return false;
}

void onFileUpload() {
    // Handle the animation (gif) file upload
    if (webserver.uri() == "/api/animations") onAnimationFileUpload();
}

void onHttpRequest() {
    // Get the path and method
    HTTPMethod method = webserver.method();
    String path = webserver.uri();

    DEBUG(httpMethodStrings[method]);
    DEBUG(" ");
    DEBUGLN(path);
    
    // Handle CORS
    webserver.sendHeader("Access-Control-Allow-Origin", "*");
    webserver.sendHeader("Access-Control-Max-Age", "10000");
    webserver.sendHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
    webserver.sendHeader("Access-Control-Allow-Headers", "*");

    if (method == HTTP_OPTIONS) {
        webserver.send(204);
        return;
    }

    // Handle api request and return, if successful
    if (path.startsWith("/api/")) {
        if (onApiRequest(path, method)) {
            return;
        } else {
            webserver.send(404, "text/plain", "404 Not Found. Yikes.");
            return;
        }
    }

    // Link the root path to the index.html file
    if (path.equals("/")) {
        path = "/index.html";
    }

    // Get the mime type
    String mimeType = "text/plain";
    if (path.endsWith(".html")) mimeType = "text/html";
    if (path.endsWith(".css")) mimeType = "text/css";
    if (path.endsWith(".js")) mimeType = "application/javascript";
    if (path.endsWith(".map")) mimeType = "application/octet-stream";
    if (path.endsWith(".ico")) mimeType = "image/x-icon";

    // Load the requested file from the webserver root directory
    File file = SPIFFS.open(String(DIR_HTML_ROOT) + path, "r");
    if (file) {
        // File exists. Send it over the response.
        webserver.streamFile(file, mimeType);
    } else {
        // File does not exist. Respond 404. TODO: Load a 404 File
        webserver.send(404, "text/html", "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found.</h1><p>Yikes.</p></body></html>");
    }
}

void onConnected(const WiFiEventStationModeConnected& event) {
    DEBUGF("Connected to %s\n", WIFI_SSID)

    // Start mdns
    if (MDNS.begin(WIFI_HOSTNAME)) {
        DEBUGF("mDNS service running as: %s\n", WIFI_HOSTNAME)
    } else {
        WARN("Could not start mDNS service")
    }

    // Start webserver
    webserver.begin();
    DEBUGLN("Webserver running")
}


/*****************************
 *    MAIN SETUP FUNCTION    *
 *****************************/

void setup() {
    // Init serial
    Serial.begin(SERIAL_BAUD_RATE);
    #ifdef SERIAL_DEBUG
        delay(1000);
    #endif
    DEBUGLN("\n\nLED Matrix Controller. Developed by Amon Benson.\n")

    // Debug the matrix size to the serial output
    #ifdef SERIAL_MATRIX_DATA
        Serial.printf("LEDINFO%.2X%.2X\n", MATRIX_WIDTH, MATRIX_HEIGHT);
    #endif

    // Init variables TODO: Load from file / store when changed
    mode = MODE_ANI;
    cycleDelay = 10;
    resetNextCycle();

    // Init the SPIFFS and open the first gif file
    FileIO::init(DIR_ANIMATIONS);

    // Set the Gif decoder callback methods
    gifDecoder.setScreenClearCallback(onGifScreenClear);
    gifDecoder.setUpdateScreenCallback(onGifUpdateScreen);
    gifDecoder.setDrawPixelCallback(onGifDrawPixel);

    gifDecoder.setFileSeekCallback(FileIO::onGifFileSeek);
    gifDecoder.setFilePositionCallback(FileIO::onGifFilePosition);
    gifDecoder.setFileReadCallback(FileIO::onGifFileRead);
    gifDecoder.setFileReadBlockCallback(FileIO::onGifFileReadBlock);

    // Load the first animation
    loadNextAnimation();

    // Init the visualization colors
    visualization.colors[0] = CRGB(255, 0, 255);
    visualization.colors[1] = CRGB(0, 255, 255);
    visualization.colors[2] = CRGB(255, 255, 255);
    visualization.colors[3] = CRGB(100, 100, 255);

    // Init WiFi
    DEBUGLN("Initializing WiFi")

    WiFi.disconnect();
    WiFi.persistent(false);
    onConnectedHandler = WiFi.onStationModeConnected(onConnected);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(WIFI_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PSK);

    // Initialize the webserver. The on not found handler will handle all html file requests
    webserver.onNotFound(onHttpRequest);

    // Animation file upload must be handled specially
    webserver.on("/api/animations", HTTP_POST, []() {
        webserver.sendHeader("Access-Control-Allow-Origin", "*");
        webserver.sendHeader("Access-Control-Max-Age", "10000");
        webserver.sendHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
        webserver.sendHeader("Access-Control-Allow-Headers", "*");
        webserver.send(200, "text/plain", "");
    }, onAnimationFileUpload);
}


/****************************
 *    MAIN LOOP FUNCTION    *
 ****************************/

void loop() {
    // Update mdns
    MDNS.update();

    // Check for incoming http requests
    webserver.handleClient();

    // ======== ANIMATION MODE ========
    if (mode == MODE_ANI) {
        // Cycle through all animations
        if (cycleDelay > 0 && millis() > nextCycle) {
            resetNextCycle();
            loadNextAnimation();
        }

        // Decode frame will handle the delay
        gifDecoder.decodeFrame();
    }

    // ======== VISUALIZATION MODE ========
    if (mode == MODE_VIS) {
        // Update will take a sample, create the fft and update the visualization
        visualization.update(leds);
    }


    FastLED.show();

    // Debug the current led data to the serial output
    #ifdef SERIAL_MATRIX_DATA
        Serial.print("LEDDATA");
        for (int i = 0; i < NUM_LEDS; i++) {
            Serial.printf("%.2X%.2X%.2X", leds[i].r, leds[i].g, leds[i].b);
        }
        Serial.println();
    #endif
}