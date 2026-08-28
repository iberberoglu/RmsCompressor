# RMSCompressor

**A compressor plug-in with a user-adjustable RMS detection window and a user-adjustable envelope curve shape.**

`C++` · `JUCE 7.0.12` · `AU` · `VST3` · `macOS`

*[Türkçe README →](README.tr.md)*

![Plug-in interface](docs/ui.png)

---

## What it is

Most compressors give you attack and release **times**. This one also gives you
the **shape** of the curve those times produce, and lets you decide how long a
slice of audio the detector averages before it decides to act.

Both of those are normally fixed constants buried in the DSP code. Here they are
front-panel controls.

The plug-in was built from scratch in C++ with JUCE as an undergraduate project
at Istanbul Technical University (see [Project context](#project-context)). Two
JUCE DSP classes — `juce::dsp::Compressor` and `juce::dsp::BallisticsFilter` —
were patched to make the two ideas below possible; the patched modules ship in
this repository under `modules/`.

---

## The two ideas

### 1. Adjustable RMS detection window

JUCE's `AudioBuffer::getRMSLevel()` averages over whatever buffer the host hands
you — 128 samples, 512, 1024, whatever the DAW is set to. You don't control it,
and it changes when the user changes their buffer size.

To decouple the detector from the host, incoming samples are pushed into a
lock-free FIFO ring buffer (`Source/Fifo.h`). RMS is then computed over an
arbitrary slice pulled back out of that ring — set by the **RMS Period** slider,
1 to 1500 ms.

![FIFO ring buffer](docs/fifo-diagram.png)

The window length changes how much the detector "sees", and therefore how
aggressive the compressor feels. Measured in PluginDoctor, same source, same
threshold, same ratio:

| RMS Period = 100 ms | RMS Period = 1000 ms |
|---|---|
| ![100 ms](docs/measure-rms-window-100ms.png) | ![1000 ms](docs/measure-rms-window-1000ms.png) |

A short window tracks the signal closely, so gain reduction varies a lot and the
compression reads as aggressive. A long window averages over the material, gain
reduction stays steadier, and the effect is gentler and more level-riding.

### 2. Adjustable envelope curve shape

This is the part that doesn't exist in stock JUCE.

The ballistics filter is a one-pole smoother. Each sample it moves the envelope
toward the current level by a coefficient `cte`:

```cpp
result = level + cte * (yold - level);
cte    = std::exp (expFactor / timeMs);
```

In stock JUCE, `expFactor` is hardcoded:

```cpp
expFactor = -2.0 * pi * 1000.0 / sampleRate;   // the -2.0 is not adjustable
```

Here attack and release each get their own multiplier, exposed as a parameter:

```cpp
expFactorAttack  = attackCo  * pi * 1000.0 / sampleRate;   // -5.0 … -0.01
expFactorRelease = releaseCo * pi * 1000.0 / sampleRate;
```

The consequence: **two compressors set to the same attack time can hold the
signal completely differently.** Both measurements below are Attack = 300 ms,
RMS Period = 100 ms. Only the coefficient differs:

| Attack Coefficient = −0.1 | Attack Coefficient = −5.0 |
|---|---|
| ![coef -0.1](docs/measure-attack-coef-0.1.png) | ![coef -5.0](docs/measure-attack-coef-5.0.png) |

At −0.1 the envelope is still settling two seconds in — a long, slow lean into
the gain reduction. At −5.0 it snaps down and is essentially done within
300 ms. Same knob positions everywhere else.

The same control exists for release:

| Release Coefficient = −1.0 | Release Coefficient = −5.0 |
|---|---|
| ![coef -1.0](docs/measure-release-coef-1.0.png) | ![coef -5.0](docs/measure-release-coef-5.0.png) |

Coefficients closer to zero mean a slower, softer response; further from zero
means faster and tighter.

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
(`Source/GainReductionMeter.h`) drawn as an analogue needle gauge — the
metaphor is a car speedometer, where "faster" maps to "more gain reduction".

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

All parameters are automatable. The header also displays **Max RMS** (peak RMS
over roughly the last 4 seconds) and **Current RMS** per channel, refreshed
24 times a second.

When **Peak** is selected, RMS Period has no effect — the detector reads the
sample magnitude directly.

---

## Building

Requires macOS and Xcode. The project targets **JUCE 7.0.12**, and the patched
modules are vendored in `modules/` — you do not need a separate JUCE checkout to
build the DSP.

There is one wrinkle: `JuceLibraryCode/` was generated by a **JUCE 8** Projucer,
while `modules/` is JUCE 7. Two JUCE 8-only translation units have to be
excluded. This command is verified working (AU target, `BUILD SUCCEEDED`,
passes `auval`):

```bash
cd Builds/MacOSX
xcodebuild -project RMSCompressor.xcodeproj \
  -target "RMSCompressor - AU" -configuration Release \
  EXCLUDED_SOURCE_FILE_NAMES="include_juce_graphics_Harfbuzz.cpp include_juce_core_CompilationTime.cpp" \
  HEADER_SEARCH_PATHS="\$(SRCROOT)/../../JuceLibraryCode \$(SRCROOT)/../../modules \$(SRCROOT)/../../modules/juce_audio_plugin_client/AU \$(SRCROOT)/../../modules/juce_audio_processors/format_types/VST3_SDK" \
  build
```

Swap the target for `"RMSCompressor - VST3"` to build the VST3.

Xcode's plug-in copy step installs the built component to
`~/Library/Audio/Plug-Ins/Components/` on every Release build. Restart your DAW
to pick up the new version.

> Regenerating the project from a JUCE 7 Projucer, or porting the patches onto
> JUCE 8, would remove the need for the flags above. See
> [Known limitations](#known-limitations).

---

## Repository layout

| Path | What it is |
|---|---|
| `Source/PluginProcessor.*` | Parameters, RMS computation, signal routing |
| `Source/PluginEditor.*` | Interface, 24 Hz refresh timer |
| `Source/Fifo.h` | Lock-free ring buffer for the detection window |
| `Source/GainReductionMeter.h` | Custom analogue-style needle meter |
| `modules/juce_dsp/widgets/juce_Compressor.*` | **Patched** — RMS detection path, make-up gain, bypass |
| `modules/juce_dsp/processors/juce_BallisticsFilter.*` | **Patched** — exposed envelope coefficients |
| `docs/` | Screenshots and measurements used in this README |

### Branches

| Branch | Purpose |
|---|---|
| `au-release` | **Default.** The working line — patched modules, builds and passes `auval` |
| `main` | Archive. An abandoned experiment adding a third detection mode; its DSP half was lost, so it does not build |
| `arsiv-2024-nisan` | Archive. An early April 2024 snapshot, before the JUCE DSP modules were patched |

---

## Known limitations

Honest list. This was written while learning C++, and a re-read in August 2026
turned up things worth fixing. Checked items have been addressed; the rest are
open.

- [ ] **Per-sample heap allocation on the audio thread.** `processSample` takes
      `std::vector<float> rmsLevels` by value inside the sample loop — roughly
      44,100 allocations per second per channel. Passing by `const&` is the fix.
- [ ] **Ratio is initialised with its index, not its value.** `prepareToPlay`
      passes the choice index straight to `setRatio()`. Loading a session saved
      at ratio 1.0 calls `setRatio(0)`, which divides by zero.
- [ ] **Out-of-bounds read on mono.** The editor unconditionally reads channel 1;
      `isBusesLayoutSupported` accepts mono buses.
- [ ] **`prepareToPlay` does not push every parameter.** Attack, release,
      make-up gain and bypass only reach the DSP on the next parameter change,
      so the first blocks after a session load can run with stale values.
- [ ] **Data race on the gain reduction value.** A plain `float` written from the
      audio thread and read from the UI thread; the RMS buffer is also written
      from both, which breaks the FIFO's single-producer contract.
- [ ] **Ballistics filter has no fallback return.** If neither peak nor RMS is
      selected, `processSample` falls off the end of the function.
- [ ] **Gain reduction meter is not calibrated.** Scale marks are drawn at equal
      angular spacing while the values they represent are not linear, so the
      needle disagrees with the labels except at the extremes.
- [ ] **JUCE 7 / JUCE 8 mismatch** between `modules/` and `JuceLibraryCode/`,
      requiring the build flags above.
- [ ] Variable shadowing in `processBlock`; `setSize()` called from inside
      `resized()`.

---

## Project context

Built for **Serbest Proje Çalışması 2** (Independent Project Study 2) at
**Istanbul Technical University**, Department of Music Technology, spring 2024.
Submitted 7 June 2024. Advisor: **Dr. Ozan Sarıer**, whose suggestion the
adjustable-window idea originally came from.

I started the term with essentially no programming experience — some JavaScript,
no C++ at all. The compressor and the language were learned in parallel over one
semester.

The full project report (29 pages, in Turkish) covers the DSP background, the
development process, the problems hit along the way and the PluginDoctor
measurements. It is not in this repository; ask if you'd like a copy.

### Acknowledgements

**Akash Murthy** — after an email out of the blue, he gave up an hour of his time
for a Zoom call and explained how a FIFO ring buffer could decouple RMS
detection from the host buffer size. That conversation unblocked the central
problem of the project.

**Dr. Ozan Sarıer** — for the original idea and for supervising the work.

---

## License

This project embeds modified copies of JUCE 7 modules, used under JUCE's GPLv3
option (the JUCE splash screen is enabled). Any redistribution therefore falls
under **GPLv3**.
