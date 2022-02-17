#pragma once

#include <TFT_eSPI.h>

#define BUFLEN 2048
#define GAIN   2000

class SpectrumAnalyzer
{
private:
  TFT_eSPI &tft;

  int8_t   *buffer;
  int       bufferReadOfs;
  int       bufferWriteOfs;
  int       bufferSamples;

  int       waveX;
  uint16_t *waveYmap;
  uint32_t  waveYmid;
  int       waveSamples;
  int32_t  *waveValues;
  uint32_t  waveBg;
  uint32_t  waveMid;
  uint32_t  waveFg;
  bool      waveViewInitialized;

  int       specX;
  uint16_t *specYmap;
  int       specYmapSize;
  int       specBarWidth;
  int       specBars;
  int32_t  *specValues;
  uint32_t  specBg;
  uint32_t  specFg;
  bool      specViewInitialized;

  int      *fftTable;

  TaskHandle_t hProcessTask;
  static void processTask(void *);

  void drawSample(int32_t x, int32_t y, int32_t &yo);
  void drawBar(int32_t x, int32_t y, int32_t &yo);
  void process();

public:
  SpectrumAnalyzer(TFT_eSPI &t, int samples, int bars);
  void waveView(int x, int y, int h, uint32_t fg, uint32_t mid, uint32_t bg);
  void specView(int x, int y, int w, int h, uint32_t fg, uint32_t bg);
  void clearWaveView();
  void clearSpecView();
  void addSample(const int8_t sample);
  void generateWave(double divisor);
  void draw() { xTaskNotify(hProcessTask,1,eIncrement); }
};