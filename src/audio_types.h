#pragma once

#define SAMPLE_RATE  (44100)
#define FRAMES_PER_BUFFER (10)

typedef float SAMPLE; // float [-1...1] 

#define SAMPLE_MAX_VALUE  1.0f
#define SAMPLE_MIN_VALUE -1.0f

typedef struct audio_params_t{
    int volume;
    int channels;
    float gain;      
} audio_params_t;

typedef void (*audio_process_fn)(SAMPLE *samples,
                                 unsigned long frameCount,
                                 const audio_params_t *p);