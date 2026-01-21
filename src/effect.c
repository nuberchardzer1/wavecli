#include <stdio.h>
#include "audio_types.h"
#include "effect.h"

static float SOFTCLIP_BORDER = 2.0f/3.0f;
void soft_clip(SAMPLE *samples, unsigned long frameCount, const audio_params_t *p){
    for(int i = 0; i < frameCount; i++){
        SAMPLE s = samples[i] * p->gain;
        if (s > SAMPLE_MAX_VALUE)
            s = SOFTCLIP_BORDER;
        else if (s < SAMPLE_MIN_VALUE)
            s = -SOFTCLIP_BORDER;
        else
            s = s - (s*s*s/3);
        samples[i] = s;
    }
}

void hard_clip(SAMPLE *samples, unsigned long frameCount, const audio_params_t *p){
    for(int i = 0; i < frameCount; i++){
        SAMPLE s = samples[i] * p->gain;
        if (s > SAMPLE_MAX_VALUE)
            s = SAMPLE_MAX_VALUE;
        else if (s < SAMPLE_MIN_VALUE)
            s = SAMPLE_MIN_VALUE;
        samples[i] = s;
    }
}

void invert(SAMPLE *samples, unsigned long frameCount, const audio_params_t *p){
    (void)p;
    for (unsigned long i = 0; i < frameCount; ++i)
        samples[i] = -samples[i];
}

void no_change(SAMPLE *samples, unsigned long frameCount, const audio_params_t *p){
    (void)samples;
    (void)frameCount;
    (void)p;
}

static SAMPLE state_register = 0;
void feed_forward_filter(SAMPLE *samples, unsigned long frameCount, const audio_params_t *p){
    const float a0 = 0.5f;
    const float b0 = 0.5f;
    for(int i = 0; i < frameCount; i++){
        SAMPLE new_s = a0 * samples[i] + b0 * state_register;
        state_register = samples[i];
        samples[i] = new_s;
    }
}

const effect_t effects[] = {
    {"none", "No processing", no_change },
    {"soft", "Soft clipping", soft_clip },
    {"hard", "Hard clipping", hard_clip },
    {"inversion", "inverted samples", invert },
    {"feed forward", "-", feed_forward_filter}
};

const size_t effects_count = sizeof(effects) / sizeof(effects[0]);

// const effect_t *effect_by_id(effect_id id) {
//     for (size_t i = 0; i < effects_count; i++){
//         if (effects[i].id == id) 
//             return &effects[i];
//     }
//     return NULL;
// }

// audio_process_fn effect_func_by_id(effect_id id){
//     const effect_t *e = effect_by_id(id);
//     return e ? e->func : NULL;
// }