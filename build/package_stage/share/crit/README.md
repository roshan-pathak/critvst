# crit

A plugin for controlled instability - designed for 'distorted pop' production.

## Overview

**crit** models behavior under load, not static effects. every control alters how the system responds to audio over time, creating organic instability and character.

### Compatibility:
   - VST/AU compatibility
   - Originally tested on Logic Pro
   - Validated on pluginval

## What Do My Knobs Mean?

### 5 Macro Parameters

1. **Pressure**: stress it out.
   - Gives drive and stress input to the system
   - Pushes input gain into level-dependent waveshaper
   - Harmonic generation
   - Dynamic EQ tilt with low-end tightening and high-mid emphasis
   - Nonlinear response curve
   - Generates the "stress" that will drive your other effects.

2. **Fault**: now break it. strain, edge, crunch.
   - Instability, but only under stress (effect = fault x pressure)
   - Signal-following micro pitch drift (no LFO!)
   - Phase skew between partial bands
   - Sub-millisecond time smear
   - Minimal impact at low Pressure
   - Designed to sound "strained," not detuned

3. **Split**: pull it apart.
   - Stereo divergence under load
   - Saturates left/right audio differently with microtiming offset
   - Reduce correlation without completely inverting phase
   - Mono compatibility preserved up to ~0.7

4. **Momentum**: shape it.
   - Reaction speed, recovery behavior
   - Attack/release times of saturation envelope
   - Pitch drift onset and correction speed
   - Rate of stereo divergence

5. **Resolve**: rescue it.
   - Dynamic recovery and stabilization
   - Dynamic soft limiting / Harmonic smoothing / density reduction
   - Pitch and phase re-centering
   - Transient preservation under heavy stress

### Tips

- This plugin responds to signal level. Quiet sounds don't work as good.
- Loud transients disproportionately increase stress
- For subtle character: low Pressure + moderate Fault works well
- For aggressive processing: high Pressure + high Resolve is crunchy
- For wide stereo: moderate Split + high Momentum is spacey

## License

MIT License - see LICENSE file for details

## Author

rosh
