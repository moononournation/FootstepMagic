#pragma once

// Display
#include <Arduino_GFX_Library.h>
#define GFX_DEV_DEVICE LILYGO_T_QT
#define GFX_BL 10
Arduino_DataBus *bus = new Arduino_ESP32SPI(6 /* DC */, 5 /* CS */, 3 /* SCK */, 2 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_GC9107(bus, 1 /* RST */, 2 /* rotation */, true /* IPS */);

#define GFX_SPEED 80000000UL

// Button
#define BTN_A_PIN 0

// I2S
//#define I2S_DEFAULT_GAIN_LEVEL 0.5
//#define I2S_OUTPUT_NUM I2S_NUM_0
//#define I2S_MCLK -1
//#define I2S_BCLK 7
//#define I2S_LRCK 5
//#define I2S_DOUT 6
//#define I2S_DIN -1

// #define AUDIO_MUTE_PIN 48   // LOW for mute
