// Presets.h — factory presets for the Silktone ambient suite.
// Values are raw APVTS values (see each processor's parameter layout).
// Ambient: sat 0-100%, EQ -12..+12 dB, midFreq 200-8000 Hz, compThr -40..0 dB,
// ratio 1-8, chorus rate 0.05-5 Hz, depth/mix 0-100%, delay time 10-1000 ms,
// feedback/mix 0-100%, tape time 80-750 ms, tape feedback/mix knobs 1-9,
// tape age 0-10, reverb size/decay/damping/diffusion/movement/shimmer/early
// 0-100%, pre-delay 0-200 ms. Dual space: reverb type 0 = Hall, 1 = Plate,
// 2 = Room, 3 = Chamber; per-side EQ -12..+12 dB, levels 0-100%,
// space blend 0-50%.
// Vocal Channel: hpFreq 20-400 Hz, EQ -12..+12 dB, midFreq 200-8000 Hz,
// compThr -40..0 dB, ratio 1-10, attack 1-100 ms, release 50-1000 ms,
// makeup 0-12 dB, sat/blend 0-100%, output -12..+12 dB.
#pragma once
#include <array>

struct AmbientPreset
{
    const char* section; // menu section header, or nullptr to continue a section
    const char* name;
    const char* note;    // what was recorded in this room — shown under the preset bar
    float sat, lowDb, midDb, midFreqHz, highDb;
    bool compOn; float compThrDb, compRatio;
    bool chorusOn; float chorusRateHz, chorusDepth, chorusMix;
    bool delayOn; float delayTimeMs, delayFdbk, delayMix; bool delayTape;
    bool tapeOn; float tapeTimeMs, tapeFdbk, tapeMix, tapeAge;
    bool reverbOn; float revSize, revDecay, revDamp, revDiff, revMod,
        revShimmer, revPreDelayMs, revEarly;
    // Dual space: two reverbs, one per side, each with its own type
    // and EQ. Type: 0 = Hall, 1 = Plate, 2 = Room, 3 = Chamber.
    bool revDual; float revCharA, revCharB, revEqLowA, revEqHighA,
        revEqLowB, revEqHighB, revLevelA, revLevelB, revSpaceBlend;
    float blend, outputDb;
};

// The Ambient presets are stylistic spaces — real rooms, researched from
// their actual acoustic parameters, not guesses. The reverb decay knob
// maps to RT60 as 0.3 * 40^(decay/100) seconds, so each preset's decay is
// set from the room's measured or documented reverberation time.
//   Westlake Room A — 1,200 sq ft tracking room, Tom Hidley's flat,
//     controlled design: low reverb, extensive bass traps (Wikipedia,
//     "Westlake Recording Studios"). RT60 ~0.75 s. Michael Jackson's
//     Thriller was cut in this room.
//   Sunset Sound — Studio 1, built as a car workshop with a sloping
//     floor; "not too wet and splashy but still a nice reverb time and
//     ambience... a natural '70s kind of tone" (Sound on Sound, Joe
//     Chiccarelli). Original echo chambers. RT60 ~0.9 s. The Doors,
//     Prince and Van Halen tracked here.
//   Capitol Echo Chambers — eight subterranean trapezoidal concrete
//     chambers, 30 ft under the Capitol Tower parking lot, shellacked
//     hard lacquer; Les Paul designed chambers 1-4 (1956), 5-8 added
//     1969. Average delay ~3 s, maximum reverb time 5 s; "smooth sound
//     decay curve" (PS Audio / Sound on Sound). RT60 ~3 s. Sinatra, Nat
//     King Cole and the Beach Boys sang through these chambers.
//   EastWest Studio One — Bill Putnam's huge orchestral room (Western
//     Recorders, 1958): "rich and musical without being overwhelming,"
//     natural reverb "almost magically balanced." RT60 ~1.6 s. Pet
//     Sounds and Sinatra's classic sessions were cut here.
//   Stax — Memphis Studio A, a converted movie theater (Capitol Cinema)
//     with a sloped floor and no parallel surfaces: "big, deep yet raw
//     sound," "famously short, dry reverb." RT60 ~0.7 s. Otis Redding,
//     Booker T. & the MGs and Isaac Hayes.
//   Motown — Hitsville U.S.A. Studio A, the "Snakepit": a tiny basement
//     with a low ceiling, tight and punchy, plus the shellac-coated attic
//     echo chamber and an EMT 140 plate. RT60 ~0.6 s. The Supremes,
//     Stevie Wonder, Marvin Gaye and the Temptations.
//   Abbey Road Studio Two — the Beatles room: 60 x 38 x 24 ft, thick
//     solid wood floor, RT60 1.2 s (Native Instruments Abbey Road 60s
//     Drums manual) to 1.6 s (Abbey Road tour). Set to 1.4 s.
//   Electric Lady Studio A — Jimi Hendrix's room, designed by John
//     Storyk (opened August 1970, with Eddie Kramer): curved walls, a
//     thin air-entrained plaster ceiling that accidentally became a
//     giant low-frequency membrane absorber, so "reverb time actually
//     goes down as you go down in frequency... a very tight rock sound"
//     (Berklee / Storyk). RT60 ~0.8 s.
//   AIR Lyndhurst Hall — 1884 gothic church, hexagonal hall, George
//     Martin's studio: natural reverb 7.5 s, down to about 3 s via the
//     motorized canopy. Hans Zimmer's home. RT60 7.5 s.
//   Hansa Meistersaal — Berlin, 1910 chamber hall, 266 sq m, 7-metre
//     wooden coffered ceiling; Bowie's "Heroes" room, Tony Visconti's
//     three-microphone gated technique. RT60 ~1.8 s. Bowie's "Heroes"
//     was cut live in this room.
//   Columbia 30th Street — "The Church," New York: converted church
//     with 100-foot ceilings; Kind of Blue, Time Out, Highway 61.
//     Warm natural reverb. RT60 ~3 s.
//   Vintage Plate — the EMT 140 plate: dense, bright, the classic
//     plate sound. RT60 ~1.8 s.
//   Plate & Hall — dual space: vintage plate on the left, hall on the
//     right, each EQ'd differently, lightly blended.
inline constexpr std::array<AmbientPreset, 13> kAmbientPresets{{
    // Westlake Room A — the Thriller room. Big, clean, tight, flat.
    { "WEST COAST VERBS", "Westlake Room A",
      "Michael Jackson's Thriller was cut in this room.",
      10, 0, 0, 1000, 0, true, -18, 2.0f,
      true, 0.6f, 30, 20, true, 420, 30, 20, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 70, 25, 50, 80, 40, 0, 12, 55,
      false, 2, 2, 0, 0, 0, 0, 1, 1, 0.25f, 55, 0 },
    // Sunset Sound — natural '70s tone, wood, honest rooms.
    { nullptr, "Sunset Sound",
      "The Doors, Prince and Van Halen tracked here.",
      14, 1, 0, 1000, 1, true, -18, 2.0f,
      true, 0.7f, 35, 25, true, 380, 30, 22, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 65, 30, 55, 75, 45, 0, 15, 60,
      false, 2, 2, 0, 0, 0, 0, 1, 1, 0.25f, 55, 0 },
    // Capitol Echo Chambers — smooth concrete chambers, Sinatra's bloom.
    { nullptr, "Capitol Echo Chambers",
      "Sinatra, Nat King Cole and the Beach Boys sang through these chambers.",
      8, 0, 0, 1000, 0, true, -20, 2.0f,
      false, 0.8f, 40, 30, false, 320, 35, 30, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 60, 62, 45, 90, 30, 0, 25, 40,
      false, 3, 3, 0, 0, 0, 0, 1, 1, 0.25f, 60, 0 },
    // EastWest Studio One — Putnam's big orchestral room, rich and warm.
    { nullptr, "EastWest Studio One",
      "Pet Sounds and Sinatra's classic sessions were cut here.",
      10, 1, 0, 1000, 0, true, -18, 2.0f,
      true, 0.5f, 30, 20, true, 460, 25, 18, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 90, 45, 45, 85, 50, 0, 30, 55,
      false, 0, 0, 0, 0, 0, 0, 1, 1, 0.25f, 55, 0 },
    // Stax — Memphis theater, sloped floor, big deep raw soul room.
    { "SOUL", "Stax",
      "Otis Redding, Booker T. & the MGs, Isaac Hayes — the Memphis soul sound.",
      20, 2, 0, 1000, -1, true, -16, 2.5f,
      false, 0.8f, 40, 30, true, 290, 35, 25, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 75, 23, 55, 85, 35, 0, 10, 65,
      false, 2, 2, 0, 0, 0, 0, 1, 1, 0.25f, 55, 0 },
    // Motown — Hitsville Snakepit: tiny, tight, punchy, warm.
    { nullptr, "Motown — Hitsville",
      "The Supremes, Stevie Wonder, Marvin Gaye, the Temptations.",
      18, 1, 1, 1000, 0, true, -16, 2.5f,
      false, 0.8f, 40, 30, true, 240, 30, 22, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 40, 19, 60, 70, 30, 0, 8, 70,
      false, 2, 2, 0, 0, 0, 0, 1, 1, 0.25f, 50, 0 },
    // Abbey Road Studio Two — the Beatles room, wood floor, warm and live.
    { "CLASSIC ROOMS", "Abbey Road Studio Two",
      "The Beatles — most of their records were made in this room.",
      12, 0, 0, 1000, 0, true, -18, 2.0f,
      true, 0.6f, 30, 22, true, 420, 28, 20, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 75, 42, 50, 80, 45, 0, 20, 55,
      false, 0, 0, 0, 0, 0, 0, 1, 1, 0.25f, 55, 0 },
    // Electric Lady Studio A — Storyk's curved room, tight low end, warm.
    { nullptr, "Electric Lady Studio A",
      "Jimi Hendrix built it; his room, his sound.",
      15, -1, 0, 1000, 0, true, -18, 2.0f,
      true, 0.8f, 35, 25, true, 320, 30, 22, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 60, 27, 50, 85, 40, 0, 12, 60,
      false, 2, 2, 0, 0, 0, 0, 1, 1, 0.25f, 55, 0 },
    // AIR Lyndhurst Hall — gothic church hall, 7.5 s of natural reverb.
    { "GREAT HALLS", "AIR Lyndhurst Hall",
      "Hans Zimmer's home — the big film-score hall.",
      8, 0, 0, 1000, 0, true, -20, 2.0f,
      true, 0.4f, 40, 25, false, 320, 35, 30, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 95, 87, 35, 85, 55, 0, 45, 50,
      false, 0, 0, 0, 0, 0, 0, 1, 1, 0.25f, 60, 0 },
    // Hansa Meistersaal — Bowie's "Heroes" room, wooden coffered hall.
    { nullptr, "Hansa Meistersaal",
      "Bowie's \"Heroes\" — cut live in this room.",
      12, 1, 0, 1000, 1, true, -18, 2.0f,
      true, 0.5f, 35, 22, true, 380, 25, 18, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 70, 49, 45, 80, 40, 0, 25, 60,
      false, 0, 0, 0, 0, 0, 0, 1, 1, 0.25f, 55, 0 },
    // Columbia 30th Street — "The Church": Kind of Blue's warm expanse.
    { nullptr, "Columbia 30th Street",
      "Kind of Blue, Time Out, Highway 61 — the Church.",
      10, 1, 0, 1000, 0, true, -18, 2.0f,
      true, 0.6f, 30, 20, true, 420, 25, 18, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 90, 62, 40, 85, 45, 0, 30, 55,
      false, 0, 0, 0, 0, 0, 0, 1, 1, 0.25f, 55, 0 },
    // Vintage Plate — the EMT 140: dense, bright, the classic plate.
    { "CLASSICS", "Vintage Plate",
      "The EMT 140 — the plate on a thousand classic records.",
      15, 0, 0, 1000, 2, true, -18, 2.0f,
      true, 0.7f, 30, 20, true, 300, 25, 20, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 55, 49, 40, 90, 35, 0, 15, 55,
      false, 1, 1, 0, 0, 0, 0, 1, 1, 0.25f, 50, 0 },
    // Plate & Hall — dual space: vintage plate on the left, hall on the
    // right, each EQ'd differently, lightly blended.
    { nullptr, "Plate & Hall",
      "Plate on the left, hall on the right — blend two spaces at once.",
      10, 0, 0, 1000, 1, true, -18, 2.0f,
      true, 0.6f, 35, 25, true, 420, 25, 20, true,
      false, 320, 4, 4.5f, 3.5f,
      true, 70, 62, 45, 85, 50, 0, 30, 50,
      true, 1, 0, 2, 3, -1, 1, 1, 1, 0.3f, 55, 0 },
}};

struct VocalPreset
{
    const char* name;
    bool hpOn; float hpFreqHz;
    float lowDb, midDb, midFreqHz, highDb;
    float compThrDb, compRatio, compAttackMs, compReleaseMs, compMakeupDb;
    float sat, blend, outputDb;
};

inline constexpr std::array<VocalPreset, 10> kVocalPresets{{
    // Airy Vocal — high-pass the rumble, lift the air, gentle glue.
    { "Airy Vocal", true, 100, 0, 0, 2500, 3, -20, 3.0f, 8, 200, 4, 8, 100, 0 },
    // Warm & Close — a little low warmth, a little tape-ish drive.
    { "Warm & Close", true, 80, 2, -1, 300, 0, -18, 2.5f, 10, 220, 3, 15, 100, 0 },
    // Radio Voice — present mids, firm compression.
    { "Radio Voice", true, 120, -1, 3, 2500, 1, -16, 4.0f, 6, 160, 5, 12, 100, 0 },
    // Gentle Glue — barely-there compression, mostly the EQ.
    { "Gentle Glue", true, 70, 0, 0, 2000, 1, -24, 2.0f, 12, 260, 2, 6, 80, 0 },
    // Bright Pop — sparkle on top, controlled bottom.
    { "Bright Pop", true, 90, -1, 0, 3000, 4, -18, 3.0f, 8, 200, 4, 12, 100, 0 },
    // Clean & Dry — flat, just the high-pass and a touch of glue.
    { "Clean & Dry", true, 80, 0, 0, 2500, 0, -22, 2.0f, 10, 240, 2, 5, 100, 0 },
    // Studio Sheen — polished top end, even dynamics, sits in the mix.
    { "Studio Sheen", true, 90, 0, 1, 3200, 4, -19, 3.0f, 7, 190, 4, 10, 100, 0 },
    // Lo-Fi Warmth — dark top, warm lows, soft saturation.
    { "Lo-Fi Warmth", true, 60, 3, 0, 1800, -3, -17, 2.0f, 14, 260, 2, 22, 100, -1 },
    // Spoken Word — tight lows, clear mids, fast-ish compression.
    { "Spoken Word", true, 110, -1, 2, 2200, 1, -18, 3.5f, 5, 140, 4, 8, 100, 0 },
    // Choir Glue — soft and wide: gentle everything, blend pulled back.
    { "Choir Glue", true, 70, 1, 0, 2800, 2, -24, 2.0f, 15, 300, 3, 6, 85, 0 },
}};
