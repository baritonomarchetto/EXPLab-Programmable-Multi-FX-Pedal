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

Input stage is very close to the one adopted by Electrosmash in their PedalShieldUNO, which in turn is very similar to those adopted by manufacturers of commercial digital processor. ElectroSmash themself made a very interesting analysis of the input stage of a Danelectro delay which closely resembles the solution adopted.

Input stage is MONO, and served by an op-amp in order to increase the impedence and keep the guitar signal unaffected. A series of RC filters reduce noise before hitting the microcontroller board.

Being that Pi Pico accepts a maximun input level of 3.3V, one easy way to limit the incoming voltage to such tensions is by powering the buffer stage @3.3V.

The most common opamp one could think of (TL072) calls for at least 6V rail-to-rail to operate in its linear region, so it's a no-go in this case. I have then adopted a less common op-amp (TL972, or TS972) which works well at low voltages.

Input stage op-amp is over and under-voltage protected with low drop Schottky diodes (BAT42). Being the room for input voltage already limited, using two diodes such as 1N4148 (0.7V drop each) is not recommended.

The remaining input op-amp is used to buffer the 1.6V virtual ground and keep it steady.

**Digital-to-Analog Convertion**

In a previous project of mine I messed up with PWM audio. I had to face the fact that PWM is not good enought to my ears, and a DAC (even the cheaper available) gives sensibly better results.

In the aforementioned project I used a PT8211 stereo DAC to fight against PWM limits, and it saved the project. A no brainer then.

PT8211 is a 16 bits DAC, massively used by DIY community, so it's a good candidate for a project like this, even for novices.

**Wet/Dry Mixing Stage**

After being processed, the left DAC channel goes through an inverting, mixing (summing) stage where it is mixed with a user definable amount of (filtered) dry signal. This stage is convenient to give more "life" to an otherwise 100% digital signal.

This stage gives a 2X gain to the summed signal.

The right, processed channel is instead buffered through the remaining opamp in 2X gain, inverting configuration.

Please notice that only the left channel has analog control over dry/wet signal.

**Filter Stage**
The output stage adopts the same solution I used in my Pi Pico Wavetable Oscillator Eurorack module. Both signals from the previous stage are independently low-pass filtered to limit digital artifacts at higher frequencies (noises and whines).

The filter adopted is a Sallen-Key low-pass filter: a simple-but-effective, second-order active filter.

The filter is not amplified, like I did in my wavetable oscillator, because there's already a 2X gain in the mixing stage.

An AC coupling capacitor and a current limiting series resistor complete che output stage for the RIGHT channel. The LEFT/MONO channel goes instead through the true bypass section before being filtered and outputted (see next).

**True Bypass**

The foot switch is configured in true bypass mode. When you deactivate the pedal, the input jack is wired directly to the left/mono output and the right channel muted at DAC level.

Common guitar pedal footswitches have 3 poles to toy with. Two are used to deliver the bypass function. The third pole is commonly used to light a LED indicating the status (on or off) of the effect.

Being that I also wanted to receive a status indication of the FX engagement, I adopted a simple circuit to catch both functions.

Here a Falstad's CircuitJS simulation of the circuit.

Please notice that only the left channel has true bypass. Right channel signal is "digital-only" and fully generated inside the pedal (MONO-to-STEREO).

**Daughterboards**

I designed three daughterboards to host different elements.

The first one is the footswitch daughterboard. It hosts the footswitch and a secondary circuit to monitor the state of the footswitch.

The second one is a potentiometer board. It hosts two potentiometers and is intended to be mounted on the left side of the pedal. I didn't adopted this in my project, but it's there for future use.

The third one is similar to the potentiometers board, but hosts two rotary encoders. This is the one that I actually used in this project because (a) it reduce the ADCInput load (monitoring two pots in less CPU intensive than four), (b) it is more stable than pot readings.

The rotary encoders board has a built-in debounce circuit to avoid false readings.

Potentiometers board and rotary board have elements placed with the same spacing, so they can be swapped with no further hardware modifications.


