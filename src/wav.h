#ifndef WAV_H
#define WAV_H

#include <stdio.h>
#include <stdint.h>

#define WAV_HEADER_SIZE (44)

#define WAVE_FORMAT_PCM       1
#define WAVE_FORMAT_IEEE_FLOAT  3
#define WAVE_FORMAT_ALAW 6
#define WAVE_FORMAT_MULAW 7
#define WAVE_FORMAT_EXTENSIBLE (0xFFFE)

typedef struct wav_writer{
    FILE *file;
    size_t num_samples;
    int num_channels;
    int bits_per_sample;
    int sample_rate;
}wav_writer;

wav_writer *wav_open(const char *path, int audio_format, 
        int sample_rate, int channels, int bits_per_sample);
size_t wav_write(wav_writer *w, const void *data, size_t bytes);
int wav_close(wav_writer *w);

#endif