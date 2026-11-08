# Nuked-OPM
Cycle accurate YM2151(OPM) and YM2164(OPP) emulator

## Pitch conversion cache

Nuked-OPM caches pitch conversion per operator slot using key/fraction,
effective LFO, PMS and DT2 as the cache key. Unchanged inputs reuse the existing
frequency and key-code outputs. Reset invalidates the keys, and YM2164 pipeline
updates still run on every clock. The cache adds 128 bytes to `opm_t`, so users
must rebuild code that embeds that structure.

Run the pitch-cache regression for both YM2151 and YM2164 with:

```sh
cc -std=c99 -O2 -fsanitize=undefined -fno-sanitize-recover=undefined \
  tests/pitch_cache.c -o /tmp/opm-pitch-cache
/tmp/opm-pitch-cache
```

## Phase increment cache

Phase increments are cached per slot using frequency, key code, DT1 and
multiplier. Cache hits restore the pipeline increment because phase reset can
clear it independently. Phase accumulation and reset timing still run on every
clock. This adds another 256 bytes to `opm_t`; rebuild code embedding it.

Run the arithmetic and per-clock state regression for both chip variants with:

```sh
cc -std=c99 -O2 -fsanitize=undefined -fno-sanitize-recover=undefined \
  tests/phase_increment_cache.c -o /tmp/opm-phase-increment-cache
/tmp/opm-phase-increment-cache
```
