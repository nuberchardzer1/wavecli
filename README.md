# wavecli

Simple terminal tool for DSP experiments

I'm really interested in DSP (digital signal processing).
I wanted to create something small, without heavy frameworks or complex APIs — just a tool for quick experiments with PCM audio and its analysis.
So I built a simple TUI app where you can register your own sound effects and play them back using PortAudio or save to file.

### TUI Commands

| Command | Alias | Description                          | Example                  |
|---------|-------|--------------------------------------|--------------------------|
| gain    | g     | set gain multiplier                  | gain 1.5                 |
| effect  | e     | select & apply effect                | effect                   |
| record  | r     | start recording to file              | record myfile.wav        |
| input   | di    | select input device                  | input                    |
| output  | do    | select output device                 | output                   |
| help    | h     | show this help                       | help                     |

### How to Add Your Own Effect
1. Open `effect.с`
2. Write your per-sample processing function:

```c
float my_effect(float sample, void *user_data) {
    return sample * 1.5f;
}
```
3. Register it to ```const effect_t effects[]```
### Usage
```bash
cc -o wavecli *.c $(pkg-config --cflags --libs portaudio-2.0)
./wavecli --help
./wavecli --rate 48000 --channels 2 --gain 1.5