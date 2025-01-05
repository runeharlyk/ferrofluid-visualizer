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

#define I2S_PORT I2S_NUM_0

#define SAMPLE_RATE 1280 // 44100
#define N (SAMPLE_RATE / 10)
#define RESOLUTION (SAMPLE_RATE / N)

#define LOWPASS 150
#define BIN_RANGE (LOWPASS * N / SAMPLE_RATE + 1)

CRGB leds[WS2812_NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType currentBlending;
int16_t sBuffer[N];

float fft_table[2 * N];
float fft_data[2 * N];
float fft_result[N / 2];
__attribute__((aligned(16))) float wind[N];

float mean = 0;
float range_amplitude = 0.0f;

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
  ESP_LOGI(TAG, "BIN_RANGE: %i", BIN_RANGE);
  ESP_LOGI(TAG, "RESOLUTION: %i", RESOLUTION);

  FastLED.addLeds<CHIPSET, WS2812_PIN, COLOR_ORDER>(leds, WS2812_NUM_LEDS)
      .setCorrection(TypicalLEDStrip);

  BiquadInit(&bq);
  BiquadLowpass(&bq, 0.707, LOWPASS, SAMPLE_RATE);

  delay(1000);
  pinMode(EN_PIN, OUTPUT);
  analogWrite(EN_PIN, 0);

  esp_err_t ret;
  dsps_wind_hann_f32(wind, N);
  ret = dsps_fft2r_init_fc32(NULL, N);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
    return;
  }

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

  float newMean = 0;
  float sum = 0.0f;

  for (int16_t i = 0; i < samples_read; ++i) {
    float s = static_cast<float>(sBuffer[i]);
    fft_data[2 * i] = s * wind[i]; // Real part
    fft_data[2 * i + 1] = 0;       // Imaginary part
    float temp = BiquadUpdate(&bq, s);
    sum += temp * temp;
    newMean += temp;
  }

  dsps_fft2r_fc32(fft_data, N);
  dsps_bit_rev_fc32(fft_data, N);

  for (int i = 0; i < N / 2; i++) {
    float real = fft_data[2 * i];
    float imag = fft_data[2 * i + 1];
    fft_result[i] = sqrt(real * real + imag * imag) / 32768.f * 255.f;
    // Serial.printf("255, 0, %f, ", fft_result[i] / 32768.f * 255.f);
  }

  float new_range_amplitude = 0.0f;

  for (int i = 1; i < N / 2; i++) {
    new_range_amplitude = max(new_range_amplitude, fft_result[i]);
  }

  new_range_amplitude = min(new_range_amplitude * 3, 255.f);

  range_amplitude = (new_range_amplitude > range_amplitude)
                        ? new_range_amplitude
                        : lerp(range_amplitude, new_range_amplitude, 0.1);

  Serial.println(new_range_amplitude);

  sum = min(sqrt(sum / samples_read), 255.f);

  newMean /= samples_read;

  newMean = min(abs(newMean), 255.f);

  mean = (newMean > mean) ? newMean : lerp(mean, newMean, 0.1);

  // Serial.printf("%f, %f, %f\n", sum, mean, newMean);

  analogWrite(EN_PIN, static_cast<int>(range_amplitude));
}
