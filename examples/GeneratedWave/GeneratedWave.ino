#include <SPI.h>
#include <TFT_eSPI.h>
#include <Arduino.h>

#define	MAXWAVE     220
#define	MAXSPEC     47
#define WAVEHEIGHT  49

#define DIV_STEP    0.2
#define DIV_MAX     650


TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

int w,h;
int wavX,wavY,wavH,spX,spY,spW,spH;
SpectrumAnalyzer *spec;

void setup() {
  Serial.begin(115200);

  //highest clockspeed needed
  esp_pm_lock_handle_t powerManagementLock;
  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "PowerManagementLock", &powerManagementLock);
  esp_pm_lock_acquire(powerManagementLock);

  Serial.println("Starting ESP32 Spectrum Analyzer");

  tft.init();

  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);

  w = tft.width();
  h = tft.height();

  wavX = 1;
  wavY = 1;
  wavH = WAVEHEIGHT;

  spX = 0;
  spY = WAVEHEIGHT+1;
  spW = w;
  spH = h-spY;

  tft.fillRect(0,0,w,h,TFT_BLACK);

  spec = new SpectrumAnalyzer(tft,MAXWAVE,MAXSPEC);
  spec->waveView(wavX,wavY,wavH,TFT_WHITE,TFT_RED,TFT_BLACK);
  spec->specView(spX+2,spY+2,spW-4,spH-4,TFT_YELLOW,TFT_BLACK);

  tft.drawRect(wavX-1,wavY-1,MAXWAVE+2,WAVEHEIGHT+2,TFT_WHITE);
  tft.drawRect(spX,spY,spW,spH,TFT_WHITE);

  spec->clearWaveView();
  spec->clearSpecView();
}

void loop() {
  spec->generateWave(divisor);
  divisor += DIV_STEP;
  if (divisor > DIV_MAX)
    divisor = 0;
}