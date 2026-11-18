# Silktone Ambient Suite

A pair of macOS audio plugins for Logic Pro (Audio Unit), Pro Tools / other
DAWs (VST3), and standalone use. Lexicon / Valhalla-style lush ambient
spaces, tape echo character, modulation, stereo delay, EQ, saturation and
compression — built to sound great on vocals as well as guitar and keys,
and to stay lean on CPU.

## The two plugins

| Plugin | What it is |
|---|---|
| **Silktone Ambient** | The centerpiece. Ambient multi-effects: saturation → 3-band EQ → compressor → chorus → stereo delay → tape echo → FDN reverb → dry/wet blend. |
| **Silktone Vocal Channel** | A vocal-first channel strip: high-pass → 3-band EQ → opto compressor → saturation → dry/wet blend. For a vocal insert or a clean-up pass before the Ambient unit. |

Both share one DSP codebase and one UI language: a dark Tron-style
console, near-black panels, cyan and orange neon hairlines, thin lines,
organized sections, easy on the eyes.

## The reverb — hall, plate, room, chamber

The reverb is an 8-line modulated feedback-delay network (Lexicon
224/480L + Valhalla style: Hadamard feedback matrix, Spin/Wander
modulation, diffusion allpasses, octave-up shimmer in the feedback loop).
It has **four reverb types**, selectable per space:

- **Hall** — long open lines, the classic lush space
- **Plate** — short, dense, bright (vintage EMT 140 flavour)
- **Room** — tight short lines, less diffuse, drier and closer
- **Chamber** — dense and smooth, the concrete echo-chamber sound

**Dual Space** puts a different reverb on each side — Space A on the
left, Space B on the right — each with its own type, EQ and level, and a
blend control that crossfeeds them. The "Plate & Hall" preset is the
demo: vintage plate left, hall right.

## Presets — real rooms, researched, not guessed

The Ambient presets are stylistic spaces: famous studios and halls,
built from their actual acoustic parameters (reverb/decay times, room
character, early reflections). The decay knob maps to RT60 as
0.3 × 40^(decay/100) seconds, so each preset's decay is set from the
room's measured or documented reverb time. Each preset also notes what
was recorded in the room — that line shows under the preset bar.

**West Coast Verbs**
- **Westlake Room A** — 1,200 sq ft, Tom Hidley's flat controlled
  design, low reverb, bass traps. RT60 ~0.75 s. Michael Jackson's
  *Thriller* was cut in this room.
- **Sunset Sound** — Studio 1, a converted car workshop with a sloping
  floor; "not too wet and splashy but still a nice reverb time and
  ambience... a natural '70s kind of tone" (Joe Chiccarelli). RT60
  ~0.9 s. The Doors, Prince and Van Halen tracked here.
- **Capitol Echo Chambers** — eight subterranean trapezoidal concrete
  chambers, 30 ft under the Capitol Tower, shellacked; Les Paul
  designed chambers 1–4 (1956). Average delay ~3 s, maximum 5 s, smooth
  decay. RT60 ~3 s. Sinatra, Nat King Cole and the Beach Boys sang
  through these chambers.
- **EastWest Studio One** — Bill Putnam's huge orchestral room (Western
  Recorders, 1958): "rich and musical without being overwhelming."
  RT60 ~1.6 s. *Pet Sounds* and Sinatra's classic sessions were cut
  here.

**Soul**
- **Stax** — Memphis Studio A, a converted movie theater with a sloped
  floor and no parallel surfaces: "big, deep yet raw sound," "famously
  short, dry reverb." RT60 ~0.7 s. Otis Redding, Booker T. & the MGs,
  Isaac Hayes.
- **Motown — Hitsville** — the "Snakepit": a tiny basement with a low
  ceiling, tight and punchy, plus the shellac-coated attic echo chamber
  and an EMT 140 plate. RT60 ~0.6 s. The Supremes, Stevie Wonder,
  Marvin Gaye, the Temptations.

**Classic Rooms**
- **Abbey Road Studio Two** — the Beatles room: 60 × 38 × 24 ft, thick
  wood floor, RT60 1.2 s (Native Instruments manual) to 1.6 s (Abbey
  Road tour); set to 1.4 s.
- **Electric Lady Studio A** — Jimi Hendrix's room, designed by John
  Storyk (opened August 1970, with Eddie Kramer): curved walls and a
  thin plaster ceiling that accidentally became a giant low-frequency
  membrane absorber, so "reverb time actually goes down as you go down
  in frequency... a very tight rock sound." RT60 ~0.8 s.

**Great Halls**
- **AIR Lyndhurst Hall** — 1884 gothic church, hexagonal hall, George
  Martin's studio: natural reverb 7.5 s, down to about 3 s via the
  motorized canopy. Hans Zimmer's home. RT60 7.5 s.
- **Hansa Meistersaal** — Berlin, 1910 chamber hall, 266 sq m, 7-metre
  wooden coffered ceiling. Bowie's "Heroes" room, Tony Visconti's
  three-microphone gated technique. RT60 ~1.8 s. Bowie's "Heroes" was
  cut live in this room.
- **Columbia 30th Street** — "The Church," New York: converted church
  with 100-foot ceilings. *Kind of Blue*, *Time Out*, *Highway 61*.
  Warm natural reverb. RT60 ~3 s.

**Classics**
- **Vintage Plate** — the EMT 140: dense, bright, the classic plate.
  RT60 ~1.8 s.
- **Plate & Hall** — dual space: vintage plate on the left, hall on
  the right, each EQ'd differently, lightly blended.

The Vocal Channel has 10 vocal-chain presets (Airy Vocal, Warm & Close,
Radio Voice, Gentle Glue, Bright Pop, Clean & Dry, Studio Sheen, Lo-Fi
Warmth, Spoken Word, Choir Glue).

The preset menu is Waves / Native Instruments style: a preset name with
step arrows on either side, click the name for the full menu grouped
under section headers. **RANDOMIZE** gives a new sound in one tap.
The plugin opens on Westlake Room A.

**Dry/wet:** the BLEND knob (labelled DRY/WET) is a full dry/wet
control on each plugin — 0% is the dry signal untouched, 100% is the
fully processed signal. Ambient defaults to 50%, Vocal Channel to 100%.

## Build on a Mac (Xcode)

This project was written and tested on Linux; the Audio Unit can only be
built on a Mac. You need Xcode (with the Command Line Tools) and CMake.

```sh
# 1. Open Terminal and go to the project folder
cd ~/workspace/plugins/silktone-ambient-suite

# 2. Configure (downloads JUCE 8.0.8 automatically — needs internet once)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build --config Release -j$(sysctl -n hw.ncpu)
```

That builds both plugins in all three formats. Because
`COPY_PLUGIN_AFTER_BUILD` is on, the build copies the bundles to the
standard locations for you:

| Format | Where it lands |
|---|---|
| Audio Unit (.component) | `~/Library/Audio/Plug-Ins/Components/Silktone Ambient.component` and `Silktone Vocal Channel.component` |
| VST3 (.vst3) | `~/Library/Audio/Plug-Ins/VST3/Silktone Ambient.vst3` and `Silktone Vocal Channel.vst3` |
| Standalone app | `build/SilktoneAmbient_artefacts/Release/Standalone/` and `build/SilktoneVocalChannel_artefacts/Release/Standalone/` — drag to Applications if you want it there |

Then quit Logic and reopen it; the plugins appear under the
manufacturer **Silktone** (Audio Units > Silktone > Silktone Ambient /
Silktone Vocal Channel). If Logic doesn't see them, run AUVal once:

```sh
auval -v aufx SAmb SgEn
auval -v aufx SVch SgEn
```

If you'd rather place the bundles by hand, they are in
`build/SilktoneAmbient_artefacts/Release/` and
`build/SilktoneVocalChannel_artefacts/Release/` (AU, VST3 and Standalone
subfolders).

## Pro Tools (AAX)

There is no AAX build for this suite, and adding the format to the
build would not be enough on its own. An unsigned `.aaxplugin` will
not load in retail Pro Tools at all — it is silently absent from the
insert menu. Loading it in a normal Pro Tools needs PACE code signing,
which is a human approval process, not a build step:

1. A free Avid developer account (developer.avid.com) and the AAX SDK
   licence, plus an iLok account.
2. Pro Tools Developer (free) to test unsigned builds.
3. Email Avid (`audiosdk@avid.com`, subject "PACE Eden Signing Tools
   Request") for the PACE EDEN signing tools, sign PACE's NDA, and get
   a signing certificate on an iLok USB key (or PACE Cloud Signing for
   CI). Only a PACE-signed bundle loads in retail Pro Tools.

Until that process is done, the suite runs in Logic (AU), any VST3
host, and as a Standalone app. The same applies to the drum machine
plugin — see its `docs/AAX.md` for the full write-up.

## Use

- **In Logic:** insert Silktone Ambient on a bus or a track. Start from
  a preset (Westlake Room A is the default), then turn the knobs.
  The tape echo and the stereo delay both have a Tempo Sync option so
  they follow the song tempo.
- **On vocals:** put Silktone Vocal Channel first (high-pass the
  rumble, gentle compression, a little air), then Silktone Ambient
  after it for the space. Or use the Ambient unit alone — it has the
  same EQ and compressor built in.
- **Randomize:** one tap gives a new sound. If you like it, save it as
  a Logic preset from the plugin header.

## Demo audio

`demo/silktone-ambient-demo.wav` — 8 seconds, 0–4 s dry, 4–8 s through
the AIR Lyndhurst Hall preset (a guitar-like plucked phrase).
`demo/silktone-vocal-demo.wav` — same shape through the Airy Vocal
preset. Both were rendered by the DSP test binary on Linux.

## Files

```
CMakeLists.txt            JUCE 8.0.8 (FetchContent), AU + VST3 + Standalone
Shared/
  DSPBlocks.h/.cpp        one-pole filters, RBJ biquad, 3-band EQ,
                          saturation, opto compressor, delay line,
                          chorus, stereo delay, pitch shifter
  FDNReverb.h/.cpp        8-line modulated FDN reverb, dual-space mode,
                          shimmer, hall / plate / room / chamber types
  TapeEchoDSP.h/.cpp      tape echo, reused from the Silktone Tape Echo
                          project (EP-3 style: fractional delay, wow &
                          flutter, tape saturation, Sound on Sound)
  AmbientDSP.h/.cpp       the Ambient signal chain
  VocalChannelDSP.h/.cpp  the Vocal Channel signal chain
  Presets.h               13 Ambient space presets + 10 Vocal presets
  TronLookAndFeel.h/.cpp  the dark neon UI (cyan/orange, hairlines)
Ambient/                  Silktone Ambient processor + editor
VocalChannel/             Silktone Vocal Channel processor + editor
tests/test_dsp.cpp        standalone DSP checks + demo WAV renders
demo/                     the two rendered demo WAVs
```

## Plugin IDs

- Manufacturer code: `SgEn` (Sounds Good Ent.)
- Silktone Ambient: bundle `com.soundsgoodent.silktone-ambient`,
  plugin code `SAmb`
- Silktone Vocal Channel: bundle
  `com.soundsgoodent.silktone-vocal-channel`, plugin code `SVch`

The two plugin codes are distinct, so Logic and other hosts see them as
separate plugins.

## What was verified (and what was not)

Verified on Linux (this machine, November 17, 2026):

- **22 DSP checks pass** (`build/test_dsp`): reverb tail decays to the
  set RT60, decay control changes tail length, stereo tail is
  decorrelated, plate is brighter than hall, Room and Chamber types
  return earlier than Hall (Room ~96 ms, Chamber ~148 ms, Hall
  ~174 ms first return), dual-space per-side EQ affects only its side,
  shimmer adds octave-up energy (88×), pitch shifter is ~one octave up
  (ratio 1.99), ping-pong delay alternates sides, tape echo delay time
  and repeats decay, chorus modulates and stays bounded, compressor
  gain reduction, EQ shelves, saturation harmonics, dry/wet blend
  passthrough at 0 and wet tail at 1, maxed settings stay bounded with
  no NaN/inf.
- **Both plugins build as VST3 and Standalone on Linux** and both
  Standalones launch and run under Xvfb. The VST3 bundles have valid
  `moduleinfo.json` files with distinct CIDs, and no missing libraries.
- **Three JUCE 8 issues were found and fixed during the Linux build:**
  `drawTextButton` no longer exists in JUCE 8 (split into
  `drawButtonBackground` + `drawButtonText`);
  `AudioProcessorValueTreeState::getParameters()` was removed (use the
  processor's own `getParameters()`); and the static DSP library needs
  position-independent code to link into the VST3 shared object.

**Not verifiable here:** the Audio Unit (.component) cannot be built or
tested on Linux — it needs a Mac with Xcode. The Mac build steps above
are the same JUCE/CMake flow that produced the working Linux VST3 and
Standalone, but the AU itself is unproven until it is built and run in
Logic on a Mac. The plugin editors were compiled but not driven by
hand; the preset menu, the preset notes and the Room/Chamber type
menus are new since the last Linux build and should be clicked through
once in Logic.

## Notes

- The tape echo DSP is reused verbatim from the Silktone Tape Echo
  project (`~/workspace/plugins/silktone-tape-echo/`), not duplicated.
- The company website and email in the plugin metadata
  (`soundsgoodent.example`, `joel@soundsgoodent.example`) are
  placeholders — update them in `CMakeLists.txt` if you have a real
  domain before shipping.
- There is no AAX (Pro Tools) build in this project. AAX needs the
  Avid AAX SDK, which is a separate download from Avid and is not
  bundled with JUCE. If you want a Pro Tools version, that is a
  follow-up: add the AAX SDK path and the `AAX` format to the
  `juce_add_plugin` call, then build on the Mac.
