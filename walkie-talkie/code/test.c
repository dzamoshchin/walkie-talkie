#include "stdio.h"
#include "wav.h"

#define SECS 1
#define SAMPLE_RATE 44100
#define N (SAMPLE_RATE * SECS)

int main() {
  size_t num_bytes = sizeof(wav_header_t) + (N * sizeof(int32_t));

  INIT_WAV_HEADER(w);
  w.overall_size = num_bytes - 8;
  w.data_size = (N * w.channels * w.bits_per_sample) / 8;

  char *iter = (char *)&w;
  for (int i = 0; i < 44; i++) {
    printf("%X\n", *(iter + i));
  }
  return 1;
}
