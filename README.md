# Csabiscope

Csabiscope is an STM32 oscilloscope project, using STM32F103C8T6 IC.

Supported features:
- drawing oscilloscope screen
- calculating min/max/effective values
- calculating frequency
- single or dual channel sampling upto 1.25 MHz
- triggers (rising/falling edge, change, min peak, max peak, external)
- optional sampling before trigger, configurable
- hardware amplification ( 1x, 2x, 4x, 5x, 8x, 10x, 16x, 32x )
- AC decoupling switch
- FFT
- USB data transfer up to 833 kbyte/s
- configurable test square signal (PWM degrades sampling quality)
- saving configuration onto flash
