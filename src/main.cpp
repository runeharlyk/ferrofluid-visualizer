#include <Arduino.h>
#include <driver/i2s.h>
#include <dsps_fft2r.h>
// #include <dsp_math.h>
#include "esp_dsp.h"

static const char *TAG = "main";

#define I2S_PORT I2S_NUM_0

#define bufferLen 64
#define SAMPLES 1024
#define SAMPLE_RATE 16000 // 44100

int16_t sBuffer[bufferLen];

float mean = 0;

inline float lerp(float start, float end, float t) {
  return (1 - t) * start + t * end;
}

void setup() {
  Serial.begin(115200);

  delay(1000);
  pinMode(EN_PIN, OUTPUT);
  analogWrite(EN_PIN, 0);

  esp_err_t ret;
  ESP_LOGI(TAG, "Start Example.");
  ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
    return;
  }

  const i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = i2s_bits_per_sample_t(16),
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = bufferLen,
      .use_apll = false};

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {.bck_io_num = I2S_SCK,
                                       .ws_io_num = I2S_WS,
                                       .data_out_num = -1,
                                       .data_in_num = I2S_SD};

  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_start(I2S_PORT);

  delay(500);
}

void loop() {
  size_t bytesIn = 0;
  esp_err_t result =
      i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);

  if (result == ESP_OK) {
    int16_t samples_read = bytesIn / 8;
    if (samples_read > 0) {
      float newMean = 0;
      for (int16_t i = 0; i < samples_read; ++i) {
        newMean += (sBuffer[i]);
      }

      newMean /= samples_read;

      newMean = min(abs(newMean / 5), 255.f);

      if (newMean > mean) {
        mean = newMean;
      } else {
        mean = lerp(mean, newMean, 0.1);
      }

      analogWrite(EN_PIN, mean);

      Serial.printf("%f\n", mean);
    }
  } else {
    Serial.println("Error reading I2S data");
  }
}