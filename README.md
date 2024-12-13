# C-HAT feeding logger
Schematics and code for C-HAT loggers with sensor for mandible movement

Open-source, homebuilt dataloggers designed initially for work on lobsters.

C-HAT: Crustacean Heart and Activity Tracker (including mandible sensor for feeding)

Arduino-based with substantial use of code snippets from Adafruit example libraries (among others).

## Basic usage
For basic usage and understanding of the process, see the readme for the main C-HAT repo (https://github.com/gutzler/C-HAT). This feeding branch repo assumes basic familiarity with the system.

### Important tips
- The Hall effect sensor is sensitive to polarity of the magnet - so swings may be positive or negative from a baseline. It's also a very short-range sensor and aligning the sensor with the magnet to ensure good data is a challenge. Practice and testing is going to be important in understanding how to set it up to succeed.
- Consider the shape of the magnetic field with whatever magnet used. I like bar-shaped magnets with through-thickness (axial) polarization to maximize the chance of getting a good signal.
