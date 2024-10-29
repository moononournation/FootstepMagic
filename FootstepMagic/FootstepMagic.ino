// Value tobe adjust to your measured value
#define SHAKE_TIME_FRAME 100
#define SHAKE_TRIGGER_THRESHOLD 8

/*******************************************************************************
 * AVI Player example
 *
 * Dependent libraries:
 * Arduino_GFX: https://github.com/moononournation/Arduino_GFX.git
 * avilib: https://github.com/lanyou1900/avilib.git
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 *
 * Setup steps:
 * 1. Change your LCD parameters in Arduino_GFX setting
 * 2. Upload AVI file
 *   FFat/LittleFS:
 *     upload FFat (FatFS) data with ESP32 Sketch Data Upload:
 *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
 *   SD:
 *     Copy files to SD card
 *
 * Video Format:
 * code "cvid": Cinepak
 * cod "MJPG": MJPEG
 ******************************************************************************/
const char *root = "/root";
// char *avi_filename = (char *)"/root/cinepak100x88.avi";
char *avi_filename = (char *)"/root/mjpeg100x88.avi";

// uncomment one of below dev device pin definition header
#include "T_QT.h"

#define SHAKE_SENSOR_PINA 16
#define SHAKE_SENSOR_PINB 17

#include <FFat.h>
#include <LittleFS.h>
#include <SPIFFS.h>
#include <SD.h>
#include <SD_MMC.h>

size_t output_buf_size;
uint16_t *output_buf;

#include "AviFunc_callback.h"
#ifdef AVI_SUPPORT_AUDIO
#include "esp32_audio.h"
#endif

unsigned long shake_start = 0;
int shake_count = 0;
bool triggered = true;

void IRAM_ATTR ISR()
{
  if (shake_start < (millis() - SHAKE_TIME_FRAME))
  {
    shake_start = millis();
    shake_count = 0;
  }
  ++shake_count;
  if (shake_count > SHAKE_TRIGGER_THRESHOLD)
  {
    triggered = true;
  }
}

int avi_x_offset = 14, avi_y_offset = 24;
// drawing callback
void draw_callback(uint16_t x, uint16_t y, uint16_t *p, uint16_t w, uint16_t h)
{
  // Serial.printf("draw_callback(%d, %d, *p, %d, %d)\n", x, y, w, h);

  unsigned long s = millis();
  gfx->draw16bitBeRGBBitmap(avi_x_offset + x, avi_y_offset + y, p, w, h);
  s = millis() - s;
  avi_total_show_video_ms += s;
  avi_total_decode_video_ms -= s;
}

int drawMCU(JPEGDRAW *pDraw)
{
  // Serial.printf("Draw pos = (%d, %d), size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);

  unsigned long s = millis();
  gfx->draw16bitBeRGBBitmap(avi_x_offset + pDraw->x, avi_y_offset + pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  s = millis() - s;
  avi_total_show_video_ms += s;
  avi_total_decode_video_ms -= s;

  return 1;
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("AVI Player - foostep magic revision");

  // If display and SD shared same interface, init SPI first
#ifdef SPI_SCK
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
#endif

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  // if (!gfx->begin())
  if (!gfx->begin(GFX_SPEED))
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR < 3)
  ledcSetup(0, 1000, 8);
  ledcAttachPin(GFX_BL, 0);
  ledcWrite(0, 0);
#else  // ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(GFX_BL, 1000, 8, 1);
  ledcWrite(GFX_BL, 0);
#endif // ESP_ARDUINO_VERSION_MAJOR >= 3
#endif // GFX_BL

  // gfx->setTextColor(WHITE, BLACK);
  // gfx->setTextBound(60, 60, 240, 240);

#ifdef AVI_SUPPORT_AUDIO
#ifdef AUDIO_MUTE_PIN
  pinMode(AUDIO_MUTE_PIN, OUTPUT);
  digitalWrite(AUDIO_MUTE_PIN, HIGH);
#endif

  i2s_init();
#endif

#if defined(SD_D1)
#define FILESYSTEM SD_MMC
  SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */, SD_D1, SD_D2, SD_CS /* D3 */);
  if (!SD_MMC.begin(root, false /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
#elif defined(SD_SCK)
#define FILESYSTEM SD_MMC
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */);
  if (!SD_MMC.begin(root, true /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
#elif defined(SD_CS)
#define FILESYSTEM SD
  if (!SD.begin(SD_CS, SPI, 80000000, "/root"))
#else
#define FILESYSTEM FFat
  if (!FFat.begin(false, root))
  // if (!LittleFS.begin(false, root))
  // if (!SPIFFS.begin(false, root))
#endif
  {
    Serial.println("ERROR: File system mount failed!");
  }
  else
  {
    output_buf_size = gfx->width() * 4 * 2;
    output_buf = (uint16_t *)heap_caps_aligned_alloc(16, output_buf_size * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!output_buf)
    {
      Serial.println("output_buf aligned_alloc failed!");
    }

    avi_init();
  }

  pinMode(SHAKE_SENSOR_PINA, OUTPUT);
  digitalWrite(SHAKE_SENSOR_PINA, LOW);
  pinMode(SHAKE_SENSOR_PINB, INPUT_PULLUP);
  attachInterrupt(SHAKE_SENSOR_PINB, ISR, FALLING);
}

void loop()
{
  if (triggered)
  {
    if (avi_open(avi_filename))
    {
      Serial.println("AVI start");
      gfx->fillScreen(BLACK);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR < 3)
      ledcWrite(0, 0);
#else  // ESP_ARDUINO_VERSION_MAJOR >= 3
      ledcWrite(GFX_BL, 0);
#endif // ESP_ARDUINO_VERSION_MAJOR >= 3

#ifdef AVI_SUPPORT_AUDIO
      if (avi_aRate > 0)
      {
        i2s_set_sample_rate(avi_aRate);
      }

      avi_feed_audio();

      if (avi_aFormat == PCM_CODEC_CODE)
      {
        Serial.println("Start play PCM audio task");
        BaseType_t ret_val = pcm_player_task_start();
        if (ret_val != pdPASS)
        {
          Serial.printf("pcm_player_task_start failed: %d\n", ret_val);
        }
      }
      else if (avi_aFormat == MP3_CODEC_CODE)
      {
        Serial.println("Start play MP3 audio task");
        BaseType_t ret_val = mp3_player_task_start();
        if (ret_val != pdPASS)
        {
          Serial.printf("mp3_player_task_start failed: %d\n", ret_val);
        }
      }
      else
      {
        Serial.println("No audio task");
      }
#endif

      avi_start_ms = millis();

      Serial.println("Start play loop");
      while (avi_curr_frame < avi_total_frames)
      {
#ifdef AVI_SUPPORT_AUDIO
        avi_feed_audio();
#endif

        if (avi_decode())
        {
          avi_draw();
        }
      }

      avi_close();
      Serial.println("AVI end");

      // avi_show_stat();
    }

    triggered = false;

    int i = 255;
    while ((--i) && (!triggered))
    {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR < 3)
      ledcWrite(0, 0);
#else  // ESP_ARDUINO_VERSION_MAJOR >= 3
      ledcWrite(GFX_BL, 256 - i);
#endif // ESP_ARDUINO_VERSION_MAJOR >= 3
      delay(10);
    }
  }
  else
  {
    Serial.println(shake_count); // measure your step value here
    delay(SHAKE_TIME_FRAME);
  }
}
