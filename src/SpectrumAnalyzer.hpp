#pragma once

#include <Arduino.h>

#define BUFLEN 2048
#define GAIN   1500

template <class Graphics> class SpectrumAnalyzer {
  private:
    Graphics &g;

    int8_t *buffer;
    int     bufferReadOfs;
    int     bufferWriteOfs;
    int     bufferSamples;

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

    int *fftTable;

    TaskHandle_t hProcessTask;

    static void processTask(void *context)
    {
      SpectrumAnalyzer<Graphics> *spec = reinterpret_cast<SpectrumAnalyzer<Graphics> *>(context);
      const TickType_t            xMaxBlockTime = pdMS_TO_TICKS(100);
      while (true) {
        uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        if (ulNotificationValue > 0) {
          while (spec->bufferSamples >= spec->waveSamples)
            spec->process();
        }
      }
    }

    void drawSample(int32_t x, int32_t y, int32_t &yo)
    {
      if (y == yo)
        return;
      g.drawPixel(x, yo, yo == waveYmid ? waveMid : waveBg);
      g.drawPixel(x, y, waveFg);
      yo = y;
    }

    void drawBar(int32_t x, int32_t y, int32_t &yo)
    {
      if (y == yo)
        return;
      int32_t len = abs(y - yo);
      if (y <= yo)
        g.fillRect(x, y, specBarWidth, len + 1, specFg);
      else
        g.fillRect(x, yo, specBarWidth, len, specBg);
      yo = y;
    }

    void process()
    {
      int8_t wave[waveSamples];

      int cnt = 0;
      int sx = specX;
      for (int k = 0; k < specBars; k++) {
        int     a = 0;
        int     b = 0;
        int32_t wx = waveX;
        for (int n = 0; n < waveSamples; n++) {
          int8_t &w = wave[n];
          if (k == 0) {
            // get sample
            w = buffer[bufferReadOfs];
            bufferReadOfs = (bufferReadOfs + 1) % BUFLEN;
            if (bufferSamples-- <= 0) {
              Serial.println("overrun");
              return;
            }
            if (waveViewInitialized)
              drawSample(wx++, waveYmap[(uint8_t)w], waveValues[n]);
          }
          a += fftTable[cnt++] * w;
          b += fftTable[cnt++] * w;
        }
        a /= GAIN;
        b /= GAIN;
        int sum = (a * a + b * b);
        if (specViewInitialized)
          drawBar(sx, sum >= specYmapSize ? specYmap[specYmapSize - 1] : specYmap[sum], specValues[k]);
        sx += specBarWidth + 1;
      }
    }

  public:
    SpectrumAnalyzer(Graphics &graphics, int samples, int bars) : g(graphics)
    {
      buffer = (int8_t *)malloc((BUFLEN + 1) * sizeof(int8_t));
      bufferReadOfs = bufferWriteOfs = bufferSamples = 0;

      waveSamples = samples;
      waveValues = (int *)malloc(waveSamples * sizeof(int));
      waveYmap = NULL;

      specBars = bars;
      specValues = (int *)malloc(specBars * sizeof(int));
      specYmap = NULL;

      fftTable = (int *)malloc(specBars * waveSamples * 2 * sizeof(int));
      int   cnt = 0;
      float k1 = 1;
      for (int k = 0; k < specBars; k++) {
        for (int n = 0; n < waveSamples; n++) {
          float a = 2 * PI * (k1 * n / waveSamples);
          fftTable[cnt++] = round(INT8_MAX * sin(a) * sin(PI * n / waveSamples));
          fftTable[cnt++] = round(INT8_MAX * cos(a) * sin(PI * n / waveSamples));
        }
        k1 = k1 + 1;
      }

      waveViewInitialized = false;
      specViewInitialized = false;

      if (buffer && waveValues && specValues && fftTable)
        xTaskCreatePinnedToCore(processTask, "processTask", 4096, this, 2, &hProcessTask, 1);
      else
        Serial.println("Initialization error.");
    }

    void waveView(int x, int y, int h, uint32_t fg, uint32_t mid, uint32_t bg)
    {
      waveX = x;
      waveBg = bg;
      waveFg = fg;
      waveMid = mid;

      if (waveYmap != NULL)
        free(waveYmap);

      waveYmap = (uint16_t *)malloc(256 * sizeof(uint16_t));
      for (int i = 0; i <= UINT8_MAX; i++)
        waveYmap[i] = map((int8_t)i, INT8_MIN, INT8_MAX, y + h - 1, y);
      waveYmid = waveYmap[0];
      for (int n = 0; n < waveSamples; n++)
        waveValues[n] = waveYmid; // initialize
      waveViewInitialized = true;
    }

    void specView(int x, int y, int w, int h, uint32_t fg, uint32_t bg)
    {
      specBg = bg;
      specFg = fg;

      if (specYmap != NULL)
        free(specYmap);

      specBarWidth = floor((w - 1) / specBars) - 1;
      specX = x + 1 + ((w - 1 - (specBars * (specBarWidth + 1))) / 2); // centered

      specYmapSize = (h - 1) * (h - 1); // min 1 pixel max height^2
      specYmap = (uint16_t *)malloc(specYmapSize * sizeof(uint16_t));
      for (int i = 0; i < specYmapSize; i++) {
        specYmap[i] = y + (h - 1) - sqrt(i); // y coordinate of the sqrt value
      }
      for (int k = 0; k < specBars; k++)
        specValues[k] = specYmap[0]; // initialize
      specViewInitialized = true;
    }

    void clearWaveView()
    {
      if (!waveViewInitialized)
        return;

      int32_t x = waveX;
      int32_t y = waveYmap[(uint8_t)INT8_MAX];
      int32_t w = waveSamples;
      int32_t h = waveYmap[(uint8_t)INT8_MIN] - waveYmap[(uint8_t)INT8_MAX] + 1;

      g.fillRect(x, y, w, h, waveBg);
      for (int i = 0; i < waveSamples; i++)
        g.drawPixel(x + i, waveYmid, waveMid);
    }

    void clearSpecView()
    {
      if (!specViewInitialized)
        return;
      int32_t x = specX;
      int32_t y = specYmap[specYmapSize - 1];
      int32_t w = specBars * (specBarWidth + 1);
      int32_t h = specYmap[0] - y + 1;
      g.fillRect(x, y, w, h, specBg);
      int barx = specX;
      for (int i = 0; i < specBars; i++) {
        uint16_t bary = specYmap[0];
        specValues[i] = bary;
        g.fillRect(barx, bary, specBarWidth, 1, specFg);
        barx += specBarWidth + 1;
      }
    }

    void addSample(const int8_t sample)
    {
      if (!buffer)
        return;
      if (bufferSamples <= BUFLEN) {
        buffer[bufferWriteOfs] = sample;
        bufferWriteOfs = (bufferWriteOfs + 1) % BUFLEN;
        bufferSamples++;
      }
      else {
        Serial.println("underrun");
      }
    }

    void generateWave(double divisor)
    {
      double step = waveSamples / 2.0 * PI / divisor;
      for (int i = 0; i < waveSamples; i++) {
        int8_t sample = INT8_MAX * sin(i / step);
        addSample(sample);
      }
      xTaskNotify(hProcessTask, 1, eIncrement);
    }

    void draw() { xTaskNotify(hProcessTask, 1, eIncrement); }
};