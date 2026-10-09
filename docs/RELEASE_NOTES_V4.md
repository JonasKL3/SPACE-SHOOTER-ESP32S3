# SPACE SHOOTER v4.0.0 — Arsenal & Bosses Update

Major update for ESP32-S3 with expanded gameplay and performance improvements.

## What's New

- Campaign mode extended to 20 levels, with bosses at levels 5, 10, 15 and 20.
- Five unique bosses: Sentinel, Hydra, Prime, Devourer and Omega (Omega in Endless mode).
- Six weapons: Default, Double, Triple, Spread, Laser and Homing Missile.
- Piercing lasers and guided missiles.
- Five changing backgrounds: Space, Nebula, Asteroids, Storm and Chaos.
- Distinct weapon power-ups, shield, extra lives, combos and persistent statistics.

## Stability & Performance

- Improved display scheduling (~24 visual FPS target) to reduce temporary freezes during item pickups and boss encounters.
- Optimized particle spawning and reduced event logging during gameplay.
- Deferred Game Over NVS writing and added a short transition interval between consecutive bosses.
- Added DRAW/SPI/LOGIC/SLOW performance diagnostics to the pause screen.

## Fixes & Compatibility

- Improved Dabble analog input, D-pad priority, Y-axis mapping and diagonal movement normalization.
- Restored player movement speed to 6.0 and prevented rapid repeated damage.
- Fixed laser duplicate hits, menu transition bugs and false new high-score alerts.
- Preserved previous NVS statistics and skin unlocks; 20-level campaign victory is tracked separately.
- Preserved the approved Cyan/Gold visual label adjustment in the ship skin selection screen.

## Hardware

- ESP32-S3 N16R8
- 2.8-inch ILI9341 TFT (320×240)
- XPT2046 touch controller (not used for gameplay)
- Dabble BLE GamePad

See the included `README.md`, hardware setup, installation instructions and test plan for details.

**Version:** `v4.0.0`
