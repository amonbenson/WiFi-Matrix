#ifndef SETTINGS_H
#define SETTINGS_H


#define SERIAL_DEBUG                    // Send debug data via Serial Monitor
#define SERIAL_MATRIX_DATA              // Send the Led-Data as Hex-Encoded String via Serial Monitor. Must be enabled for the Virtual Matrix

#define SERIAL_BAUD_RATE                115200 // Serial baud rate used for debugging and the virtual matrix

// The uploaded gif files must have the same dimensions as the matrix. Therefore, if the dimensions are changed here, all
// gif files need to be reuploaded. The provided example files have a resolution of 12x12.
// Also, the animations have only been tested with square matrices and may not work, if the width and height have different values.
#define MATRIX_WIDTH                    12 // X-Dimension of the matrix
#define MATRIX_HEIGHT                   12 // Y-Dimension of the matrix

// Pin for the NeoPixel led strip and the microphone
#define PIN_LEDS                        D8
#define PIN_MICROPHONE                  A0

// Wifi credentials and hostname / mdns name of the esp
#define WIFI_SSID                       "Wifi SSID"
#define WIFI_PSK                        "The Password"
#define WIFI_HOSTNAME                   "matrix"

// Webserver port. This should not be changed, as the webserver is configured to use port 80
#define WEBSERVER_PORT                  80
#define DIR_HTML_ROOT                   "/htdocs"
#define DIR_ANIMATIONS                  "/animations"

// Maximum number of animations that can be uploaded.
#define MAX_NUM_ANIMATIONS              100


#endif