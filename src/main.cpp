#include "Biquad.h"
#include "esp_dsp.h"
#include <Arduino.h>
#include <BiquadFilterTypes.h>
#include <FastLED.h>
#include <driver/i2s.h>
#include <dsps_fft2r.h>

#ifndef WS2812_NUM_LEDS
#define WS2812_NUM_LEDS 6
#endif
#define COLOR_ORDER GRB
#define CHIPSET WS2811

static const char *TAG = "main";

#define SAMPLE_RATE 16000
#define N 64
#define LOWPASS 150

#define I2S_PORT I2S_NUM_0

int16_t sBuffer[N];

CRGB leds[WS2812_NUM_LEDS];

float rms = 0;

Biquad bq;

inline float lerp(float start, float end, float t) {
  return (1 - t) * start + t * end;
}

void updateLeds(float amplitude) {
  for (int i = 0; i < WS2812_NUM_LEDS; i++)
    leds[i] = CRGB(amplitude, amplitude, amplitude);
  FastLED.show();
}

void setup_i2s() {
  const i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = i2s_bits_per_sample_t(16),
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = N,
      .use_apll = false};

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {.bck_io_num = I2S_SCK,
                                       .ws_io_num = I2S_WS,
                                       .data_out_num = -1,
                                       .data_in_num = I2S_SD};

  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_start(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  ESP_LOGI(TAG, "LOWPASS: %i", LOWPASS);
  ESP_LOGI(TAG, "N: %i", N);

  FastLED.addLeds<CHIPSET, WS2812_PIN, COLOR_ORDER>(leds, WS2812_NUM_LEDS)
      .setCorrection(TypicalLEDStrip);

  BiquadInit(&bq);
  BiquadLowpass(&bq, 0.707, LOWPASS, SAMPLE_RATE);

  delay(1000);
  pinMode(EN_PIN, OUTPUT);
  analogWrite(EN_PIN, 0);

  setup_i2s();

  delay(500);
  updateLeds(127);
}

void loop() {
  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, N * sizeof(int16_t), &bytesIn,
                              portMAX_DELAY);

  if (result != ESP_OK)
    return;

  int16_t samples_read = bytesIn / sizeof(int16_t);

  if (!samples_read)
    return;

  float energy_sum = 0.0f;

  for (int16_t i = 0; i < samples_read; ++i) {
    float s = static_cast<float>(sBuffer[i]);
    float temp = BiquadUpdate(&bq, s);
    energy_sum += temp * temp;
  }

  float new_rms = min(sqrt(energy_sum / samples_read), 255.f);

  rms = (new_rms > rms) ? new_rms : lerp(rms, new_rms, 0.1);

  Serial.printf("%f\n", rms);

  analogWrite(EN_PIN, static_cast<int>(rms));
}
