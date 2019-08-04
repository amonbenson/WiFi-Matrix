#ifndef SETTINGS_H
#define SETTINGS_H


#define SERIAL_DEBUG                    // Send debug data via Serial Monitor
#define SERIAL_MATRIX_DATA              // Send the Led-Data as Hex-Encoded String via Serial Monitor. Must be enabled for the Virtual Matrix

#define SERIAL_BAUD_RATE                460800 // Serial baud rate used for debugging and the virtual matrix

// The uploaded gif files must have the same dimensions as the matrix. Therefore, if the dimensions are changed here, all
// gif files need to be reuploaded. The provided example files have a resolution of 12x12.
// Also, the animations have only been tested with square matrices and may not work, if the width and height have different values.
#define MATRIX_WIDTH                    32 // X-Dimension of the matrix
#define MATRIX_HEIGHT                   8 // Y-Dimension of the matrix

// Pin for the NeoPixel led strip and the microphone
#define PIN_LEDS                        D8
#define PIN_MICROPHONE                  A0

// Wifi credentials and hostname / mdns name of the esp
#define WIFI_SSID                       "ssid"
#define WIFI_PSK                        "password"
#define WIFI_HOSTNAME                   "matrix"

// Interval at which to try to reconnect in ms
#define WIFI_RECONNECT_INTERVAL         5000

// Webserver port. This should not be changed, as the webserver is configured to use port 80
#define WEBSERVER_PORT                  80
#define DIR_HTML_ROOT                   "/htdocs"
#define DIR_ANIMATIONS                  "/animations"

// Maximum number of animations that can be uploaded.
#define MAX_NUM_ANIMATIONS              100

// Number of samples for the visualizations. Must be a power of 2 and at least double the size of MATRIX_WIDTH.
// Higher values will give better looking FFTs with slightly increased computation time
#define FFT_SAMPLES                     32

// Speed at which the frequency bands rise (attack) and fall (release).
#define FFT_ATTACK                      0.5
#define FFT_RELEASE                     0.6


#endif