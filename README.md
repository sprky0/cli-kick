# clikick

A minimal command-line utility in C that generates a 909-style kick drum (APPROXIMATELY JEEEEEZ) as a 24-bit, 48 kHz WAV and writes it either to stdout or a file.

## Features

- Simple **envelope** (attack/decay) for amplitude.
- **Pitch sweep** to mimic the characteristic “thwack” then drop to a lower frequency.
- A small **click transient** at the start.
- Adjustable parameters via command-line arguments.

## Building

1. Make sure you have a C compiler (e.g. `gcc`) and `make`.
2. Run `make` in this directory:
   ```bash
   make

This will produce an executable named clikick.

## Usage


```bash
  ./clikick <tune> <attack> <decay> <level> [-o <output.wav>]

```

Where each parameter is in the range 0.0 to 1.0.

### Parameters

- tune: Affects the fundamental pitch (lowest frequency) and initial pitch sweep.
- attack: Controls how quickly the amplitude ramps from 0 to max at the start.
    - note that attack is sort of incorrect here relative to the 909 circuit but don't worry about it
- decay: Controls how quickly the sound fades out after the attack.
- level: Overall output volume multiplier.

## Examples

### Write a WAV file to disk:

```bash
./clikick 0.5 0.5 0.5 1.0 -o mykick.wav

```

Generates a 1-second kick with mid-range settings and writes mykick.wav.


### Pipe to stdout:

```bash
./clikick 0.2 0.2 0.8 1.0 > output.wav

```

Captures stdout into output.wav.

### Play directly using another tool:

#### On Linux with aplay:

```bash
./clikick 0.5 0.5 0.5 1.0 | aplay

```

#### With SoX play:

```bash
./clikick 0.5 0.5 0.5 1.0 | play -t wav -

```

#### With FFmpeg’s ffplay:

```bash
./clikick 0.5 0.5 0.5 1.0 | ffplay -autoexit -nodisp -

```

