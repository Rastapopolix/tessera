# Parameters

Every setting of Tessera with its number, generated from `firmware/src/pid.c` by `tests/pid_test.c` (`PID_UPDATE=1 sh tests/run_tests.sh` rewrites it; the suite fails when it is out of date). The numbers are fixed: mods and saved projects keep them.

The engine's eight (ENGINE 1 to 8) are named by the track's engine; their ranges here are the first track's default engine's.

## Track settings (one value per track)

| ID | Name | Range | Default | What it does |
| --- | --- | --- | --- | --- |
| 0x000 | LEVEL | 0 to 127 | 104 | The track's volume in the mix. |
| 0x001 | ENV ATTACK | 0 to 127 | 10 | How long a note takes to reach full level. |
| 0x002 | ENV DECAY | 0 to 127 | 70 | How long it then takes to fall to SUSTAIN. |
| 0x003 | ENV SUSTAIN | 0 to 127 | 90 | The level a held note stays at. |
| 0x004 | ENV RELEASE | 0 to 127 | 60 | How long a note takes to fade once let go. |
| 0x005 | ENV TO FILTER | -64 to 63 | 0 | How far the envelope opens (or, below 0, closes) the filter. |
| 0x006 | ENV TO PITCH | -64 to 63 | 0 | How far the envelope bends the pitch. |
| 0x007 | ENV TO SHAPE | -64 to 63 | 0 | How far the envelope moves the engine's SHAPE. |
| 0x008 | RECORD QUANTIZE | OFF, 1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/2, 1/1, 2BAR, 4BAR | OFF | Moves a recorded take onto this grid when recording ends; OFF keeps the timing as played. |
| 0x009 | LFO RATE | 0 to 127 | 60 | How fast the track's LFO moves. |
| 0x00A | LFO WAVE | SIN, TRI, SAW, SQR, S&H | SIN | The LFO's shape: sine, triangle, saw, square or a random step each cycle (S&H). |
| 0x00B | LFO PHASE | 0 to 127 | 0 | Where in its cycle the LFO starts. |
| 0x00C | LFO FADE | 0 to 127 | 0 | How long the LFO takes to fade in after a note starts. |
| 0x00D | LFO TO PITCH | -64 to 63 | 0 | How far the LFO moves the pitch (vibrato). |
| 0x00E | LFO TO FILTER | -64 to 63 | 0 | How far the LFO moves the filter. |
| 0x00F | LFO TO SHAPE | -64 to 63 | 0 | How far the LFO moves the engine's SHAPE. |
| 0x010 | LFO TO LEVEL | 0 to 127 | 0 | How far the LFO moves the level (tremolo). |
| 0x011 | ARP MODE | OFF, UP, DN, UPDN, RND, ORD, REPEAT, DNUP, RND1 | OFF | The arpeggio: UP, DN down, UPDN up then down, DNUP down then up, RND a new random note each step, RND1 one random order kept until the held notes change, ORD as the keys were pressed, REPEAT the last note; OFF: none. |
| 0x012 | ARP RATE | 1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/2, 1/1, 2BAR, 4BAR | 1/16 | How often the arpeggiator plays a note. |
| 0x013 | ARP OCTAVES | 1 to 4 | 1 | How many octaves the arpeggio climbs through. |
| 0x014 | ARP GATE | 1 to 127 | 64 | How long each arpeggio note lasts, as a share of its step. |
| 0x015 | ARP SWING | 0 to 100 % | 0 | Delays every second arpeggio note, for a swung feel. |
| 0x016 | ARP PROBABILITY | 0 to 127 | 127 | The chance each arpeggio note plays. |
| 0x017 | ARP HOLD | OFF, ON | OFF | ON keeps the arpeggio going after the keys are let go. |
| 0x018 | ARP ORDER | NOTE, PLAY | NOTE | NOTE: the held notes from low to high; PLAY: in the order they were pressed. MODE ORD always plays them as pressed. |
| 0x019 | ROOT | 0 to 11 | 0 | The scale's key note. |
| 0x01A | SCALE | CHR, MAJ, MIN, DOR, MIX, PEN, MPEN, HARM, PHRY, LYD, LOC, MEL, BLUES, WHOLE, DIMHW, DIMWH, 5EDO, 7EDO, 9EDO, 10EDO, 13EDO, 14EDO, 15EDO, 16EDO, 17EDO, 18EDO, 19EDO, 20EDO, 21EDO, 22EDO, 23EDO, 24EDO, 26EDO, 27EDO, 29EDO, 31EDO, 34EDO, 36EDO, 41EDO, 53EDO, JI-MAJ, JI-MIN, JI-12, JI-7L, JI-7, HARM8, HARM16, PARTCH, HARRIS, PYTH, MT-1/4, MT-1/3, MT-1/6, WERCK3, KIRN3, VALLOT, YOUNG2, RAST-J, SABA-J, SHRUTI, SLENDR, PELOG, BP-ET, BP-JI, RAST, BAYATI, HIJAZ, SABA, HUSAYN, SIKAH, IN, YO, HIRA, IWATO, KUMOI, RYUK | CHR | The scale the keys and the generators play in. SCALES' USER family has four of your own, USER1 to USER4, made on the keys. |
| 0x01B | QUANTIZE | OFF, SNAP, WHITE, ALL, MPC, CHRD | OFF | How the keys meet the scale: OFF plays every key, SNAP moves a note into the scale, WHITE puts the scale on the white keys, ALL and MPC lay it out over the keys or MPC pads; CHRD snaps the keys, MIDI and the generators to the nearest note of the chord a CHORD track plays (no CHORD track: as SNAP). Not OFF: KNOB 2 on NOTES moves a note by the scale's notes (CHRD: the chord's). Notes already written stay: Snap to scale in the pattern's sheet (OCT+ held) moves them onto the scale, once. |
| 0x01C | TRANSPOSE | -24 to 24 semitones | 0 | Moves the track's notes up or down by semitones. |
| 0x01D | PATTERN LENGTH | 1 to 64 | 16 | How many steps the track's pattern has. Each of its 8 patterns keeps its own. |
| 0x01E | PATTERN DIVISION | 1/4, 1/8, 1/16, 1/32, 8T, 16T, 1/2, 1/1, 2BAR, 4BAR | 1/16 | How long one step is. Each of the track's 8 patterns keeps its own. |
| 0x01F | PATTERN SWING | 0 to 100 % | 0 | Delays every second step, for a swung feel. Each of the track's 8 patterns keeps its own. |
| 0x020 | PATTERN GATE | 1 to 127 | 64 | How long each step's note lasts, as a share of the step. Each of the track's 8 patterns keeps its own. |
| 0x025 | VOICE MODE | POLY, MONO, LEG, UNI | POLY | POLY plays chords; MONO one note at a time; LEG(ATO) one at a time, a note played while one is held not starting again; UNI stacks detuned voices on one note. |
| 0x026 | GLIDE | 0 to 127 | 0 | How long a note takes to slide to the next. |
| 0x027 | PAN | -64 to 63 | 0 | The track's place from left to right. |
| 0x028 | MUTE | OFF, ON | OFF | ON silences the track; it keeps playing in time. |
| 0x029 | GLIDE MODE | RATE, TIME | RATE | RATE: the slide's speed is fixed; TIME: every slide takes the same time, however far. |
| 0x02A | NOTE PRIORITY | LAST, LOW, HIGH | LAST | Which note sounds in MONO when several are held: the last, the lowest or the highest. |
| 0x02B | VOICE ALLOCATION | ROT, REUSE | ROT | ROT gives each new note the next free voice; REUSE the free voice last nearest its pitch (smoother glides). |
| 0x02C | UNISON DETUNE | 0 to 127 | 40 | How far apart UNI's stacked voices are tuned. |
| 0x031 | MOD 1 SOURCE | OFF, LFO, ENV, VEL, KEY, RAND, MODW, AT, EXPR, S&H, SLEW | OFF | MOD route 1: what moves the destination (LFO, envelope, velocity, key, random, the controllers). |
| 0x032 | MOD 1 DESTINATION | OFF, PITCH, CUT, SHP, AMP, PAN, RATE, VIB, E1, E2, E3, E4, E5, E6, E7, E8, DEPTH | OFF | MOD route 1: what is moved (pitch, filter, shape, level, pan, LFO rate or depth, the engine's own). |
| 0x033 | MOD 1 AMOUNT | -64 to 63 | 0 | MOD route 1: how far, and which way. |
| 0x034 | MOD 2 SOURCE | OFF, LFO, ENV, VEL, KEY, RAND, MODW, AT, EXPR, S&H, SLEW | OFF | MOD route 2: what moves the destination (LFO, envelope, velocity, key, random, the controllers). |
| 0x035 | MOD 2 DESTINATION | OFF, PITCH, CUT, SHP, AMP, PAN, RATE, VIB, E1, E2, E3, E4, E5, E6, E7, E8, DEPTH | OFF | MOD route 2: what is moved (pitch, filter, shape, level, pan, LFO rate or depth, the engine's own). |
| 0x036 | MOD 2 AMOUNT | -64 to 63 | 0 | MOD route 2: how far, and which way. |
| 0x037 | MOD 3 SOURCE | OFF, LFO, ENV, VEL, KEY, RAND, MODW, AT, EXPR, S&H, SLEW | OFF | MOD route 3: what moves the destination (LFO, envelope, velocity, key, random, the controllers). |
| 0x038 | MOD 3 DESTINATION | OFF, PITCH, CUT, SHP, AMP, PAN, RATE, VIB, E1, E2, E3, E4, E5, E6, E7, E8, DEPTH | OFF | MOD route 3: what is moved (pitch, filter, shape, level, pan, LFO rate or depth, the engine's own). |
| 0x039 | MOD 3 AMOUNT | -64 to 63 | 0 | MOD route 3: how far, and which way. |
| 0x03A | MOD 4 SOURCE | OFF, LFO, ENV, VEL, KEY, RAND, MODW, AT, EXPR, S&H, SLEW | OFF | MOD route 4: what moves the destination (LFO, envelope, velocity, key, random, the controllers). |
| 0x03B | MOD 4 DESTINATION | OFF, PITCH, CUT, SHP, AMP, PAN, RATE, VIB, E1, E2, E3, E4, E5, E6, E7, E8, DEPTH | OFF | MOD route 4: what is moved (pitch, filter, shape, level, pan, LFO rate or depth, the engine's own). |
| 0x03C | MOD 4 AMOUNT | -64 to 63 | 0 | MOD route 4: how far, and which way. |
| 0x051 | CHORD | OFF, DIA3, DIA7, MAJ, MIN, DOM7, MAJ7, MIN7, SUS4, POW, SUS2, 7SUS4, ADD9, MAJ6 | OFF | One key plays a chord: in the scale (DIA3, DIA7) or a fixed kind (MAJ, MIN, 7ths, SUS4, POWER, SUS2, 7SUS4, ADD9, MAJ6); OFF plays one note. |
| 0x052 | CHORD VOICING | CLOSE, OPEN, INV1, INV2, +OCT | CLOSE | How the chord's notes are spread: close, open, an inversion, or with the root an octave down. |
| 0x053 | MPC DEGREE | 1 to 12 | 1 | With QUANTIZE MPC: the scale degree the pad H02 plays. |
| 0x054 | DRUM KICK | 0 to 127 | 127 | The kick's level in the kit. |
| 0x055 | DRUM SNARE | 0 to 127 | 127 | The snare's level in the kit. |
| 0x056 | DRUM CLAP | 0 to 127 | 127 | The clap's level in the kit. |
| 0x057 | DRUM CLOSED HAT | 0 to 127 | 127 | The closed hi-hat's level in the kit. |
| 0x058 | DRUM OPEN HAT | 0 to 127 | 127 | The open hi-hat's level in the kit. |
| 0x059 | DRUM TOM | 0 to 127 | 127 | The tom's level in the kit. |
| 0x05A | DRUM RIM | 0 to 127 | 127 | The rimshot's level in the kit. |
| 0x05B | DRUM BELL | 0 to 127 | 127 | The bell's level in the kit. |
| 0x05C | LFO SYNC | OFF, 4BAR, 2BAR, 1/1, 1/2, 1/4, 1/8, 8T, 1/16, 16T, 1/32 | OFF | Ties the LFO to the tempo: one cycle every bar, beat or step chosen; OFF runs at its own RATE. |
| 0x05D | LFO TRIGGER | NOTE, FREE | NOTE | NOTE starts the LFO again with each note; FREE lets it run on. |
| 0x05E | LFO POLARITY | BI, UNI | BI | BI moves both ways from the setting; UNI only one way. For this page's depths: a mod from the LFO has its own + or +- (OCT+ on MODS). |
| 0x05F | STEREO SPREAD | 0 to 127 | 0 | Spreads the track's voices from left to right. |
| 0x060 | FX 1 TYPE | OFF, PHASER, WIDTH, FORMANT, TAPE, SPIT, ECHO, ROOM, REVERB, CHORUS, FILTER | OFF | The track's insert effect 1. |
| 0x061 | FX 1 MIX | 0 to 127 | 0 | How much of insert effect 1 is heard. |
| 0x062 | FX 1 RATE | 0 to 127 | 60 | Insert effect 1's speed (its time or size, by the effect). |
| 0x063 | FX 1 CHARACTER | 0 to 127 | 64 | Insert effect 1's character (its depth, decay or the like, by the effect). |
| 0x064 | FX 2 TYPE | OFF, PHASER, WIDTH, FORMANT, TAPE, SPIT, ECHO, ROOM, REVERB, CHORUS, FILTER | OFF | The track's insert effect 2. |
| 0x065 | FX 2 MIX | 0 to 127 | 0 | How much of insert effect 2 is heard. |
| 0x066 | FX 2 RATE | 0 to 127 | 60 | Insert effect 2's speed (its time or size, by the effect). |
| 0x067 | FX 2 CHARACTER | 0 to 127 | 64 | Insert effect 2's character (its depth, decay or the like, by the effect). |
| 0x068 | FX 3 TYPE | OFF, PHASER, WIDTH, FORMANT, TAPE, SPIT, ECHO, ROOM, REVERB, CHORUS, FILTER | OFF | The track's insert effect 3. |
| 0x069 | FX 3 MIX | 0 to 127 | 0 | How much of insert effect 3 is heard. |
| 0x06A | FX 3 RATE | 0 to 127 | 60 | Insert effect 3's speed (its time or size, by the effect). |
| 0x06B | FX 3 CHARACTER | 0 to 127 | 64 | Insert effect 3's character (its depth, decay or the like, by the effect). |
| 0x06C | FX SEND | 0 to 127 | 0 | How much of the track goes to the effects (the first send effect when they are SPLIT). |
| 0x06D | FX SEND 2 | 0 to 127 | 0 | How much of the track goes to send effect 2 (FX MODE SPLIT). |
| 0x06E | FX SEND 3 | 0 to 127 | 0 | How much of the track goes to send effect 3 (FX MODE SPLIT). |
| 0x06F | ENGINE 1 | 0 to 0 | 0 | The engine's parameter 1: what it is depends on the track's engine (its name on the EDIT pages). |
| 0x070 | ENGINE 2 | 0 to 0 | 0 | The engine's parameter 2: what it is depends on the track's engine (its name on the EDIT pages). |
| 0x071 | ENGINE 3 | 0 to 0 | 0 | The engine's parameter 3: what it is depends on the track's engine (its name on the EDIT pages). |
| 0x072 | ENGINE 4 | 0 to 0 | 0 | The engine's parameter 4: what it is depends on the track's engine (its name on the EDIT pages). |
| 0x073 | ENGINE 5 | 0 to 0 | 0 | The engine's parameter 5: what it is depends on the track's engine (its name on the EDIT pages). |
| 0x074 | ENGINE 6 | 0 to 0 | 0 | The engine's parameter 6: what it is depends on the track's engine (its name on the EDIT pages). |
| 0x075 | ENGINE 7 | 0 to 0 | 0 | The engine's parameter 7: what it is depends on the track's engine (its name on the EDIT pages). |
| 0x076 | ENGINE 8 | 0 to 0 | 0 | The engine's parameter 8: what it is depends on the track's engine (its name on the EDIT pages). |

## Global settings

| ID | Name | Range | Default | What it does |
| --- | --- | --- | --- | --- |
| 0x100 | TEMPO | 40 to 240 | 120 | The project's speed in beats per minute. |
| 0x101 | SWING | 0 to 100 % | 0 | Delays every second step of every track, for a swung feel. |
| 0x102 | CLOCK SOURCE | INT, USB, TRS | INT | Where the tempo comes from: the FM-1 itself, USB MIDI or the TRS MIDI input. |
| 0x103 | TUNE | -50 to 50 | 0 | Fine tuning of every track, in cents. |
| 0x10C | MIDI INPUT | USB, TRS | USB | Which MIDI input plays the tracks: USB or TRS. |
| 0x10D | DRUM MIDI CHANNEL | OFF, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 | OFF | The MIDI channel the first drum track listens to and sends on (MIDI OUT). OFF (the default): it follows ROUT like the other tracks. 10 is General MIDI's drum channel. (The device's, not the project's.) |
| 0x10E | MIDI ROUTING | CH1-4, SEL, CH5-8, CH9-12, CH13-16 | CH1-4 | Which MIDI channels reach which tracks: four channels to the four tracks, or every channel to the selected track (SEL). MIDI OUT sends each track on the same channels (SEL: 1 to 4). |
| 0x110 | PROJECT SLOT | (a button) |  | The project slot LOAD and SAVE use. |
| 0x111 | BOOT PROJECT | OFF, A, B, C, D | OFF | The project loaded when the FM-1 starts; OFF starts a new one. (The device's, not the project's.) |
| 0x112 | PROJECT LOAD | (a button) |  | Loads the project in the slot. |
| 0x113 | PROJECT SAVE | (a button) |  | Saves the project to the slot. |
| 0x114 | ENGINE | (a button) |  | The selected track's sound engine. |
| 0x115 | A4 TUNING | 400 to 480 Hz | 440 | The pitch of A4 for the whole device, in hertz. (The device's, not the project's.) |
| 0x116 | CLEAR SEQUENCE | (a button) |  | Empties the selected track's pattern (it asks first). |
| 0x117 | INIT SOUND | (a button) |  | Resets the selected track's sound to its engine's starting point. |
| 0x11B | SEND FX 1 TYPE | OFF, PHASER, WIDTH, FORMANT, TAPE, SPIT, ECHO, ROOM, REVERB, CHORUS, FILTER | OFF | Send effect 1, shared by every track. |
| 0x11C | SEND FX 1 MIX | 0 to 127 | 127 | How much of send effect 1 is heard. |
| 0x11D | SEND FX 1 RATE | 0 to 127 | 60 | Send effect 1's speed (its time or size, by the effect). |
| 0x11E | SEND FX 1 CHARACTER | 0 to 127 | 64 | Send effect 1's character (its depth, decay or the like, by the effect). |
| 0x11F | SEND FX 2 TYPE | OFF, PHASER, WIDTH, FORMANT, TAPE, SPIT, ECHO, ROOM, REVERB, CHORUS, FILTER | OFF | Send effect 2, shared by every track. |
| 0x120 | SEND FX 2 MIX | 0 to 127 | 127 | How much of send effect 2 is heard. |
| 0x121 | SEND FX 2 RATE | 0 to 127 | 60 | Send effect 2's speed (its time or size, by the effect). |
| 0x122 | SEND FX 2 CHARACTER | 0 to 127 | 64 | Send effect 2's character (its depth, decay or the like, by the effect). |
| 0x123 | SEND FX 3 TYPE | OFF, PHASER, WIDTH, FORMANT, TAPE, SPIT, ECHO, ROOM, REVERB, CHORUS, FILTER | OFF | Send effect 3, shared by every track. |
| 0x124 | SEND FX 3 MIX | 0 to 127 | 127 | How much of send effect 3 is heard. |
| 0x125 | SEND FX 3 RATE | 0 to 127 | 60 | Send effect 3's speed (its time or size, by the effect). |
| 0x126 | SEND FX 3 CHARACTER | 0 to 127 | 64 | Send effect 3's character (its depth, decay or the like, by the effect). |
| 0x127 | FX RETURN | 0 to 127 | 100 | The level of the send effects in the mix. |
| 0x128 | FX MODE | CHAIN, SPLIT | CHAIN | CHAIN: one send through all three effects in a row; SPLIT: a send to each effect of its own. |
| 0x129 | VOICE BUDGET | 8 to 32 pts | 16 | How much sound work the FM-1 shares out between the tracks; lower leaves more headroom, higher allows more voices. (The device's, not the project's.) |

## Tessera (one value per track)

| ID | Name | Range | Default | What it does |
| --- | --- | --- | --- | --- |
| 0x200 | GENERATOR MODE | STEP, TURING, EUCLID, MARKOV, MORPH, ATLAS, BOUNCE, AWAKE, POLY, QUAD, CONSTL, SERIES, CHORD, ANSWER | STEP | SEQ and ARP show the pages of the MODE only. STEP plays the track's own steps; ARP the keys held, as an arpeggio; CHORD plays a progression's chords in the track's scale (QNT CHRD: the other tracks follow them); TURING, EUCLID, MARKOV and MORPH make its notes; ATLAS plays kick, snare, hat and toms (best on a DRUM track); BOUNCE plays where a ball hits the walls of its box; AWAKE adds two loops of its own lengths; POLY steps four notes on four rhythms; QUAD plays the track's row of a four by four grid the tracks share; CONSTL (CONSTELLATION) plays the stars of a passing sky as they cross a ship's crosshair; SERIES plays Per Norgard's infinity series; ANSWER answers what you play on its keys. |
| 0x201 | TURING LENGTH | 2 to 16 steps | 16 | How many steps the register loops over before it comes round again. |
| 0x202 | TURING PROB | 0 to 100 % | 0 | The chance a step changes as it comes round: 0 locks the loop, 100 is new every pass. |
| 0x203 | TURING SPAN | 1 to 3 oct | 2 | How many octaves the notes spread over. |
| 0x204 | EUCLID STEPS | 1 to 32 steps | 16 | The rhythm's length in steps. |
| 0x205 | EUCLID PULSES | 0 to 32 | 4 | How many hits, spread as evenly as they go over STEPS (at most STEPS). |
| 0x206 | EUCLID ROTATION | 0 to 31 | 0 | Turns the rhythm round by this many steps (fewer than STEPS). |
| 0x207 | MARKOV LEAP | 0 to 100 % | 25 | How far each note jumps from the last: 0 moves to its neighbours, 100 leaps widely. |
| 0x208 | MARKOV DENSITY | 5 to 100 % | 70 | The chance a step plays a note; the others rest. |
| 0x209 | MARKOV SPAN | 1 to 3 oct | 2 | How many octaves the notes spread over. |
| 0x20A | MORPH POSITION | 0 to 127 | 64 | Blends rhythm A (0) into rhythm B (full). |
| 0x20B | MORPH RHYTHM A | 0 to 7 | 0 | Rhythm A: one of eight preset patterns. |
| 0x20C | MORPH RHYTHM B | 0 to 7 | 2 | Rhythm B: one of eight preset patterns. |
| 0x20D | EVOLVE LOCK | 0 to 100 % | 0 | How much of the phrase stays when EVOLVE mutates it: 100 loops it, 0 is new every time. Turing's is its PROB turned round, Atlas' its CHAOS; Constellation's the chance a step plays its loop instead of the sky; Euclid, Bounce, Awake, Poly and Quad have none. |
| 0x20E | EVOLVE EVERY | OFF, 1, 2, 4, 8, 16 | OFF | Bars between mutations; OFF keeps the phrase as it is. |
| 0x20F | EVOLVE AMOUNT | 0 to 100 % | 30 | How much changes at each mutation. |
| 0x210 | EVOLVE ROLL | 1 to 128 | 1 | The track's seed, 1 to 128: each one a line of its own, the same every time. Playing, a new one starts on the next bar line. OCT- held on EVOLVE, KNOB 1: ROLL ALL, every generator track a new seed at once; turn left to get the last ones back. |
| 0x211 | FORM | OFF, AB, ABAC, ABCB, AABA, ABCA, AABC | OFF | The order the sections play in (A, B, C), each EVERY bars long; OFF plays one phrase. |
| 0x212 | FORM CONTRAST | 0 to 100 % | 60 | How different a new section is from the one before it. |
| 0x213 | FORM FLOURISH | -12 to 12 semitones | 0 | Semitones added on the last beat of every phrase; 0 is off. |
| 0x214 | TWEAK SHIFT | -24 to 24 semitones | 0 | Transposes the notes by semitones; while following, by scale steps from the followed note. |
| 0x215 | TWEAK DROP | 0 to 100 % | 0 | The chance a hit is left out, whatever the generator. |
| 0x216 | TWEAK HUMAN | 0 to 100 % | 0 | Varies each note's velocity and length a little, as a player would: at 100 up to 20 softer or louder and up to half as long; the same each time the piece plays. STEP tracks too. The timing: TIMING's HUMAN. |
| 0x217 | TWEAK FOLLOW | OFF, T1, T2, T3, T4 | OFF | Plays another track's pitch, SHIFT away, in this track's scale; OFF is its own. |
| 0x218 | ATLAS X | 0 to 255 | 128 | Where on the map of drum patterns, left to right; between two, a blend of both. |
| 0x219 | ATLAS Y | 0 to 255 | 128 | Where on the map of drum patterns, top to bottom; between two, a blend of both. |
| 0x21A | ATLAS DENSITY | 0 to 100 % | 50 | How full the kick, snare, hat and toms are: 0 silent, 100 every hit the map offers. Hold OCT+ on GEN for each part's own: KICK, SNR, HAT and TOMS. |
| 0x21B | BOUNCE SPEED | 1 to 64 | 24 | How far the ball travels each step: faster, more wall hits, more notes. In FALL, how fast the leaves fall. |
| 0x21C | BOUNCE ANGLE | 0 to 90 | 37 | The ball's path from the floor, in degrees: 0 and 90 go back and forth, an angle between draws a longer loop. EVOLVE nudges it. In FALL it is the WIND: the middle is calm, lower blows the leaves left (the melody leans low), higher right (high); further out, gusts hurry them down together. |
| 0x21D | BOUNCE BALLS | 1 to 5 | 1 | How many balls: each one more starts from a place and an angle of its own, so they cross; balls hitting on the same step play together, three notes at most. Past 4, FALL: eight leaves fall at speeds of their own and play where they land (left low, right high; one in seven an octave down), rest, and fall again. |
| 0x21E | AWAKE LENGTH A | 1 to 16 | 8 | Steps in loop A, the melody: its values are degrees of the scale or rests. ROLL draws them, EVOLVE changes a few; a white key picks a step: KNOB 1 its A, KNOB 2 its B. |
| 0x21F | AWAKE LENGTH B | 1 to 16 | 7 | Steps in loop B, which lifts the melody up the scale; a length other than A's shifts it against A. |
| 0x220 | AWAKE PROB | 0 to 100 % | 100 | The chance each of A's notes plays: 100 every one, lower leaves gaps. |
| 0x221 | POLY RHYTHM 1 | OFF, /1, /2, /3, /4, /5, /6, /7, /8, /9, /10, /11, /12, /13, /14, /15, /16 | /4 | Every how many steps rhythm 1 moves the four notes on (OFF: never). Rhythms that land together play once, louder: 4 against 3 is a polyrhythm. |
| 0x222 | POLY RHYTHM 2 | OFF, /1, /2, /3, /4, /5, /6, /7, /8, /9, /10, /11, /12, /13, /14, /15, /16 | /3 | Every how many steps rhythm 2 moves the four notes on (OFF: never). |
| 0x223 | POLY RHYTHM 3 | OFF, /1, /2, /3, /4, /5, /6, /7, /8, /9, /10, /11, /12, /13, /14, /15, /16 | OFF | Every how many steps rhythm 3 moves the four notes on (OFF: never). |
| 0x224 | POLY RHYTHM 4 | OFF, /1, /2, /3, /4, /5, /6, /7, /8, /9, /10, /11, /12, /13, /14, /15, /16 | OFF | Every how many steps rhythm 4 moves the four notes on (OFF: never). |
| 0x225 | POLY SPAN | 1 to 3 | 1 | Octaves the four notes spread over; ROLL draws them, EVOLVE moves them a little. |
| 0x226 | TWEAK BRANCH | 0 to 100 % | 0 | The chance a hit plays on track TO's sound instead of this one's, so two tracks answer each other; 0 is off. The tosses replay with the SEED. |
| 0x227 | TWEAK BRANCH TO | OFF, T1, T2, T3, T4 | OFF | The track a branched hit plays on; OFF keeps every hit here. Turned on: COIN tosses each hit, TOGGLE switches side on a toss won, LATCH tosses once a bar. |
| 0x228 | TWEAK BRANCH MODE | COIN, TOGGLE, LATCH | COIN | COIN tosses each hit; TOGGLE switches side on a toss won (100 alternates); LATCH tosses once a bar and keeps the bar's hits on one side. |
| 0x229 | TWEAK LOGIC | OFF, AND, OR, XOR, NOT | OFF | Combines the hits with track WITH's: AND plays only when both hit, OR when either does, XOR when one does, NOT only when the other rests; OFF is its own. |
| 0x22A | TWEAK LOGIC WITH | OFF, T1, T2, T3, T4 | OFF | The track LOGIC reads: a lower track's hit on this step, a higher one's on the step before. |
| 0x22B | TWEAK COND | OFF, 1:2, 2:2, 1:4, 2:4, 3:4, 4:4, 1:8, 8:8, FIRST, !FIRST | OFF | The bars a hit may play in, counted from PLAY (with FORM on, from the section's start): 1:4 is the first bar of every four; FIRST only the first, !FIRST the rest. |
| 0x22C | TIDE SPEED | 0 to 100 | 50 | How fast the track's TIDE drifts (mods: TIDE 1, 2, 3, three slow chaotic circuits): 50 is about 17, 3 and 1 minutes a turn; each 12.5 more twice as fast. |
| 0x22D | SWARM RATE | 0 to 100 | 50 | How fast the track's eight SWARM LFOs run: 50 is 64 s for SWARM 1 down to 2.4 s for SWARM 8; each 12.5 more twice as fast. |
| 0x22E | QUAD DIRECTION | FWD, BACK, PEND, RAND, HOLD | FWD | How the track's playhead moves along its row of the grid: forward, back, back and forth, at random; HOLD rests on its own steps and moves only when a LINK moves it. |
| 0x22F | QUAD PROB | 0 to 100 % | 100 | The chance each of the row's notes plays: 100 every one, lower leaves gaps. |
| 0x230 | QUAD LINK | OFF, T1, T2, T12, T3, T13, T23, T123, T4, T14, T24, T124, T34, T134, T234, T1234 | OFF | Other QUAD tracks this one's steps also move on, one, two or all three, each played for this step's gate: slow tracks pulled along by a fast one, or HOLD tracks played only by this one. |
| 0x231 | NOTE LENGTH | STEP, FIXED, HIT | STEP | How long the generator holds its notes. STEP: the step's GATE. FIXED: LEN, whatever DIV. HIT: until the next hit, so rests let notes ring on. Atlas keeps STEP. |
| 0x232 | NOTE LENGTH LEN | 1/32, 1/16, 1/8, 1/4, 1/2, 1/1 | 1/8 | FIXED's length, from a 32nd note to a bar (1/1), at the tempo the note starts. |
| 0x233 | NOTE LENGTH KEYS | 1 to 4 | 4 | FIXED: how many notes may hold at once. One more ends the note nearest its end; the same pitch again ends the one before. |
| 0x234 | CONSTELLATION Y | 0 to 14 | 7 | The height of the ship's crosshair in the sky: the stars crossing it play as they cross, so it sets where the melody sits. AUTO steers it instead. |
| 0x235 | CONSTELLATION DENS | 0 to 100 % | 50 | How many stars the sky holds: more stars, more notes. |
| 0x236 | CONSTELLATION SIZE | 0 to 7 | 2 | How many rows either side of the crosshair a star still plays in: 0 only its own row, higher a wider melody. The GEN sheet (OCT+ held), Size. |
| 0x237 | CONSTELLATION AUTO | OFF, 1, 2, 3, 4, 5, 6, 7, 8, 9, SEEK | OFF | Who steers the ship: OFF your Y; 1 to 9 a wander of its own, faster further up; SEEK flies towards the next star. Set by a mod (no knob of its own). |
| 0x238 | SERIES START | 0 to 127 | 0 | Where in the infinity series the melody begins: each START a different line, all of them alike at every scale. EVOLVE picks new ones. |
| 0x239 | SERIES INTV | 1 to 7 | 1 | How many degrees of the scale a step of the series is: 1 close and stepwise, higher wider leaps (folded back into two octaves). |
| 0x23A | SERIES LOOP | OFF, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 | OFF | OFF lets the series run on, never repeating; 1 to 16 plays that many steps of it round and round. |
| 0x23E | QUAD RANGE | 3 OCT, 1 OCT, 2 OCT, +-1 | 3 OCT | The octaves the row's notes fold into: 3 OCT as they are, 1 OCT or 2 OCT from the root up, +-1 an octave either side of it. The GEN sheet (OCT+ held), Range. |
| 0x23F | QUAD MOVE 1 | STOP, DOWN, UP, RAND | STOP | How a column of QUAD's grid moves its cells between the four rows: STOP, DOWN (each note a track down, track 4's to track 1), UP, or RAND (a jump of 1 to 3 rows). Rows not in QUAD turn too, keeping the notes. OCT- held on GEN: KNOB 2 the column, KNOB 3 its MOVE. |
| 0x240 | QUAD MOVE 2 | STOP, DOWN, UP, RAND | STOP | How a column of QUAD's grid moves its cells between the four rows: STOP, DOWN (each note a track down, track 4's to track 1), UP, or RAND (a jump of 1 to 3 rows). Rows not in QUAD turn too, keeping the notes. OCT- held on GEN: KNOB 2 the column, KNOB 3 its MOVE. |
| 0x241 | QUAD MOVE 3 | STOP, DOWN, UP, RAND | STOP | How a column of QUAD's grid moves its cells between the four rows: STOP, DOWN (each note a track down, track 4's to track 1), UP, or RAND (a jump of 1 to 3 rows). Rows not in QUAD turn too, keeping the notes. OCT- held on GEN: KNOB 2 the column, KNOB 3 its MOVE. |
| 0x242 | QUAD MOVE 4 | STOP, DOWN, UP, RAND | STOP | How a column of QUAD's grid moves its cells between the four rows: STOP, DOWN (each note a track down, track 4's to track 1), UP, or RAND (a jump of 1 to 3 rows). Rows not in QUAD turn too, keeping the notes. OCT- held on GEN: KNOB 2 the column, KNOB 3 its MOVE. |
| 0x243 | QUAD EVERY 1 | /1, /2, /3, /4, /6, /8, /16 | /4 | How often a column moves: every 1 to 16 steps of the first QUAD track. OCT- held on GEN: KNOB 2 the column, KNOB 4 its EVERY. |
| 0x244 | QUAD EVERY 2 | /1, /2, /3, /4, /6, /8, /16 | /4 | How often a column moves: every 1 to 16 steps of the first QUAD track. OCT- held on GEN: KNOB 2 the column, KNOB 4 its EVERY. |
| 0x245 | QUAD EVERY 3 | /1, /2, /3, /4, /6, /8, /16 | /4 | How often a column moves: every 1 to 16 steps of the first QUAD track. OCT- held on GEN: KNOB 2 the column, KNOB 4 its EVERY. |
| 0x246 | QUAD EVERY 4 | /1, /2, /3, /4, /6, /8, /16 | /4 | How often a column moves: every 1 to 16 steps of the first QUAD track. OCT- held on GEN: KNOB 2 the column, KNOB 4 its EVERY. |
| 0x247 | CONSTELLATION LEN | 1 to 16 | 16 | How many steps the loop of stars is, 1 to 16: with LOCK up a short phrase repeats. OCT- held on GEN, KNOB 3. |
| 0x248 | CONSTELLATION FILL | OFF, ARP | OFF | What the steps between stars play: OFF a rest, ARP the last four stars again, low to high and softer. OCT- held on GEN, KNOB 4. |
| 0x249 | CHORD PROG | POP, 50S, 2-5-1, CIRCLE, TWO, DRONE, WALK, USER | POP | The progression, on the degrees of the track's scale: POP I V vi IV, 50S I vi IV V, 2-5-1 ii V I I, CIRCLE the scale's fifths down, TWO I IV, DRONE I alone; WALK four chords of the track's SEED (ROLL another, EVOLVE changes one); USER up to eight of your own: hold OCT+ on GEN and play their roots on the keys, one key a chord (rounded down to the scale; letting OCT+ go ends it). SCL CHORD's CHRD and VOIC shape the chords (CHRD OFF: triads); hold OCT+ on GEN for OCT, CHRD and VOIC. |
| 0x24A | CHORD BARS | 1/2, 1, 2, 4 | 1 | How long each chord lasts: half a bar, 1, 2 or 4 bars. |
| 0x24B | CHORD HIT | HOLD, BEAT, STEP, OFFBEAT | HOLD | How the chord plays: HOLD once, held to the next chord; BEAT struck again every beat; STEP every step, for the step's GATE; OFFBEAT a short stab on the off-beats, the "and" halfway between the beats (reggae's skank), for the step's GATE: at 1/8 an eighth at most, at 1/16 a sixteenth, and SWING moves it with the step (at 1/8 the "and" is the swung step). |
| 0x24C | CHORD OCT | -2 to 2 | 0 | The chords' register, -2 to +2 octaves from their home around C3. OCT- held on GEN, KNOB 4; SCL CHORD's CHRD and VOIC are on the GEN sheet. |
| 0x24D | ATLAS KICK | 0 to 100 % | 100 | How much of DENSITY the kick gets: 100 all of it, 50 half, 0 none (the kick silent). OCT- held on GEN, KNOB 2. |
| 0x24E | ATLAS SNARE | 0 to 100 % | 100 | How much of DENSITY the snare gets: 100 all of it, 0 none. OCT- held on GEN, KNOB 3. |
| 0x24F | ATLAS HAT | 0 to 100 % | 100 | How much of DENSITY the closed hat gets: 100 all of it, 0 none. OCT- held on GEN, KNOB 4. |
| 0x250 | ATLAS TOMS | 0 to 100 % | 0 | A fourth part, on the tom or the sound PART4 picks: the hits the kick and snare only just miss, so fills follow the groove. 0 none; up, misses further under the line play too (DENSITY scales it). The GEN sheet (OCT+ held), Fourth part; OCT- held on GEN, a black key picks the sound. |
| 0x251 | CHORD USER LEN | 1 to 8 | 4 | How many of USER's chords play, 1 to 8: playing them in on the keys (OCT- held on GEN) sets it; shorten it to leave the last ones out. The GEN sheet, Length. |
| 0x252 | STEP EVOLVE LOCK | 0 to 100 % | 50 | A STEP track's EVOLVE page: how much of the variation stays from round to round. 100 holds it, 0 makes a fresh one each time. |
| 0x253 | STEP EVOLVE EVERY | OFF, 1, 2, 4, 8, 16 | 1 | A STEP track's EVOLVE page: bars between rounds of changes; OFF plays the steps as written. |
| 0x254 | STEP EVOLVE AMOUNT | 0 to 100 % | 0 | A STEP track's EVOLVE page: how many steps play a change, 100 half of them: a note moved by a degree or two or an octave, a step dropped, two notes swapped, a ghost note in a rest. 0 plays the steps as written; the steps themselves never change until REC held keeps the changes. |
| 0x255 | ATLAS PART4 | TOM, CLAP, HATOP, RIM, BELL | TOM | The fourth part's sound: TOM, CLAP, HATOP (the open hat), RIM or BELL, the DRUM lanes besides the kick, snare and closed hat. Hold OCT+ on GEN and press its black key (as on the drum grid: CP OH TM RS CB). |
| 0x256 | TIMING RATCH | x1, x2, x3, x4, x5, x6, x7, x8 | x1 | Plays each step's notes 1 to 8 times in equal parts of the step, a ratchet; x1 plays them once, as written. Each hit gets its share of the step's GATE. Notes NOTE LENGTH holds (FIXED, HIT) and BRANCHed ones play once. |
| 0x257 | TIMING CHANCE | 0 to 100 % | 100 | How often a step ratchets: 100 every step, 0 none. The toss replays with the track's SEED. |
| 0x258 | CHORD BREATH | 0 to 100 % | 0 | With HIT HOLD, the chord breathes: each voice is struck again, softer, on a slow cycle of its own (11 to 19 s), and the same chord again is tied over instead of struck (DRONE: one long chord). BREATH is the chance a voice above the lowest sits a cycle out, one at a time; 0 is off. The GEN sheet (OCT+ held), Breath (USER: Length there). |
| 0x259 | ENERGY ARC | 0 to 100 % | 0 | The project's slow tide, the same for every track: over 3 to 6 minutes it swings from the tracks as set to sparse and back. At the sparse end the generator tracks leave out up to 70 % more of their hits and play up to 16 softer, at the full end they take back their DROP and play up to 12 louder; ARC is how far it swings, round LEVEL, 0 off. It is the cord source ENERGY too (to a filter, a send). |
| 0x25A | ENERGY RESTS | 0 to 100 % | 0 | How often the music rests: 8 to 18 s with the generator tracks silent (a CHORD track's held chord and its BREATH go on), at 100 about every 4 minutes, near 0 about every 20, sooner while ARC's tide is sparse. 0 off. |
| 0x25B | ANSWER VARY | 0 to 100 % | 30 | How much each answer changes the phrase you played: a note moved a degree or two, an octave, left out or swapped with the next. Each answer changes it a little more. 0 answers it as played. |
| 0x25C | ANSWER ECHO | 0 to 100 % | 20 | The chance a note it plays comes back once more, at half its velocity, 3 to 8 steps later, one in four an octave up. 0 off. |
| 0x25D | ANSWER FADE | 0 to 100 % | 33 | How long it remembers: 1 answer at 0 to 16 at 99, each softer than the last, then it rests until you play again. Past 16 it never forgets: the notes play on as four loops of different lengths, SHORT, MID or LONG (LEN), so it never quite repeats. |
| 0x25E | ANSWER ORDER | PLAYED, REVERSE, SHUFFLE | PLAYED | The order of the notes in an answer, over the rhythm you played: PLAYED, REVERSE or SHUFFLE (a new order each answer). OCT- held on GEN, KNOB 3. |
| 0x25F | ANSWER SHIFT | 0 to 7 | 2 | Each answer is moved up or down by up to this many degrees of the scale, a new amount each time. 0 keeps it where you played it. OCT- held on GEN, KNOB 4. |
| 0x260 | ANSWER SPACE | 0 to 100 % | 25 | The rest before each answer: 0 answers straight away, 100 waits 4 bars. A bar without a key ends the phrase you play. The GEN sheet (OCT+ held), Space. |
| 0x261 | ANSWER LEN | SHORT, MID, LONG | MID | With FADE at 100, the lengths of the four loops the notes play on, in steps: SHORT 5 7 11 13, MID 13 17 23 29, LONG 23 29 31 37. FADE turned past 16. |
| 0x262 | TIMING SHAPE | EVEN, ACCEL, DECEL | EVEN | Where a ratchet's hits fall in their step: EVEN in equal parts, ACCEL closer and closer (a roll that speeds up), DECEL further and further apart (a bounce). The track's, for its RATCH and for every step's Ratchet on a STEP track. |
| 0x263 | MIDI OUT CHANNEL | AUTO, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, OFF | AUTO | The MIDI channel this track's notes go out on over USB, so each track can play its own instrument. AUTO follows SYSTEM's ROUT (CH1-4: tracks on 1 to 4), a drum track the DRUM channel; OFF sends none of its notes. Saved with the project. Which notes go out at all is the menu's NOTES OUT: KEYS, ALL or OFF. GLO > MIDI OUT, a knob for each track. |
| 0x264 | KEY LINK | OWN, LINKED | LINKED | LINKED: one key for the whole project. Changing ROOT on any melodic track sets it on the others too, as the scale always is; drum tracks keep theirs. OWN: each track its own ROOT. Turning it on gives every track the selected track's key. SCL's Key link row; on in a new project, saved with it. |
| 0x265 | SOURCE | OFF, T1, T2, T3, T4 | OFF | A STEP pattern for TURING, EUCLID, MARKOV or MORPH to play from: T1 to T4 is that track's pattern playing now (this track's own too), which stays as written. TURING loops its first LEN steps and PROB changes them a step at a time; EUCLID plays its notes in order on its hits; MARKOV learns what follows what in it (LEAP: jumps anywhere); MORPH blends it (A) with a variation of it (B). OFF, or a pattern without notes: the generator's own. ANSWER: the pattern's first 8 notes are its call, read again before each answer; the keys then just play. A track with a generator: the notes it plays are the call. OCT- held on GEN, KNOB 2. |
| 0x266 | KEEP | FREE, RHYTHM, PITCH | FREE | With a SOURCE, what stays as written. FREE: a step's pitch or its hit may change. RHYTHM: only the pitches change (EUCLID: the source's own rhythm, on EUCLID's pitches). PITCH: only the hits come and go (MARKOV: the notes in written order). OCT- held on GEN, KNOB 3. |
| 0x267 | TIMING HUMAN | 0 to 60 ms | 0 | Plays each step a little late, as a player behind the beat: up to this many ms, a new amount each step, never early and never more than a quarter of the step. The same each time the piece plays. 0 on the grid. STEP tracks too (with a step's NUDGE). TWEAK's HUMAN varies velocity and length. |
| 0x268 | ACCUM LIMIT | 1 to 14 | 7 | How far a step's Accumulate takes its notes, in scale degrees from the note as written: 7 is an octave of a 7-note scale. The pattern's sheet (OCT+ held on NOTES). |
| 0x269 | ACCUM MODE | WRAP, PING, CLAMP | WRAP | What a step's Accumulate does at LIMIT. WRAP: back to the note as written, and on again. PING: turns round and comes back. CLAMP: stays there. PLAY and a new pattern start it again. |
| 0x26A | GRAVITY | 0 to 100 % | 0 | The pull home. With EVERY on, this share of EVOLVE's rounds moves the generator back toward its settings as you last set them, instead of changing it: it drifts, then comes home. With a SOURCE, this share of the steps plays the source as written at its place. 0: none. OCT- held on EVOLVE or GEN, KNOB 4. |
| 0x26B | EVOLVE HOW | DRIFT, ADORN, SERIAL | DRIFT | What STEP EVOLVE's rounds do to the steps. DRIFT: moves by scale degrees, octave jumps, drops, swaps and ghost notes. ADORN, ornaments that only add: ratchets, octave jumps, neighbour notes, passing notes in the rests. SERIAL: the whole phrase at once: backwards, upside down, rotated or half of it at half speed; AMOUNT is the chance of one a round, LOCK of it staying. OCT- held on EVOLVE, KNOB 4. |
| 0x26C | ENERGY LEVEL | -100 to 100 | 0 | The project's energy, played by hand: ARC's tide swings round it. Up, the generator tracks take back their DROP and play up to 12 louder; down, they leave out hits and play softer. EVOLVE changes less when low, more when high. 0: as set. |
| 0x26D | ENERGY DEPTH | 0 to 100 % | 100 | How much ENERGY (its LEVEL, ARC's tide, RESTS) moves this track. 100 all of it, 0 an anchor that plays as set whatever the energy. OCT- held on ENERGY, KNOB 1. |
| 0x26E | FORM DEVELOP | 0 to 100 % | 0 | How far a section varies each time FORM comes back to it: it stays recognisable but is never quite the same. The section itself is kept, so the next return starts from it again. GRAVITY is the chance a return stays exact. 0: every return exact. OCT- held on FORM, KNOB 1. |
| 0x26F | MIDI OUT TRANSPOSE | -48 to 48 st | 0 | Moves this track's notes up or down on their way out over MIDI, in semitones (12 an octave), for an instrument or a MIDI-to-CV module that plays them too high or too low. The track's own sound is unchanged; a note moved past MIDI's range is not sent. 0: as played. Saved with the project. OCT- held on GLO > MIDI OUT, a knob for each track. |
| 0x270 | TUNING | OFF, USR1, USR2, USR3, USR4 | OFF | A tuning of your own for the whole project, as the scale is: USR1 to USR4 play the Scala (.scl) files sent from the web editor (PROJECTS, Tunings), saved with the project. The keys, the generators, QNT and chords play its degrees; ROOT and TRN move it. OFF, or an empty slot, plays SCL's scale. SCL > SCALES, the USER family. |
| 0x271 | SOUND | ON, OFF | ON | OFF: this track plays no sound of its own, so it takes none of the FM-1's voices. Its notes still go out over MIDI on its channel, and its GATE, NOTE and CV still move the mods: a track for an outside synth, or a lane that only modulates the others. ON: as always. Saved with the project. GLO > MIDI OUT's sheet (OCT+ held), Sound T1 to T4. |
| 0x272 | CHAIN FORM | OFF, AB, ABAC, ABCB, AABA, ABCA, AABC | OFF | One FORM for the whole project: on each phrase line it moves to its next letter, and every generator track that FOLLOWs moves with it to its own A, B or C, so the tracks' sections line up. OFF: each track as set (its own FORM, or none). GLO > CHAIN, KNOB 1. |
| 0x273 | CHAIN PHRASE | 4, 8, 16 | 8 | How long a phrase of the CHAIN is: 4, 8 or 16 bars, counted from PLAY. The letters change on its lines. GLO > CHAIN, KNOB 2. |
| 0x274 | CHAIN WANDER | 0 to 100 % | 0 | 0 plays the CHAIN's FORM as written, round and round. Up, the next letter is more often chosen than read: A comes back the longer the music is away, a new letter only after a return to A, and each phrase away makes the next new section more different. The same piece from PLAY each time. GLO > CHAIN, KNOB 3. |
| 0x275 | CHAIN REPEAT 1 | 1 to 8 x | 1 | How many phrases the CHAIN's first letter plays, 1 to 8. GLO > CHAIN, OCT+ held, KNOB 1. |
| 0x276 | CHAIN REPEAT 2 | 1 to 8 x | 1 | How many phrases the CHAIN's second letter plays, 1 to 8. GLO > CHAIN, OCT+ held, KNOB 2. |
| 0x277 | CHAIN REPEAT 3 | 1 to 8 x | 1 | How many phrases the CHAIN's third letter plays, 1 to 8. GLO > CHAIN, OCT+ held, KNOB 3. |
| 0x278 | CHAIN REPEAT 4 | 1 to 8 x | 1 | How many phrases the CHAIN's fourth letter plays, 1 to 8. GLO > CHAIN, OCT+ held, KNOB 4. |
| 0x279 | FORM FOLLOW | FOLLOW, OWN | FOLLOW | FOLLOW: this track's sections change with the CHAIN's letters, on its phrase lines (its EVERY waits meanwhile). OWN: it keeps its own FORM. Not set, a track follows while its own FORM is OFF. Only while the CHAIN's FORM is on. OCT- held on FORM, KNOB 2. |
| 0x27A | FORM SKIP | 0 to 100 % | 0 | The chance this track stays in its section one phrase longer where the CHAIN moves on, then catches up; never twice running. 0: always with the CHAIN. OCT- held on FORM, KNOB 3. |
| 0x27B | ENTRIES | 0 to 100 % | 0 | 0: every track plays as set. Up, tracks drop out and come back on the CHAIN's phrase lines: how many may be out at once, up to all but one. Exits come where a section begins, melody first; entries one a phrase, bass first. Above 50 the phrase before a return to A thins to the anchors and A comes back full. A muted track stays muted; its key on GLO brings an out track back. GLO > ENTRIES, KNOB 1. |
| 0x27C | ENTRIES DWELL | 1 to 8 ph | 2 | The phrases a track stays in or out at least before Entries moves it again, 1 to 8. GLO > ENTRIES, KNOB 2. |
| 0x27D | ENTRIES STYLE | CUT, FADE | FADE | How a track leaves and comes back: CUT at once on the phrase line, or FADE over FADE bars from it. GLO > ENTRIES, KNOB 3. |
| 0x27E | ENTRIES FADE | 1 to 4 bar | 2 | The bars a FADE takes, 1 to 4. GLO > ENTRIES, KNOB 4. |
| 0x27F | ENTRIES ROLE | AUTO, BASS, RHYTHM, HARMONY, MELODY, ANCHOR | AUTO | What the track is to Entries: BASS comes in first and goes last, then RHYTHM, HARMONY, MELODY (a melody only over a bass or harmony). ANCHOR is never taken out. AUTO: CHORD is harmony, ATLAS or a drum sound rhythm, notes mostly below C3 bass, else melody. GLO > ENTRIES with OCT- held, or OCT- held on FORM, KNOB 4. |
| 0x280 | PIECE LENGTH | FREE, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 45, 60, 75, 90, 120, 180, 240 | FREE | FREE: the music plays on unplanned. A length, 1 minute to 4 hours: the piece is planned to it from PLAY, its SHAPE steering ENERGY (ARC how far), ENTRIES (how many tracks play) and the CHAIN (further away near the top, A in the last part). At the tempo now: a faster tempo, more bars. GLO > PIECE, KNOB 1. |
| 0x281 | PIECE SHAPE | ARCH, RISE, FALL, PLATEAU, VALLEY, WAVES, BUILDS, STEPS, FLAT, USER | ARCH | The piece's arc from sparse to full: ARCH one climax at PEAK; RISE a build to the end; FALL starts full and dissolves; PLATEAU a long hold round PEAK; VALLEY full with a breakdown at PEAK; WAVES COUNT swells; BUILDS COUNT rises each dropped at once; STEPS terraces; FLAT steady; USER your eight levels. All but RISE and FALL begin and end sparse. GLO > PIECE, KNOB 2. |
| 0x282 | PIECE PEAK | 0 to 100 % | 62 | Where the climax of ARCH, PLATEAU, VALLEY or STEPS sits, as a share of the length (62 %: the golden section). GLO > PIECE, KNOB 3. |
| 0x283 | PIECE COUNT | 2 to 8  | 3 | How many swells (WAVES), builds (BUILDS) or terraces (STEPS), 2 to 8. GLO > PIECE, KNOB 3. |
| 0x284 | PIECE END | STOP, LOOP, NEW, FREE | STOP | At the end of the LENGTH: STOP lets the tracks fade and the tails ring, then stops; LOOP plays the same piece again; NEW draws new SEEDs and plays a new piece; FREE plays on unplanned. GLO > PIECE, KNOB 4. |
| 0x285 | PIECE USER 1 | 0 to 100 % | 10 | SHAPE USER: the level at the piece's start. GLO > PIECE, OCT- held: KNOB 1 point 1, KNOB 2 its level. |
| 0x286 | PIECE USER 2 | 0 to 100 % | 30 | SHAPE USER: the level a seventh in. GLO > PIECE, OCT- held: KNOB 1 point 2, KNOB 2 its level. |
| 0x287 | PIECE USER 3 | 0 to 100 % | 55 | SHAPE USER: the level two sevenths in. GLO > PIECE, OCT- held: KNOB 1 point 3, KNOB 2 its level. |
| 0x288 | PIECE USER 4 | 0 to 100 % | 80 | SHAPE USER: the level three sevenths in. GLO > PIECE, OCT- held: KNOB 1 point 4, KNOB 2 its level. |
| 0x289 | PIECE USER 5 | 0 to 100 % | 100 | SHAPE USER: the level four sevenths in. GLO > PIECE, OCT- held: KNOB 1 point 5, KNOB 2 its level. |
| 0x28A | PIECE USER 6 | 0 to 100 % | 75 | SHAPE USER: the level five sevenths in. GLO > PIECE, OCT- held: KNOB 1 point 6, KNOB 2 its level. |
| 0x28B | PIECE USER 7 | 0 to 100 % | 40 | SHAPE USER: the level six sevenths in. GLO > PIECE, OCT- held: KNOB 1 point 7, KNOB 2 its level. |
| 0x28C | PIECE USER 8 | 0 to 100 % | 10 | SHAPE USER: the level at the end. GLO > PIECE, OCT- held: KNOB 1 point 8, KNOB 2 its level. |
| 0x28D | MACRO 1 | 0 to 100 % | 0 | A hand control for the whole project, 0 to 100 %. On its own it does nothing: pick MACRO as a mod's SOURCE (MODS) and send it anywhere, to as many places as you like. GLO > PERF, KNOB 1 to 4. |
| 0x28E | MACRO 2 | 0 to 100 % | 0 | A hand control for the whole project, 0 to 100 %. On its own it does nothing: pick MACRO as a mod's SOURCE (MODS) and send it anywhere, to as many places as you like. GLO > PERF, KNOB 1 to 4. |
| 0x28F | MACRO 3 | 0 to 100 % | 0 | A hand control for the whole project, 0 to 100 %. On its own it does nothing: pick MACRO as a mod's SOURCE (MODS) and send it anywhere, to as many places as you like. GLO > PERF, KNOB 1 to 4. |
| 0x290 | MACRO 4 | 0 to 100 % | 0 | A hand control for the whole project, 0 to 100 %. On its own it does nothing: pick MACRO as a mod's SOURCE (MODS) and send it anywhere, to as many places as you like. GLO > PERF, KNOB 1 to 4. |
| 0x291 | FOLLOW HOLD | OFF, T1, T2, T3, T4 | OFF | Sample and hold for FOLLOW: the followed track's note is taken only when the HOLD track plays, and this track keeps playing it in its own rhythm until the next one. OFF: a new note on each of its own hits. TWEAK, OCT- held, KNOB 4. |
| 0x292 | KEY B | -12 to 12 semitones | 0 | A key change for the CHAIN's section: while the project plays it, every track that FOLLOWs the CHAIN moves its notes this many semitones, scale and all (+5 a fourth up, +7 a fifth, -2 a tone down). STEP tracks move too; drum tracks and OWN tracks keep theirs. A is the key as set. 0: no change. SCL's Key in B and Key in C rows; saved with the project. |
| 0x293 | KEY C | -12 to 12 semitones | 0 | A key change for the CHAIN's section: while the project plays it, every track that FOLLOWs the CHAIN moves its notes this many semitones, scale and all (+5 a fourth up, +7 a fifth, -2 a tone down). STEP tracks move too; drum tracks and OWN tracks keep theirs. A is the key as set. 0: no change. SCL's Key in B and Key in C rows; saved with the project. |
| 0x294 | TIMING SPLIT | 0 to 100 % | 0 | The chance a step plays at half its division: 1/8 to 1/16. HOW says what it does. Never the bar's first step, nor a step that ratchets or ties. The same each time the piece plays; ROLL gives new tosses. 0 off. OCT- held on TIMING, KNOB 1. |
| 0x295 | TIMING HOW | DOUBLE, PUSH, MIX | DOUBLE | What a SPLIT step does. DOUBLE: the note, then a neighbouring note of the scale halfway through the step (a drum: the same hit). PUSH: the note halfway through the step, late only. MIX: tosses for one or the other. OCT- held on TIMING, KNOB 2. |
| 0x296 | FOLLOW HOW | SAME, CHORD, CONTRA, BASS, CANON1, CANON2, CANON3, CANON4, CANON5, CANON6, CANON7, CANON8 | SAME | How FOLLOW turns the other track's note. SAME: that note, SHIFT degrees away. CHORD: the third, fifth or seventh of the chord on it, whichever is nearest this track's last note. CONTRA: moves the other way, mirrored around the first note followed. BASS: an octave or two down, on beats 1 and 3 only. CANON1..8: its note that many notes back. TWEAK, OCT- held, KNOB 2. |
| 0x297 | FOLLOW RHYTHM | OWN, SOURCE, BOTH | OWN | When a following track plays. OWN: on its own hits. SOURCE: only when the followed track has played a note, so the two move together (a lower track number lands on the same step). BOTH: its own hits and the other's. TWEAK, OCT+ held, KNOB 3. |
| 0x298 | PAT GEN | OFF, ON | OFF | ON: each of the track's 8 patterns keeps its own MODE, the generator's knobs, SEED and EVOLVE's EVERY, AMOUNT and LOCK, so switching pattern (by hand, PATTERNS, SONG) switches the generator, which starts again from the pattern's SEED. Turning it ON gives all 8 the settings as they are; COPY copies them with the steps. FORM, TWEAK and the rest stay the track's. OFF: the generator's settings are the track's. EVOLVE, OCT- held, KNOB 3. |
| 0x299 | MIDI OUT DRUM MAP | GM, ROW | GM | The notes a drum track or ATLAS sends out over MIDI. GM: General MIDI's drum notes (kick 36, snare 38, closed hat 42). ROW: one note a drum in a row from 36, in the order of a drum machine's tracks such as the Analog Rytm's: kick, snare, rim, clap, -, low tom, -, -, closed hat, open hat, -, bell. TRANSPOSE moves the row onto the machine's first note. MIDI OUT's sheet (OCT+ held), Drum map T1 to T4. |
| 0x29A | ENTRIES ORDER | AUTO, 1ST, 2ND, 3RD, 4TH | AUTO | When the track comes in: 1ST to 4TH, one a phrase in that order, and out the other way round (last in, first out). AUTO: by ROLE, after the numbered tracks. ANCHOR still never goes out; only AUTO tracks wait for a bass or harmony. GLO > ENTRIES' sheet (OCT+ held), Order T1 to T4. |
| 0x29B | PATTERN DIR | FWD, BACK, PEND, FOLD, RAND, DRUNK | FWD | The order the STEP pattern plays its steps in, each pattern its own. FWD: 1 to the end. BACK: the end to 1. PEND: there and back, the ends once. FOLD: from the ends inwards, 1, the end, 2, .. RAND: any step. DRUNK: a step on or back at random. A pass is still LEN steps, so conditions, Accumulate and pattern changes keep the bar. The pattern's sheet (OCT+ held), Direction. |
| 0x29C | CHORD STRUM | 0 to 60 ms | 0 | A chord's notes start this many ms apart, as a strummed chord (in all at most half its length). OFF: together. OCT- held on GEN, KNOB 2. |
| 0x29D | CHORD DIR | UP, DOWN, ALT | UP | A strum's first note: UP the lowest, DOWN the highest, ALT turns round each strike. OCT- held on GEN, KNOB 3. |
| 0x29E | BUS A MODE | SUM, MAX, MIN, DIFF | SUM | How the mods into BUS A combine. SUM: added, -1..1. MAX: the highest (two gates: either, OR). MIN: the lowest (both, AND). DIFF: how far apart the first two are (two gates: one or the other, XOR). The source BUS A reads the result a block later. MODS' sheet (OCT+ held). |
| 0x29F | BUS A LAG | OFF, 10 ms, 20 ms, 50 ms, 100 ms, 200 ms, 500 ms, 1 s, 2 s, 5 s, 10 s, 20 s | OFF | A slew on BUS A: the time a whole move from 0 to 1 takes, up or down. A gate into a lagged bus rises and falls as an envelope. OFF: none. MODS' sheet (OCT+ held). |
| 0x2A0 | BUS B MODE | SUM, MAX, MIN, DIFF | SUM | How the mods into BUS B combine. SUM: added, -1..1. MAX: the highest (two gates: either, OR). MIN: the lowest (both, AND). DIFF: how far apart the first two are (two gates: one or the other, XOR). The source BUS B reads the result a block later. MODS' sheet (OCT+ held). |
| 0x2A1 | BUS B LAG | OFF, 10 ms, 20 ms, 50 ms, 100 ms, 200 ms, 500 ms, 1 s, 2 s, 5 s, 10 s, 20 s | OFF | A slew on BUS B: the time a whole move from 0 to 1 takes, up or down. A gate into a lagged bus rises and falls as an envelope. OFF: none. MODS' sheet (OCT+ held). |
| 0x2A2 | MOD DEPTH | 0 to 100 % | 100 | How much of every mod's move plays, 0 to 100 %: the moving part (AMOUNT times its source) of each mod, FILL, TEMPO and CC ones too; never an OFFSET, so fixed mods, a CC's base and a GATE's threshold stay. 0: the patch dry. A mod onto MOD DEP (a MACRO) rides it. A GATE onto FILL at 100 % stops holding FILL under 50 %. GLO > PERF's sheet (OCT+ held). |

## Mods

A mod (the MODS page, under LFO) moves a destination on any track by a source on any track: by its AMOUNT (-100 to 100 %, at a full source the destination's whole range; PITCH: an octave) plus its OFFSET. A mod's direction is shown under KNOB 1 and in its bar: +- moves both ways from the knob (the bar as far down as up), + one way only. A bipolar source (LFO, KEY, RAND, S&H, SLEW, NOTE, DRIFT, TIDE, SWARM, ENERGY) is +-; OCT+ on MODS makes it +, the source folded to 0 to 1, as ENV moves. The LFO page's POL (BI, UNI) is not the mod's: it shapes only that page's own depths (LFO PIT, FLT, SHP, AMP), and a mod from the LFO is +- or + by its own setting, whatever POL is. ENV, VEL, KEY and RAND on a voice destination (PITCH, CUTOFF, SHAPE, GAIN) of their own track follow each note, as a chord's; elsewhere the latest note. Room for 32, 16 in use. They are the project's, saved with it. Melodee's MOD page is gone: the four routes a project or a sound was saved with become mods on their track, the same move (a sound's go when another sound is loaded on the track, unless one was edited).

| Source | |
| --- | --- |
| LFO | the track's LFO, both ways whatever its POL (the mod's +- or + decides) |
| ENV | the envelope of the track's latest note |
| VEL | the latest note's velocity |
| KEY | the latest note's pitch (C4 = 0) |
| RAND | a new random value at each note |
| MODW | MIDI mod wheel (CC1) |
| AT | MIDI channel aftertouch |
| EXPR | MIDI expression (CC11) |
| S&H | a random value held for each cycle of the track's LFO |
| SLEW | S&H, gliding from one value to the next |
| GATE | 1 while the track's generator plays a note |
| NOTE | the generator's last note (C4 = 0) |
| CV | the generator's CV: Turing its register as a number, Euclid and Morph a ramp over their cycle, Markov the degree it moved to |
| DRIFT | a slow wander of the track's own, a new point every 12 s or so, eased between |
| TIDE 1 | TIDE's slowest chaotic circuit, about 17 min a turn (ORGANIC page, SPEED 50) |
| TIDE 3 | TIDE's fastest chaotic circuit, about 1 min a turn |
| TIDE 2 | TIDE's middle chaotic circuit, about 3 min a turn |
| SWARM 1 | a free LFO, 64 s a turn (ORGANIC page, RATE 50) |
| SWARM 2 | a free LFO, 40 s a turn |
| SWARM 3 | a free LFO, 25 s a turn |
| SWARM 4 | a free LFO, 15.6 s a turn |
| SWARM 5 | a free LFO, 9.8 s a turn |
| SWARM 6 | a free LFO, 6.1 s a turn |
| SWARM 7 | a free LFO, 3.8 s a turn |
| SWARM 8 | a free LFO, 2.4 s a turn |
| ENERGY | the project's slow tide (ENERGY ARC), every track's the same: -100 % sparse .. 100 % full |
| MACRO 1 | GLO > PERF's KNOB 1, the project's: 0 .. 100 % |
| MACRO 2 | GLO > PERF's KNOB 2, the project's: 0 .. 100 % |
| MACRO 3 | GLO > PERF's KNOB 3, the project's: 0 .. 100 % |
| MACRO 4 | GLO > PERF's KNOB 4, the project's: 0 .. 100 % |
| BUS A | the project's MIX bus A: its mods combined by its MODE, slewed by its LAG (MODS' sheet), one block late: -100 .. 100 % |
| BUS B | the project's MIX bus B, as BUS A |
| EOC | the track's LFO cycle start as a gate: 1 from each wrap of the LFO until an eighth of the cycle has passed, else 0 (a TRIG or PLAY phase set is not a wrap). Onto FILL, a GATE mod or a bus it marks each cycle |

| Destination | Moved at | What it moves |
| --- | --- | --- |
| PITCH | each voice | the pitch of each voice: 100 % at a full source is an octave |
| CUTOFF | each voice | the filter (or the engine's brightness) of each voice |
| SHAPE | each voice | the engine's SHAPE of each voice, as ENV DEST SHP moves it |
| GAIN | each voice | each voice's loudness: up opens it from silence, down closes it |
| LEVEL | the audio block | the track's level in the mix |
| PAN | the audio block | the track's place left to right |
| SPREAD | the audio block | the stereo spread of the track's voices |
| ATTACK | the audio block | the envelope's attack time |
| DECAY | the audio block | the envelope's decay time |
| SUSTAIN | the audio block | the envelope's sustain level |
| RELEASE | the audio block | the envelope's release time |
| ENV FLT | the audio block | how far the envelope moves the filter |
| ENV PIT | the audio block | how far the envelope moves the pitch |
| ENV SHP | the audio block | how far the envelope moves SHAPE |
| LFO RTE | the audio block | the track LFO's speed |
| LFO DEP | the LFO's depth | the track LFO's depth, wherever it goes |
| LFO PIT | the audio block | how far the LFO moves the pitch (vibrato) |
| LFO FLT | the audio block | how far the LFO moves the filter |
| LFO SHP | the audio block | how far the LFO moves SHAPE |
| LFO AMP | the audio block | how far the LFO moves the level (tremolo) |
| FX SEND | the audio block | how much of the track goes to the effects |
| E1 | the audio block | the engine's first parameter (its name: the track's engine) |
| E2 | the audio block | the engine's second parameter |
| E3 | the audio block | the engine's third parameter |
| E4 | the audio block | the engine's fourth parameter |
| E5 | the audio block | the engine's fifth parameter |
| E6 | the audio block | the engine's sixth parameter |
| E7 | the audio block | the engine's seventh parameter |
| E8 | the audio block | the engine's eighth parameter |
| T PROB | the generator's step | Turing's PROB: how often its loop changes |
| T LEN | the generator's step | Turing's loop length |
| E PULSE | the generator's step | Euclid's hits: more or fewer |
| E ROT | the generator's step | Euclid's rotation |
| M LEAP | the generator's step | Markov's LEAP: how far its notes jump |
| M DENS | the generator's step | Markov's density: how many steps play |
| MP POS | the generator's step | Morph's blend from rhythm A to B |
| SHIFT | the generator's step | TWEAK's SHIFT: transposes the generator's notes |
| DROP | the generator's step | TWEAK's DROP: the chance a hit is left out |
| HUMAN | the generator's step | TWEAK's HUMAN: velocity and gate variation |
| EV AMT | the generator's step | EVOLVE's AMOUNT: how much each mutation changes |
| CONTR | the generator's step | FORM's CONTRAST: how different a new section is |
| AT X | the generator's step | Atlas' X: across its map of drum patterns |
| AT Y | the generator's step | Atlas' Y: down its map of drum patterns |
| AT DENS | the generator's step | Atlas' density: how full its parts are |
| B SPEED | the generator's step | Bounce's speed: how far the balls move a step |
| B ANGLE | the generator's step | Bounce's angle: the balls' paths (FALL: the wind) |
| A PROB | the generator's step | Awake's PROB: the chance a note plays |
| BRANCH | the generator's step | TWEAK 2's BRANCH: the chance a hit plays on track TO |
| Q PROB | the generator's step | Quad's PROB: the chance a cell plays |
| C Y | the generator's step | Constellation's Y: the crosshair's height (AUTO OFF) |
| C SIZE | the generator's step | Constellation's SIZE: how far from the crosshair a star is caught |
| C DENS | the generator's step | Constellation's DENS: how many stars the sky holds |
| S START | the generator's step | Series' START: where in the infinity series it plays |
| C AUTO | the generator's step | Constellation's AUTO: who steers the ship (OFF, a wander, SEEK) |
| CH PROG | the generator's step | Chord's PROG: which progression its chords follow |
| AT KICK | the generator's step | Atlas' KICK: how much of its density the kick gets |
| AT SNR | the generator's step | Atlas' SNR: how much of its density the snare gets |
| AT HAT | the generator's step | Atlas' HAT: how much of its density the hat gets |
| AT TOMS | the generator's step | Atlas' TOMS: how many of the near misses play the tom |
| AN VARY | the generator's step | Answer's VARY: how much each answer changes the phrase |
| H TIME | the generator's step | TIMING's HUMAN: how late a step may play |
| FX1 MIX | the audio block | the track's FX 1: how much of the effect is heard |
| FX1 RTE | the audio block | the track's FX 1 RATE (ECHO: its TIME, gliding; ROOM, REVERB: SIZE) |
| FX1 CHR | the audio block | the track's FX 1 fourth knob (DEPTH, REPEAT, DECAY ..: the effect's) |
| FX2 MIX | the audio block | the track's FX 2: how much of the effect is heard |
| FX2 RTE | the audio block | the track's FX 2 RATE (ECHO: its TIME, gliding; ROOM, REVERB: SIZE) |
| FX2 CHR | the audio block | the track's FX 2 fourth knob (DEPTH, REPEAT, DECAY ..: the effect's) |
| FX3 MIX | the audio block | the track's FX 3: how much of the effect is heard |
| FX3 RTE | the audio block | the track's FX 3 RATE (ECHO: its TIME, gliding; ROOM, REVERB: SIZE) |
| FX3 CHR | the audio block | the track's FX 3 fourth knob (DEPTH, REPEAT, DECAY ..: the effect's) |
| SEND 2 | the audio block | how much of the track goes to SEND FX 2 (FX MODE SPLIT) |
| SEND 3 | the audio block | how much of the track goes to SEND FX 3 (FX MODE SPLIT) |
| SF1 MIX | the send effects' block (every track's) | SEND FX 1's MIX, for every track (the project's) |
| SF1 RTE | the send effects' block (every track's) | SEND FX 1's RATE (ECHO: its TIME, gliding; ROOM, REVERB: SIZE) |
| SF1 CHR | the send effects' block (every track's) | SEND FX 1's fourth knob (DEPTH, REPEAT, DECAY ..: the effect's) |
| SF2 MIX | the send effects' block (every track's) | SEND FX 2's MIX, for every track (the project's) |
| SF2 RTE | the send effects' block (every track's) | SEND FX 2's RATE (ECHO: its TIME, gliding; ROOM, REVERB: SIZE) |
| SF2 CHR | the send effects' block (every track's) | SEND FX 2's fourth knob (DEPTH, REPEAT, DECAY ..: the effect's) |
| SF3 MIX | the send effects' block (every track's) | SEND FX 3's MIX, for every track (the project's) |
| SF3 RTE | the send effects' block (every track's) | SEND FX 3's RATE (ECHO: its TIME, gliding; ROOM, REVERB: SIZE) |
| SF3 CHR | the send effects' block (every track's) | SEND FX 3's fourth knob (DEPTH, REPEAT, DECAY ..: the effect's) |
| RETURN | the send effects' block (every track's) | the send effects' RETURN: how loud they come back, for every track |
| M1 AMT | another mod's move, each block | MOD 1's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M2 AMT | another mod's move, each block | MOD 2's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M3 AMT | another mod's move, each block | MOD 3's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M4 AMT | another mod's move, each block | MOD 4's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M5 AMT | another mod's move, each block | MOD 5's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M6 AMT | another mod's move, each block | MOD 6's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M7 AMT | another mod's move, each block | MOD 7's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M8 AMT | another mod's move, each block | MOD 8's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M9 AMT | another mod's move, each block | MOD 9's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M10 AMT | another mod's move, each block | MOD 10's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M11 AMT | another mod's move, each block | MOD 11's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M12 AMT | another mod's move, each block | MOD 12's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M13 AMT | another mod's move, each block | MOD 13's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M14 AMT | another mod's move, each block | MOD 14's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M15 AMT | another mod's move, each block | MOD 15's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| M16 AMT | another mod's move, each block | MOD 16's AMOUNT (a fixed mod's OFFSET), turned by another mod |
| FILL | the project's FILL, each block | the project's FILL (steps set to FILL / NOT FILL), held while the move is past half |
| TEMPO | the project's tempo on CLK INT, each block | the project's tempo on CLK INT: a full move up doubles it, down halves it (BPM stays as set) |
| CC | a MIDI CC out, each block | a MIDI CC out on the track's MIDI OUT channel, its number set with OCT- held and KNOB 3 |
| SPLIT | the generator's step | TIMING's SPLIT: the chance a step plays at half its division |
| DIR | the generator's step | the STEP pattern's DIR, stepped: FWD BACK PEND FOLD RAND DRUNK from the pattern's own |
| BUS A | the project's MIX bus, each block | the project's MIX bus A: its mods combined by its MODE (SUM MAX MIN DIFF), slewed by its LAG, read back by the source BUS A |
| BUS B | the project's MIX bus, each block | the project's MIX bus B: its mods combined by its MODE (SUM MAX MIN DIFF), slewed by its LAG, read back by the source BUS B |
| MOD DEP | the project's MOD DEPTH, each block | the project's MOD DEPTH (GLO > PERF's sheet): how much of every mod's move plays, 0..100 %; a MACRO onto it at -100 % turns the patch dry |
| PLAY | whether the track is heard, each block | whether the track's notes sound: while its mods together are at half or more; under it the track plays on unheard (it still leads, as MUTE) |
| CLOCK | the generator's clock, on the source's edges | steps the track's generator on each edge of the source, in place of its DIV (a GATE cord: OFFSET the threshold, a GATE source counts hits); AMOUNT is the division, 100 % every edge, 50 % every second, 33 % every third; negative: the gate closing |
