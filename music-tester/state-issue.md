# State Issue Documentation

## Current Problem

### What's Currently Happening (WRONG):

1. **You click 'airy' event** - airy.ogg and drone.ogg get freeverb/echo effects (GOOD)
2. **You click 'Intro' event**:
   - drums.ogg **immediately jumps to freeverb wet = 1** and then lerps down to 0 (BAD!)
   - airy.ogg **freeverb stays at its current level and doesn't lerp to 0** (BAD!)

The debug output shows:
```
[INFO] [EventSystem] Processing track: drums.ogg
[INFO] [EventSystem] Track effects: bassboost echo freeverb  // Shouldn't have freeverb!
[INFO] [EventSystem] Fading out effect: freeverb from wet: 0.923466 to 0.0

[INFO] [EventSystem] Processing track: drone.ogg
[INFO] [EventSystem] Fading out effect: freeverb from wet: 0.906476 to 0.0 (track not in target)  // This works
```

But **no processing for airy.ogg** - it's not being processed at all!

### What Should Happen (CORRECT):

1. **You click 'airy' event** - airy.ogg and drone.ogg get freeverb/echo effects, drums has NO effects
2. **You click 'Intro' event**:
   - drums.ogg should have **NO freeverb processing at all** (because drums never had freeverb)
   - airy.ogg should **fade out freeverb from current level to 0** (because Intro doesn't mention airy)
   - drone.ogg should **fade out freeverb from current level to 0** (because Intro doesn't mention drone)

The debug output should show:
```
[INFO] [EventSystem] Processing track: drums.ogg
[INFO] [EventSystem] Track effects: bassboost echo  // NO freeverb!

[INFO] [EventSystem] Processing track: airy.ogg
[INFO] [EventSystem] Fading out effect: freeverb from wet: 0.924 to 0.0 (track not in target)

[INFO] [EventSystem] Processing track: drone.ogg
[INFO] [EventSystem] Fading out effect: freeverb from wet: 0.907 to 0.0 (track not in target)
```

## Core Issues:

1. **Drums phantom freeverb**: State capture incorrectly includes freeverb on drums when it should be 0
2. **Airy not being processed**: The system isn't processing airy.ogg at all during Intro transition, so its freeverb doesn't fade out

## Root Cause Analysis:

The issue is in the per-track effect processing logic. The current implementation:
- Only processes tracks that are specified in the target event
- For tracks not in target, it should fade out ALL effects, but airy.ogg isn't being processed at all
- The state capture is incorrectly including effects that shouldn't be there

## Expected Behavior:

**All tracks are processed on every event. If a filter isn't mentioned in the new event BUT THE CURRENT AUDIO SYSTEM has that filter wet > 0, then we need to fade that filter down to 0 for THAT TRACK ONLY. Every effect is per track.** 