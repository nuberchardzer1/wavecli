#pragma once

#include "audio_types.h"

typedef struct effect_t{
    char *name;
    char *description;
    audio_process_fn func;
} effect_t;

extern const effect_t effects[];
extern const size_t effects_count;