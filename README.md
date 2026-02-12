# FXLab - DIY Programmable Stereo Multi FX Pedal
A DIY multi FX pedal to program custom guitar effects. Built around an RP2040.

# Intro
It was my convinction that developing a digital DIY pedal was not even worth a try.

Not that I am an analog fundamentalist (I am NOT), but it was safe for me to assume that common microcontroller boards we tinkerer rely on would struggle at sound processing.

What made me completely change my mind is a project from ElectroSmash called PedalShieldUNO: 100% digital, based on the cheapest Arduino board and PWM audio, it was anyway capable of producing serious Daft Punk - like robot distortion/ocave shift and honest fuzz.

Mind blown, new project on it's way!

# Features
- Full stereo path (mono input -> stereo output)
- 32KHz sample rate
- 12-bit in -> 16-bit out resolution
- True bypass
- 9 (plain) effects (more to come)
- Up to 3 parameters per effect
- Up to 16 chains of effects (max 4 effects per chain)
- Patch memory
- Customizable effects and chains
- Opensource

# Overview
FXLab is a multi effects circuit hosted in a common aluminum alloy expression pedal. These chassis are sold in every popular online shop nowadays.

These kind of pedals are often used to deliver FX like Wah Wah or voltage attenuation. They are not used for distortion, delay or any other FX other than the two cited.

I anyway adopted one of these because the nice plus an expression pedal brings with respect to box-shaped FX (stompboxes) is a on-the fly control of one of the FX parameters, at the only cost of some additional space.

I liked the idea of having such power under my foot (and honestly this bulky case was calling me for some use!).

FXLab is layed down like a common pedal, with it's signal input on the right, output on the left, effect engage visual indicator (LED) on the upper part, 9V center negative (or battery powered) supply.

What makes it special is the internal part.

At the bare bottom, we have a microcontroller board in charge of reading incoming audio signal, process it then output it through an external, dedicated 16-bits, stereo DAC. The microcontroller board acts then as a DSP (Digital Sound Processor).

The microcontroller board adopted (RP2040 Zero, a sort of Raspberry Pi Pico in small form factor) has excellent computational power, it is damn cheap and has a vast and helpful community of tinkerers around. This makes it a perfect choice for a platform oriented to give the possibility to people to create classic and even new and unheard effects with ease.

Simple analog filters at input/output stages and a DRY/WET mixer serve the microcontroller to reduce digital noise and artifacts and giveing some analog warmth to the processed signal.

# Circuits
**Power Supply Stage**

The pedal works out of a 9V alcaline battery or 9V center negative switching PSU.

Out of the main 9V power source a series of other tensions are derived.

The most important is +5V, which in turn is internally lowened to 3.3V by the built-in MCU step down converter to juice the RP2040.

A robust TL7805 is here used to generate 5V from 9V, even if the system do not drain too much current.

Other tensions are used as virtual grounds for operational amplifiers. These allow op-amps to operate with no distortion by appropriately DC shift incoming audio signals.

Input stage virtual ground is set @1.6V through a voltage divider and final buffer. Output stage virtual ground is set @4.5V with a divider only. Both see electrolite capacitors at their outputs for stabilization.

The two supply sources (battery and PSU) are kept isolated from each other by the use of the PSU connector ground breaker, a well known mechanical approach used in commercial products.

Battery is protected against wrong polarity connection through a series SB120 Schottky diode for safety.

The circuit is protected against wrong polarity with a 1N4004 diode connectind GND and PSU+.

The input audio jack is used as switch for battery operation: if no input jack is inserted, the battery "-" (B-) is not connected to ground and current cannot flow. This is a simply trick adopted by pedals manufacturers asking for no additional components. It's made by using a stereo input jack and connecting B- to the ring. When a mono input jack is inserted, the ring is shorted to ground and the battery can erogate current. Niiice :)

**Input Stage**

Input stage is very close to the one adopted by Electrosmash in their PedalShieldUNO, which in turn is very similar to those adopted by manufacturers of commercial digital processor. ElectroSmash themself made a very interesting analysis of the input stage of a [Danelectro delay](https://www.electrosmash.com/back-talk-analysis) which closely resembles the solution adopted.

Input stage is MONO, and served by an op-amp in order to increase the impedence and keep the guitar signal unaffected. A series of RC filters reduce noise before hitting the microcontroller board.

Being that Pi Pico accepts a maximun input level of 3.3V, one easy way to limit the incoming voltage to such tensions is by powering the buffer stage @3.3V.

The most common opamp one could think of (TL072) calls for at least 6V rail-to-rail to operate in its linear region, so it's a no-go in this case. I have then adopted a less common op-amp (TL972, or TS972) which works well at low voltages.

Input stage op-amp is over and under-voltage protected with low drop Schottky diodes (BAT42). Being the room for input voltage already limited, using two diodes such as 1N4148 (0.7V drop each) is not recommended.

The remaining input op-amp is used to buffer the 1.6V virtual ground and keep it steady.

**Digital-to-Analog Convertion**

In a [previous project of mine](https://www.instructables.com/Pi-Pico-Dual-Voice-Voltage-Controlled-Wavetable-Mo/) I messed up with PWM audio. I had to face the fact that PWM is not good enought to my ears, and a DAC (even the cheaper available) gives sensibly better results.

In the aforementioned project I used a PT8211 stereo DAC to fight against PWM limits, and it saved the project. A no brainer then.

PT8211 is a 16 bits DAC, massively used by DIY community, so it's a good candidate for a project like this, even for novices.

**Wet/Dry Mixing Stage**

After being processed, the left DAC channel goes through an inverting, mixing (summing) stage where it is mixed with a user definable amount of (filtered) dry signal. This stage is convenient to give more "life" to an otherwise 100% digital signal.

This stage gives a 2X gain to the summed signal.

The right, processed channel is instead buffered through the remaining opamp in 2X gain, inverting configuration.

Please notice that only the left channel has analog control over dry/wet signal.

**Filter Stage**

The output stage adopts the same solution I used in my [Pi Pico Wavetable Oscillator Eurorack module](https://www.instructables.com/Pi-Pico-Dual-Voice-Voltage-Controlled-Wavetable-Mo/). Both signals from the previous stage are independently low-pass filtered to limit digital artifacts at higher frequencies (noises and whines).

The filter adopted is a Sallen-Key low-pass filter: a simple-but-effective, second-order active filter.

The filter is not amplified, like I did in my wavetable oscillator, because there's already a 2X gain in the mixing stage.

An AC coupling capacitor and a current limiting series resistor complete che output stage for the RIGHT channel. The LEFT/MONO channel goes instead through the true bypass section before being filtered and outputted (see next).

**True Bypass**

The foot switch is configured in true bypass mode. When you deactivate the pedal, the input jack is wired directly to the left/mono output and the right channel muted at DAC level.

Common guitar pedal footswitches have 3 poles to toy with. Two are used to deliver the bypass function. The third pole is commonly used to light a LED indicating the status (on or off) of the effect.

Being that I also wanted to receive a status indication of the FX engagement, I adopted a simple circuit to catch both functions.

[Here a Falstad's CircuitJS simulation](https://tinyurl.com/2yp258vf) of the circuit.

Please notice that only the left channel has true bypass. Right channel signal is "digital-only" and fully generated inside the pedal (MONO-to-STEREO).

**Daughterboards**

I designed three daughterboards to host different elements.

The first one is the footswitch daughterboard. It hosts the footswitch and a secondary circuit to monitor the state of the footswitch.

The second one is a potentiometer board. It hosts two potentiometers and is intended to be mounted on the left side of the pedal. I didn't adopted this in my project, but it's there for future use.

The third one is similar to the potentiometers board, but hosts two rotary encoders. This is the one that I actually used in this project because (a) it reduce the ADCInput load (monitoring two pots in less CPU intensive than four), (b) it is more stable than pot readings.

The rotary encoders board has a built-in debounce circuit to avoid false readings.

Potentiometers board and rotary board have elements placed with the same spacing, so they can be swapped with no further hardware modifications.

# OK Computer

At the base of the software there's [Arduino IDE](https://www.arduino.cc/en/software) and [Earl Phil Hower Arduino Pico core](https://github.com/earlephilhower/arduino-pico). These two toghether make a solid platform to toy with a RP2040-based microcontroller board.

Audio signal handling is a niche asking for very specific features, the most important being sampling signals at high speed. Default Arduino's "analogRead()" function is not good for audio signal readings because of its "not upright" timing.

Luckily for us, Arduino Pico environment has a library named [ADCInput](https://arduino-pico.readthedocs.io/en/latest/adc.html) that makes this task tinkerer-proof :).

Another thing that made a difference in this project was the use of AI to write effects. Once the base code was written, with it's due hardware definitions and general structure, it was a matter of asking the kind of effect needed to the AI to see a list of lines generated.

Some times they worked with minimum-to-no modifications, some times I had to take full control of the situation, but I must confess that I had a lot of fun experimenting this way.

Turning the final effect from a "ethimologically correct" effect to something musically good calls for your taste, since the AI is instrucred, not smart. But it allows us to focus on WHAT we want rather than HOW to do it.

If you have already dealt with AI you certainly know how important it is to give proper input. This is what I found working best with this project:


“I’m working on a pedal-based multi-effects unit built around an RP2040. The multi-effects unit has a pedal (expression pedal) and two rotary encoders. The full audio path is: guitar → analog buffer → RP2040 (12-bit, unsigned) → PT8211 (16-bit, SIGNED) → analog wet/dry mixer → Stereo output. Keep left channel signal 100% WET. Keep right channel WET/DRY fixed at 50%. The two channels (left and right) are independent of each other. The hardware works well, and the PT8211 is handled correctly (LSBJ format). Write the code for a EFFECT NAME HERE ”


Notice that left channel is kept WET-only because the DRY/WET blending is hardware set through the dedicated on-board trimmer. This isn't a valuable information for the AI.

You also want to attach the full code, for a copy-and-paste result.

# Code Structure and How-to

Effect's Chains
The code is structured such that effects can be developed independently from each other, then used alone or in any effects chain, in any order.

In the moment I am writing, default effects are:

Compressor
Octaver
Distortion
Daft Distorstion
Bit Crusher
Delay
Chorus
Flanger
Reverb
Current code handles 16 chains of maximum 4 effects each.

In the moment I am writing, default chains are:

Compressor → Distortion → Chorus

Compressor → Distortion → Flanger

Compressor → Distortion → Reverb

Compressor → Daft Distortion → Delay

Compressor → Daft Distortion → Chorus

Compressor → Daft Distortion → Flanger

Compressor → Daft Distortion → Reverb

Octaver → Distortion → Delay

Octaver → Distortion → Flanger

Octaver → Daft Distortion → Delay

Octaver → Bit Crush → Delay

Distortion

Delay

Chorus

Flanger

Reverb

User can select the chain of interest by setting the 4-bit DIP switch according to attached scheme (0 means OFF, 1 means ON).

Please notice that actual chains are limited to max 3 effects each, even if the code could handle 4.

Effect's Parameters
Each effect has two or three control parameters. In general, the most effective (or "expressive") is at player's foot, the other two assigned to rotaries.

Here is the current effects parameters mapping:


| Effect | Foot Pedal (Param 0)| Rotary 1 (Param 1) | Rotary 2 (Param 2) |

| Compressor | Sustain / Threshold | Attack | Output Level (Make-up Gain) |

| Octaver | (unused) | Left Octave Shift | Right Octave Shift |

| Distortion | Drive | Body Output Gain | Tone (Low-pass Filter) |

| Daft Distortion | Drive + Smpl Redct. | (unused) | Stereo Width |

| Bit Crush | Drive | Bit Depth | Sample Rate Reduction |

| Delay | Feedback | Delay Time | Stereo Spread (L/R Offset) |

| Chorus | LFO Rate | Modulation Depth | Base Delay Time |

| Flanger | LFO Rate | Modulation Depth | Feedback |

| Reverb | Pre-Delay | Decay | Damping |


The code is written such that user can change the modulator (footpedal or encoder) for any parameter. This has to be done at code level, not on the fly.

Effect parameters can be changed when the parameter is in focus. To move focus from one parameter to the previous/next, user can press the encoder "Z" button.

All effects have left channel 100% WET (mix is set at hardware level through the dedicated trimmer), right channel is fixed 50% WET, 50% DRY.

Memory!
Parameters values can be stored for recall. Memorized parameters are kept in flash memory after shut down and reloaded at power on.

Flash memory cannot be written for more than 100.000 times, so writing to it must be a user-trigged action, not authomatic.

User can record parameters values simply by pressing the on-board, dedicated button.

# Pedal Chassis Modification

The expression pedal alone limits the ability to fine-tune our effects. It therefore becomes necessary to modify the pedal chassis so that two rotary encoders can be installed on the right side and made accessible.

The encoders PCB can be used as a practical drill mask. In the following the steps to drill the two holes:

- Apply some tape to the case in the drill zone. This is to prevent damages (or better "surface scratches") if the driller hits the case surface,
- With the help of the drill mask and a pencil, set drill positions
- Punch the holes with a hole puncher (optional)
- Drill the holes, starting with a very small drill. These are guides for the next drills.
- Increase the hole up to 4 or 6 mm
- Use a multi-step drill to enlarge the holes up to 8 mm (7 would be a perfect fit).

Now remove the tape, vacuum the working area from debrids et-voilà: you chassis is now ready for the assembly Step.

