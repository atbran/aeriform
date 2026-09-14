# AERIFORM: User Feature Request Review & Sound Design Roadmap

This document captures prospective feature enhancements, physical modeling expansions, workflow ergonomics, and sound design tools for AERIFORM.

---

## 1. Organic Performance & Humanization

Because AERIFORM is an **oscillator-free physical-modelling synthesizer**, its perceived realism and musicality depend heavily on organic performance nuances.

### 1.1 Acoustic Round-Robin & Strike Jitter
- **Concept**: In real physical instruments, striking the same string, bar, or membrane twice never excites the identical mechanical point with identical force.
- **Implementation**:
  - Introduce subtle, deterministic or seeded micro-variations to strike location ($\pm 2\%$), mallet hardness ($\pm 3\%$), bow friction, and breath onset turbulence on consecutive note attacks.
  - Prevents the "machine-gun" effect during rapid 16th-note plucks, mallet runs, or repetitive percussion without requiring manual velocity automation.

### 1.2 Strum & Pluck Arpeggiator
- **Concept**: A specialized physical-model arpeggiator tailored for plucked strings, harps, and mallet percussion.
- **Features**:
  - Directional strumming (up-strum vs. down-strum) with adjustable onset stagger delay (0–100 ms).
  - Velocity decay contour across strummed strings.
  - Automatic damping / palm-muting probability for held chords.

### 1.3 Musical Scale & Key Quantizer
- **Concept**: Pitch quantization for modal resonators and pitch modulation paths.
- **Features**:
  - Musical scale selection (Pentatonic, Dorian, Mixolydian, Harmonic Minor, Hungarian Gypsy, Raga, Gamelan Pelog/Slendro).
  - Quantizes incoming MIDI notes or internal pitch modulation sources (LFOs, randomizer, glide), enabling expressive modal improvisation and melodic sequencing.

### 1.4 Scala (`.scl` / `.kbm`) Microtonal Tuning Support
- **Concept**: Physical modeling synthesizers are heavily utilized in experimental, non-Western, and historical acoustic music.
- **Features**:
  - Full support for standard `.scl` tuning files and `.kbm` keyboard mapping files.
  - Remaps the resonator network fundamental tuning frequencies continuously while maintaining physical waveguide acoustic ratios.

---

## 2. Physical Modeling DSP Deepening

### 2.1 Custom Excitation Impulse / Transient Loader
- **Concept**: Allow users to drag-and-drop short recorded audio samples (finger taps, pencil clicks, bow scrapes, chime strikes, vocal pops) into Exciter 1 or 2.
- **Acoustic Value**: Exciting AERIFORM's tuned resonators (pipes, plates, tubes, strings) with real-world recorded physical strikes bridges sample realism with acoustic synthesis flexibility.

### 2.2 Acoustic Soundboard / Body Convolution
- **Concept**: A zero-latency acoustic cabinet/body stage simulating physical resonant chambers.
- **Models**:
  - Acoustic Guitar Body, Cello/Violin Cavity, Grand Piano Soundboard, Marimba Resonator Tubes, Metallic Gong Plate.
  - Adds acoustic radiation air volume and realistic body resonances to synthesized strings and pipes.

### 2.3 String Tension Modulation & Detune Non-Linearity
- **Concept**: In physical strings, high strike velocity momentarily increases string tension, causing the initial pitch attack to ring slightly sharp before settling to the fundamental.
- **Acoustic Value**: Produces the characteristic acoustic "twang" of acoustic guitars, harps, slap bass, and ethnic plucked lutes.

---

## 3. Visual Modulation & Modulation Depth Rings

### 3.1 Dynamic Knob Modulation Range Arcs
- **Concept**: Visual feedback directly on parameter knobs showing dynamic modulation in real time (similar to Serum, Pigments, and Massive).
- **Features**:
  - A glowing colored arc (teal/copper) around the knob diameter representing the min/max excursion of active modulators (LFO, ADSR, Macro, Velocity).
  - A small real-time indicator tracking the live modulated instantaneous value.

### 3.2 Interactive Envelope Curve / Tension Shaping
- **Concept**: Non-linear curvature control for Attack, Decay, and Release segments of the Amplitude, Modulation, and Breath envelopes.
- **Features**:
  - Curve dragging handles on the graphical ADSR display to transition smoothly between logarithmic, linear, and exponential slopes.
  - Essential for snappy percussive plucks (steep exponential decay) and swelling ambient pads (smooth s-curve attack).

### 3.3 Drag-and-Drop Modulation Assignment
- **Concept**: Quick visual modulation routing without opening the matrix tab.
- **Workflow**:
  - Drag an assignment puck from an LFO, Envelope, or Macro badge and drop it onto any target knob to immediately set modulation depth.

---

## 4. Patch Workflow & Sound Exploration

Building upon the v3.3.0 table preset browser and category system:

### 4.1 Section-Locked Randomizer ("Padlock" Dice Roll)
- **Concept**: Allow targeted randomization by locking specific modules.
- **Workflow**:
  - Small padlock toggle buttons beside major panels: *Lock Pitch/Tuning*, *Lock Exciters*, *Lock Resonators*, *Lock Effects*, *Lock Modulation*.
  - Rolling the randomizer dice generates fresh textures while keeping the core melodic tuning or sound architecture stable.

### 4.2 Preset Author, Description & Playing Tips Metadata
- **Concept**: Store sound design notes and performance tips directly inside `.aerpreset` XML files.
- **Features**:
  - Author name, creation date, and freeform text description field.
  - Displays performance tips in the preset browser (e.g. *"Mod Wheel controls breath filter cutoff; Macro 1 blends between glass and metallic resonators"*).

### 4.3 One-Click Bank Import / Export (`.aerbank` / `.zip`)
- **Concept**: Streamlined sharing of sound packs and user collections.
- **Features**:
  - "Export Bank" compresses user presets and custom categories into a single shareable archive.
  - "Import Bank" unpacks and indexes patches directly into the Preset Manager.

### 4.4 A/B Patch Quick-Compare Toggle
- **Concept**: Instant auditioning against the unmodified saved patch state.
- **Workflow**:
  - A "Compare" button in the preset bar that toggles between current in-progress sound edits and the original loaded preset without losing unsaved tweaks.

---

## 5. Interface Ergonomics & Accessibility

### 5.1 Vector UI Scaling (100% – 200%)
- **Concept**: Crisp display scaling for modern 4K, 1440p, and high-DPI displays.
- **Options**:
  - Scaler presets: 100%, 125%, 150%, 175%, 200%, plus smooth corner-drag resizing.

### 5.2 Acoustic Intuition Status Bar / Tooltips
- **Concept**: A discrete bottom status strip translating physical modeling terminology into practical sound-design guidance.
- **Examples**:
  - Hovering over *Dispersion*: *"Stiffens the waveguide, bending high harmonics sharp to simulate stiff piano wires, metallic bars, and bells."*
  - Hovering over *Energy Loop*: *"Feeds back accumulated resonator energy through the non-linear excitation junction."*

### 5.3 Themed Metallic Palette Variations
- **Concept**: Alternate visual palettes preserving AERIFORM's distinct aesthetic.
- **Themes**:
  - *Classic Copper & Brass* (Current default)
  - *Deep Obsidian* (Darker minimalist studio theme with subtle amber accents)
  - *Verdigris* (Patinated copper, aged bronze, and sea-green accents)

---

## 6. Suggested Implementation Prioritization Matrix

| Phase | Feature | Complexity | Sound Design / UX Impact |
| :--- | :--- | :--- | :--- |
| **Phase 1 (Quick Wins)** | Section Padlock Randomizer | Low | High (immediate creative patch discovery) |
| **Phase 1 (Quick Wins)** | Preset Author & Notes in Browser | Low | Medium (context for presets & macros) |
| **Phase 1 (Quick Wins)** | Acoustic Tooltip Guidance Bar | Low | High (speeds up onboarding & learning curve) |
| **Phase 2 (Modulation & Visuals)** | Interactive Envelope Curve Shaping | Medium | High (expands envelope flexibility) |
| **Phase 2 (Modulation & Visuals)** | Dynamic Knob Modulation Range Arcs | Medium | High (modern, intuitive visual feedback) |
| **Phase 2 (Modulation & Visuals)** | UI Window Scaling (100%–200%) | Medium | High (essential for 4K / laptop users) |
| **Phase 3 (Physical Performance)** | Acoustic Round-Robin & Strike Jitter | Medium | Very High (unprecedented acoustic realism) |
| **Phase 3 (Physical Performance)** | Scala Microtuning (`.scl` / `.kbm`) | Medium | High (opens ethnic and microtonal music) |
| **Phase 3 (Physical Performance)** | Strum & Pluck Arpeggiator | Medium | High (expressive string/mallet playing) |
| **Phase 4 (Advanced Acoustic DSP)**| Custom Strike Impulse Loader (WAV) | High | Massive (marries sampling with physical modeling)|
| **Phase 4 (Advanced Acoustic DSP)**| Acoustic Soundboard / Body IR Stage | High | High (realistic acoustic air & wood resonance) |
