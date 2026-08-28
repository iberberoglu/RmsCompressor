# RMSCompressor

**A compressor plug-in where you set the RMS detection window and the shape of the envelope curve yourself.**

`C++` · `JUCE 7.0.12` · `AU` · `VST3` · `macOS`

*[Türkçe README →](README.tr.md)*

![Plug-in interface](docs/ui.png)

---

## What it is

Most compressors give you attack and release **times**. I wanted one that also
gives you the **shape** of the curve those times produce, and lets you decide how
long a slice of audio the detector averages before it acts.

Both of those are normally fixed constants buried in the DSP code. Here I pulled
them out onto the front panel.

I built this from scratch in C++ and JUCE as an undergraduate project at Istanbul
Technical University (see [Project context](#project-context)). To make the two
ideas below work I patched two JUCE DSP classes — `juce::dsp::Compressor` and
`juce::dsp::BallisticsFilter` — and the patched modules ship in this repository
under `modules/`.

---

## The two ideas

### 1. An RMS detection window you can set

JUCE's `AudioBuffer::getRMSLevel()` averages over whatever buffer the host hands
you — 128 samples, 512, 1024, whatever the DAW is set to. You don't control it,
and it changes the moment the user changes their buffer size.

To cut the detector loose from the host, I push incoming samples into a lock-free
FIFO ring buffer (`Source/Fifo.h`) and compute RMS over an arbitrary slice pulled
back out of that ring. The **RMS Period** slider sets how long that slice is,
from 1 to 1500 ms.

![FIFO ring buffer](docs/fifo-diagram.png)

The window length changes how much the detector "sees", and with it how
aggressive the compressor feels. I measured this in PluginDoctor — same source,
same threshold, same ratio:

| RMS Period = 100 ms | RMS Period = 1000 ms |
|---|---|
| ![100 ms](docs/measure-rms-window-100ms.png) | ![1000 ms](docs/measure-rms-window-1000ms.png) |

A short window tracks the signal closely, so gain reduction swings a lot and the
compression reads as aggressive. A long window averages over the material, gain
reduction stays steadier, and the effect turns gentler and more level-riding.

I capped the slider at 1500 ms because past that point, in my own testing, longer
windows stopped making an audible difference to the dynamics.

**Enable smoothing** deals with a side effect of this design. I recompute RMS
once per block, so the detector sees a staircase of discrete values instead of a
continuous one, and large steps between them make the envelope harder than I
wanted. Turning smoothing on interpolates between successive RMS values with
`juce::LinearSmoothedValue`. The difference shows up most clearly in the release
tail after the signal drops at 3 s — it snaps back in well under a second without
smoothing, and eases out across the full remaining second with it:

| Smoothing off | Smoothing on |
|---|---|
| ![off](docs/measure-rms-window-100ms.png) | ![on](docs/measure-smoothing-on.png) |

### 2. An envelope curve whose shape you can change

This is the part that doesn't exist in stock JUCE, and it's my favourite thing in
the project.

The ballistics filter is a one-pole smoother. On every sample it moves the
envelope toward the current level by a coefficient `cte`:

```cpp
result = level + cte * (yold - level);
cte    = std::exp (expFactor / timeMs);
```

In stock JUCE, `expFactor` is hardcoded:

```cpp
expFactor = -2.0 * pi * 1000.0 / sampleRate;   // you don't get to touch the -2.0
```

I gave attack and release their own multiplier and exposed each as a parameter:

```cpp
expFactorAttack  = attackCo  * pi * 1000.0 / sampleRate;   // -5.0 … -0.01
expFactorRelease = releaseCo * pi * 1000.0 / sampleRate;
```

What this buys you: **two compressors set to the same attack time can hold the
signal completely differently.** In both measurements below I set Attack to
300 ms and RMS Period to 100 ms. Only the coefficient changes:

| Attack Coefficient = −0.1 | Attack Coefficient = −5.0 |
|---|---|
| ![coef -0.1](docs/measure-attack-coef-0.1.png) | ![coef -5.0](docs/measure-attack-coef-5.0.png) |

At −0.1 the envelope is still settling two seconds in — a long, slow lean into
the gain reduction. At −5.0 it snaps down and is essentially finished within
300 ms. Every other knob is in the same position.

Release gets the same control:

| Release Coefficient = −1.0 | Release Coefficient = −5.0 |
|---|---|
| ![coef -1.0](docs/measure-release-coef-1.0.png) | ![coef -5.0](docs/measure-release-coef-5.0.png) |

Coefficients closer to zero give a slower, softer response; further from zero,
faster and tighter.

I found this by accident, while experimenting with the coefficients to understand
how JUCE calculated attack and release in the first place. Once I heard what
changing them did, handing the control to the user was the obvious move.

---

## Signal flow

```
DAW buffer
   │
   ├──▶ Fifo.h ──────────▶ pull N samples (N = RMS Period × sample rate)
   │    (lock-free ring)        │
   │                            ▼
   │                    getRMSLevel() per channel
   │                            │
   │                    optional LinearSmoothedValue  ← "Enable smoothing"
   │                            │
   │                            ▼
   └──────────────────▶ juce::dsp::Compressor (patched)
                                │
                        BallisticsFilter (patched)
                        ├─ RMS mode  → envelope follows the windowed RMS
                        └─ Peak mode → envelope follows |sample|
                                │
                        attack/release coefficients shape the curve
                                │
                        envelope vs. threshold → gain reduction
                                │
                        × make-up gain ──────▶ output
                                │
                        gain reduction (dB) ──▶ UI meter, 24 Hz timer
```

The gain reduction meter is a custom `juce::Component`
(`Source/GainReductionMeter.h`). I started out drawing a conventional VU meter,
then realised those gauges aren't far from a car's speedometer — so I drew one,
and mapped "faster" onto "more gain reduction".

---

## Parameters

| Control | Range | Default |
|---|---|---|
| Threshold | −60 … 0 dB | −20 dB |
| Ratio | 1, 2, 3 … 10, 15, 20, 50, 100 | 5.0 |
| Attack | 0.5 … 300 ms | 15 ms |
| Release | 5 … 1000 ms | 50 ms |
| **Attack Coefficient** | −5.0 … −0.01 | −0.1 |
| **Release Coefficient** | −5.0 … −0.01 | −0.4 |
| Make Up | −20 … +20 dB | 0 dB |
| **RMS Period** | 1 … 1500 ms | 50 ms |
| Detection | RMS / Peak | RMS |
| Enable Smoothing | on / off | off |
| Bypass | on / off | off |

Every parameter is automatable. The header also shows **Max RMS** (the highest
RMS over roughly the last 4 seconds) and **Current RMS** per channel, refreshed
24 times a second.

Make-up gain isn't in JUCE's compressor class, so I wrote it and added it to the
output stage.

When you pick **Peak**, RMS Period stops doing anything — the detector reads the
sample magnitude directly.

I tested the plug-in in Logic Pro, Ableton Live and Reaper.

---

## Building

You need macOS and Xcode. The project targets **JUCE 7.0.12** and the patched
modules are vendored in `modules/`, so you don't need a separate JUCE checkout to
build the DSP.

There is one wrinkle to know about. `JuceLibraryCode/` in this repo came out of a
**JUCE 8** Projucer while `modules/` is JUCE 7, so two JUCE 8-only translation
units have to be excluded. This command works — AU target, `BUILD SUCCEEDED`,
passes `auval`:

```bash
cd Builds/MacOSX
xcodebuild -project RMSCompressor.xcodeproj \
  -target "RMSCompressor - AU" -configuration Release \
  EXCLUDED_SOURCE_FILE_NAMES="include_juce_graphics_Harfbuzz.cpp include_juce_core_CompilationTime.cpp" \
  HEADER_SEARCH_PATHS="\$(SRCROOT)/../../JuceLibraryCode \$(SRCROOT)/../../modules \$(SRCROOT)/../../modules/juce_audio_plugin_client/AU \$(SRCROOT)/../../modules/juce_audio_processors/format_types/VST3_SDK" \
  build
```

Swap the target for `"RMSCompressor - VST3"` to build the VST3.

Xcode's plug-in copy step installs the built component into
`~/Library/Audio/Plug-Ins/Components/` on every Release build, so restart your
DAW to pick up the new version.

> Regenerating the project from a JUCE 7 Projucer, or moving my patches onto
> JUCE 8, would drop the need for those flags. It's on the list below.

---

## Repository layout

| Path | What it is |
|---|---|
| `Source/PluginProcessor.*` | Parameters, RMS computation, signal routing |
| `Source/PluginEditor.*` | Interface, 24 Hz refresh timer |
| `Source/Fifo.h` | Lock-free ring buffer for the detection window |
| `Source/GainReductionMeter.h` | Custom analogue-style needle meter |
| `modules/juce_dsp/widgets/juce_Compressor.*` | **Patched** — I added the RMS detection path, make-up gain and bypass |
| `modules/juce_dsp/processors/juce_BallisticsFilter.*` | **Patched** — I exposed the envelope coefficients |
| `docs/` | Screenshots and measurements used in this README |

### Branches

| Branch | Purpose |
|---|---|
| `au-release` | **Default.** The working line — patched modules, builds and passes `auval` |
| `main` | Archive. An experiment I abandoned that added a third detection mode; I lost its DSP half, so it doesn't build |
| `arsiv-2024-nisan` | Archive. An early April 2024 snapshot, from before I patched the JUCE DSP modules |

---

## Known limitations

An honest list. I wrote this code while learning C++, and when I came back to it
in August 2026 I went through it properly and found things worth fixing. I tick
items off here as I fix them.

- [ ] **Per-sample heap allocation on the audio thread.** `processSample` takes
      `std::vector<float> rmsLevels` by value from inside the sample loop —
      roughly 44,100 allocations a second per channel. Passing by `const&` fixes
      it.
- [ ] **Ratio is initialised with its index instead of its value.**
      `prepareToPlay` hands the choice index straight to `setRatio()`. Load a
      session saved at ratio 1.0 and it calls `setRatio(0)`, which divides by
      zero.
- [ ] **Out-of-bounds read on mono.** The editor reads channel 1 unconditionally,
      but `isBusesLayoutSupported` accepts mono buses.
- [ ] **`prepareToPlay` doesn't push every parameter.** Attack, release, make-up
      gain and bypass only reach the DSP on the next parameter change, so the
      first blocks after loading a session can run with stale values.
- [ ] **Data race on the gain reduction value.** It's a plain `float` written from
      the audio thread and read from the UI thread, and the RMS buffer gets
      written from both too, which breaks the FIFO's single-producer contract.
- [ ] **No fallback return in the ballistics filter.** With neither peak nor RMS
      selected, `processSample` runs off the end of the function.
- [ ] **The gain reduction meter isn't calibrated.** I draw the scale marks at
      equal angular spacing, but the values they stand for aren't linear, so the
      needle disagrees with the labels everywhere except the extremes.
- [ ] **JUCE 7 / JUCE 8 mismatch** between `modules/` and `JuceLibraryCode/`,
      which is why the build flags above exist.
- [ ] Variable shadowing in `processBlock`, and `setSize()` called from inside
      `resized()`.

One thing I never fully solved: with a very short RMS window and a very short
attack, the attack curve stops being smooth. It bothered me for a long time
before I decided the artefact is usable as an effect in its own right, and left
it in.

---

## Project context

I built this for **Serbest Proje Çalışması 2** (Independent Project Study 2) at
**Istanbul Technical University**, Department of Music Technology, in the spring
of 2024, and submitted it on 7 June 2024. My advisor was **Dr. Ozan Sarıer**, and
the adjustable-window idea came out of his suggestion.

I started that term with essentially no programming experience — a little
JavaScript, no C++ at all. I learned the language and built the compressor in
parallel over a single semester.

I also wrote a full project report (29 pages, in Turkish) covering the DSP
background, the development process, the problems I ran into and the PluginDoctor
measurements. It isn't in this repository — ask me if you'd like a copy.

### Acknowledgements

**Akash Murthy** — I emailed him out of the blue, and he gave up an hour of his
time for a Zoom call and explained how a FIFO ring buffer could decouple RMS
detection from the host buffer size. That conversation unblocked the central
problem of the project. It also hadn't occurred to me that I could just talk to
an audio engineer on the other side of the world, which turned out to be worth as
much as the technical answer.

**Dr. Ozan Sarıer** — for the original idea and for supervising the work.

---

## License

This repository includes modified copies of JUCE 7 modules, which I use under
JUCE's GPLv3 option (the JUCE splash screen is enabled). Any redistribution
therefore falls under **GPLv3**.
