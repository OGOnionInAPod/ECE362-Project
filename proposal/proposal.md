# Electronic Guitar (Modeling Guitar?)

This project seeks to design a spin on the classic electric guitar called an "electronic guitar" or "digital guitar", which utilizes embedded systems and microcontrollers concepts to digitize and modify the audio of a physical instrument in real time.

After researching further, it has become apparent that this type of product exists under the name "modeling guitar". They are called this because they can "model" the sound of many other types of guitar. They do this by individually recording the signal of each string and processing the data digitally, much like this project seeks to do, just on a larger scale.

## Background

Like most musical instruments, the guitar is a flexible concept that comes in many shapes, sizes, and implementations. Despite the breadth of characteristics that may or may not be present in a particular instrument, even someone with no knowledge of guitars can make one easy distinction: acoustic or electric.

The names say it all. An acoustic guitar makes use of a large body that acts as a resonating chamber, amplifying the sound from the vibrating strings. An electric guitar converts the vibrations of the strings into an analog signal and then uses an amplifier to broadcast the sound to the audience. Both types are instantly recognizable pieces of pop-culture.

### Mechanics

The electric guitar that we know and love is, almost always, a purely analog device. The strings on an electric guitar are ferrous, meaning their vibration can be captured by a "pickup", the name given to the copper coils sitting just below the strings. The fluxuating magnetic field of the strings induces a current in the pickup, which can be interpreted as an audio signal. Aside from the pickups, most electric guitars contain little more than a handful of voltage dividers and LP/HP filters to act as gain and tone controls, altering the volume (and distortion) or sound characteristics of the instrument. From here, the audio signal is passed to an amplifier, or through "pedals" (we will come back to this).

### Shortfalls

An electric guitar by itself hardly even counts as an instrument. Without equipment to amplify the output signal (often on the order of milliwatts) to a usable volume, it becomes a very poorly designed acoustic.

A very common component of music played with an electric guitar are "effects" or modifications to the sound of the instrument. Being analog in nature, effects are usually applied by "pedals", which are additional analog circuits that are activated with your foot while playing. These are connected in series between the guitar and the amplifier, allowing the player to alter the sound the the instrument while playing to achieve interesting results.

Preamps, amplifiers, and pedals are all pieces of equipment which may be necessary to use an electric guitar. These can be very expensive, partially because of the complexities of analog design.

### Takeaway

By digitizing the signal from the pickups inside the instrument, we can circumvent many of the shortfalls in usability of an electric guitar. With a sufficiently powerful processor on-board, it is even possible to apply effects in real time without the need for bespoke equipment. These features might be very appealing to a player who uses digital producing software or just wants to record digital audio.

## Technical Description

This is a fairly complex project, but does not stray far into the realm of the unknown. The deliverable is well defined, meets requirements for project complexity, and sticks closely to the feature set of the RP2350, only reaching for external components where better performance is required (like the ADC, see below). The design can be split roughly into three stages.

### Analog Frontend

The pickups are the beating heart of an electric guitar, and they are analog devices. There's no way around this reality, so we need the appropriate analog circuitry to sit in front of the digital block in order to safely digitize it.

__A minimum frontend would require:__

* Tying the pickup negative terminals to analog ground
* Adding coupling capacitors been the pickup's positive terminals
* Applying a midpoint (1.65V) bias
* Adding a unity gain __*__ buffer to each channel

__Some optional features might include:__

* Adding an analog passthrough circuit, skipping the digital stage entirely and allowing the instrument to function like a totally normal electric guitar
* Adding filters to reduce noise, since we are only interested in the audible range of frequencies anyway

__*__ If you're wondering why we wouldn't amplify this up to 3.3V to make sure we utilize the full range of the ADC, then you're following closely. The reality is that a good external ADC that's designed for audio applications usually has a programmable gain value for each channel. The [TAA3040IRTWR](https://www.ti.com/product/TAA3040/part-details/TAA3040IRTWR) for example can apply a programable gain of 0-42dB to any of the channels, negating some of the need for a dedicated gain stage. For the rest of this section, you can assume that any references made to "the ADC" are talking about this specific part.

### Digital Backend

This is the bread and butter of this project. Having connected the pickup outputs to our ADC through the analog frontend, we now get to work on digitizing and/or modifying that signal. This can be done with our trusty RP2350, which has all the peripherals and computational power needed to make this project a reality.

The ADC can be configured via multiple interfaces, supporting both I2C and __SPI__. This application (the guitar broadly), doesn't require many peripherals communicating with one another, so SPI is a more convinient choice for its simplicity over I2C. Thus, the RP2350 must connect to the ADC via __SPI__ to read/write the controlling registers.

The ADC also needs to be able to transmit the data it converts. It wouldn't be very useful otherwise. This is done over the __I2S__ or inter-IC sound protocol. It's a serial protocol much like SPI, but with an additional frame-synchronization signal for handling stereo audio. The RP2350 doesn't have a dedicated controller for handling such a connection, so we need to implement it ourselves. Thankfully, it does have several __PIO__ (programmable input/output) blocks. These blocks are their own co-processors that are programmed with cycle-accurate procedures to interact with the GPIO pins. This means they can offload the processing of a custom protocol from the main core. Using this, we can asynchronously recieve audio data from the ADC in real time, without burdening the rest of our program. How elegant!

In addition to connecting to the ADC, the microcontroller also needs to connect to a DAC in order to connect to the upcoming output stage. This is almost exactly like the above, but in reverse, so I've omitted the details for now.

It should also be noted that, for the guitar to be "controllable" (think a volume knob or effect selection), it would be necessary to have input buttons or dials as well. The exact nature of these are beyond the scope of initial brainstorming, but buttons on GPIO or dials/potentiometers on the built-in ADC would suffice for this purpose.

### Output

It's not a guitar unless you can get some music out of it, so we need an output stage. Thankfully, this is a lot less complicated then dealing with analog inputs. To stick with the theme of unifying the digital and the analog, we design in two outputs:

* Analog output via an DAC
* Digital output over USB-C

As stated before, the analog output just converts the digital signal back into an analog output so the guitar works with the standard array of electric guitar accompanyments. Implment an I2S transmitter via PIO, configure the DAC via SPI, and be on your way. Just the mirror image of the ADC implementation from before.

For the digital output, we utilize the USB 2.0 controller built into the RP2350. The accompanying USB 1.1 PHY is capable of Full-Speed (12 megabits or 1.5 megabytes per second). This is more than enough for a single stream of audio data, so it will suffice just fine. Include a USB-C header, wire up the appropriate connections, and we're golden.

## Software

So far there's been no mention of what sort of software needs to be implemented for this project to be fruitful and functional. Fear not, for there is plenty to do.

### The ADC

The ADC responsible for processing all the incoming sound data needs to be configured. At a minimum, this involves configuring which channels should sample at what frequency and what bit depth, as well as the parameters for how data is to be sent over the I2S interface. In addition to this, it may be appealing to utilize the advanced features of a quality ADC, like the channel gain mentioned before. The exact details of how to get the most use out of this peripheral are for later in the design process.

### The DAC

Much the same as the ADC, this needs to be configured for an appropriate output for use with other analog audio equipment. A digital guitar is cool, but a digital only guitar would be more cumbersome than a purely analog one.

### USB Output

USB is just a wire, and audio is just bits. In the absence of a file system, the procedure to transfer the audio data over the USB connection will need to be implemented on the processor as well. Each time the processor is done with a chunk of data, it needs to send that to the appropriate output, be that the USB controller or the DAC.

### I2S Over PIO

This is the first fairly complex thing that the software needs to do. Since I2S is not a native protocol on the RP2350, this needs to be implemented __using assembly__ for the PIO co-processors. See [this example](https://github.com/raspberrypi/pico-examples/tree/master/pio/i2c) of how PIO can be used to implement I2C. This would be fairly interesting to someone with a taste for bitbanging. Note also that this would be a stellar use-case for the DMA controller as well. Setting up automated transfers out of the PIO FIFO registers and into memory for the processor to operate on reduces the load on the processor massively.

### Data, DSP, and Effects

And here we arrive at the whole point of this project. How can we apply effects to the audio without ever connecting a pedal? Well, having built up all the infrastructure around the processor in the above sections, we are finally ready to tackle that question.

Now that we have an incoming stream of audio data available to the processor, we have full control over the characteristics of that signal. We can incorporate user input from buttons or knobs to adjust things on the fly. For example, you could easily multiply each sample of your input by a scaling factor to adjust the volume. There's also a long list of more complicated effects. Here are some examples of computationally inexpensive effects that can be applied to alter the sound of the instrument:

* __Distortion/Overdrive__: An effect that mimics the sound a tube amplifier makes when it's driven too hard. You apply a gain to a signal, the clip with with some non-linear shaping function.
* __Equilizer__: An effect that filters out certain parts of the frequency spectrum. The implementation depends a lot on the goal, but there are plenty of options.
* __Tremolo__: An effect that adjusts the volume of a signal up and down periodically. Simply scaling the current sample by the current value of a low-frequency oscillator (hardware or a lookup table) can achieve this effect cheaply.

The list goes on forever, but the point is clear. Barring the most computationally heavy effects, you can make all kinds of adjustments to the sound of the guitar without adding a single piece of hardware.

Having said all that. Doing any of this requires handling large amounts of audio data at as it comes in from the ADC. Dealing with this amount of data efficiently is a challenge on its own. Note that there's likely also a whole world of parallel programming to explore, utilizing both available cores to their fullest extent can further push the boundary of what can be done with this project.

## Project Requirements

The top-line project requirements are as follows:

* RP2350 is the __ONLY__ computer in use
* __Four unique__ peripherals on the RP2350 must be used
* __One__ device not otherwise used in lab

The first requirment is no obstacle. Only one computer is necessary.

This project utilizes SPI, PIO, USB, and the on-board ADC, which covers the 4-peripheral requirement. Note that this is the on-board ADC, which will be used for user input such as a volume knob. This does not include the external ADC or DAC. Thus, four unique peripherals on the RP2350 are in use, meeting the requirment.

The project uses specialized ADC and DAC modules because the ADC on the RP2350 is insufficient for quality audio signals. Even if the ADC does not count, the DAC does, meeting this requirment as well.

## PCB Feasibility

A PCB of some kind for this project is essentially required. The system needs to be inside of a guitar body (or at least loosely in proximity to it) in order to serve its intended purpose. Thus, a minimal PCB is an expected deliverable of this project. The 2% version outlines the minimum requirements for this, while the 4% version takes into consideration additions that could be made to bring the design up to sufficient complexity.

### 2% Version

* The analog frontend as described in the technical description
* The ADC, DAC, and USB-C modules and accompanying connections
* Terminals for pickup input and DAC output
* Headers for the proton
* Power over USB-C

### 4% Version

* Everything from the 2% version
* RP2350A package
* QSPI flash
* GPIO headers for user input
* Battery power/charging over USB

## Parts

| Quantity | Name | URL |
| --- | --- | --- |
| 1 | ADC | [https://www.digikey.com/en/products/detail/texas-instruments/TAA3040IRTWR/25676406](https://www.digikey.com/en/products/detail/texas-instruments/TAA3040IRTWR/25676406) |
| 1 | DAC | [https://www.digikey.com/en/products/detail/texas-instruments/TAD5112IRGER/26251062](https://www.digikey.com/en/products/detail/texas-instruments/TAD5112IRGER/26251062) |
| 1 | Low-Voltage OpAmp | [https://www.mouser.com/en/ProductDetail/STMicroelectronics/TSX634IYPT?qs=Rlwi1sSxVL5taZHfZVZPDA%3D%3D](https://www.mouser.com/en/ProductDetail/STMicroelectronics/TSX634IYPT?qs=Rlwi1sSxVL5taZHfZVZPDA%3D%3D) |
| 1 | 5V - 3.3V Voltage Regulator | [https://www.digikey.com/en/products/base-product/onsemi/488/NCP1117/151453](https://www.digikey.com/en/products/base-product/onsemi/488/NCP1117/151453) |
| 1 | USB-C Breakout | [https://adafruit.com/product/4090](https://adafruit.com/product/4090) |

## Related Works

### PolyMap Pickup System

* __URL:__
  * Blog: [https://darkartguitars.com/blog/polymap](https://darkartguitars.com/blog/polymap)
  * Video: [https://www.youtube.com/watch?v=6SRUF4_frlk&t=5s](https://www.youtube.com/watch?v=6SRUF4_frlk&t=5s)
  * GitHub: [https://github.com/IIP-Group/polymap/tree/main](https://github.com/IIP-Group/polymap/tree/main)
* __Summary:__ Very similar in spirit to the goals of this project, but using a much more complex array of 64 piezoelectric pickups, giving 8 data points per string. Because of this, an FPGA is used for the processing over a microcontroller to handle the bandwidth.

### Modeling Guitar

* __URL:__ [https://www.thomannmusic.com/onlineexpert_page_midi_and_modelling_guitars_modelling_guitars.html](https://www.thomannmusic.com/onlineexpert_page_midi_and_modelling_guitars_modelling_guitars.html)
* __Summary:__ Turns out the thing I'm imagining is called a "modeling guitar". They do exist, but I can't find an open-source project or community surrounding them.

### DS-Pi | RP2040 Audio DSP Board

* __URL:__ [https://hackaday.io/project/184811-ds-pi-rp2040-audio-dsp-board](https://hackaday.io/project/184811-ds-pi-rp2040-audio-dsp-board)
* __Summary:__ Open-source audio DSP platform based on the RP2040 and using an audio codec. Totally unrelated to the guitar part, but otherwise similar.

### Guitar Effects List

* __URL:__ [https://spartanmusic.co.uk/blogs/smblog/12134577-for-beginners-full-list-of-guitar-pedals-effects](https://spartanmusic.co.uk/blogs/smblog/12134577-for-beginners-full-list-of-guitar-pedals-effects)
* __Summary:__ Not a project. Just a list of effects for inspiration.
