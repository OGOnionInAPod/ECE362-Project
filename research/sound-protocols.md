# Possible Sound Transmission Protocols

ADC and DAC components which are designed specifically for audio, such as the [TAA3040](https://www.digikey.com/en/products/detail/texas-instruments/TAA3040IRTWR/25676406) and [TAD5112](https://www.digikey.com/en/products/detail/texas-instruments/TAD5112IRGER/26251062) expect to be able to communicate audio data over a dedicated audio protocol rather than a general purpose I2C or SPI line. For the components mentioned above, supported protocols are I2S (inter-ic sound), TDM (time division multiplexing), or LJ (left-justified).

Getting straight to the point, the best choice for our use case depends on the number of pickups. Each pickup corresponds to one channel, so a two-pickup design could utilize I2S or LJ, but a three pickup design would utilize TDM. Because of this variability, we should implement __TDM__, because it can cover all cases with only marginally increased complexity.

All configurable protocols are synchronous serial protocols utilizing the same 3 physical lines: FSYNC, BCLK, and SDOUT. BCLK is the shared clock signal, SDOUT is the data line, and FSYNC is used to indicate the frame or channel that corresponds to the data being transmitted. Details about each option below.

## TDM

TDM is a generic approach to data transfer rather than a specification. As the name implies, multiple data channels use a time-sharing approach to access the bus. Because it is not a standardized protocol, the best reference material is the [TAA3040 datasheet](https://www.ti.com/lit/ds/symlink/taa3040.pdf). Section 6.3.1.2.1 figures 6-1 through 6-4 (I know) show example timings for the different possible TDM operating modes.

To summarize the mechanism, the FSYNC line is pulsed for one clock cycle to indicate the start of a word, at which point data is made available on SDOUT to be read on the rising edge of BCLK until the full word has been transmitted. Each word is considered to belong to a different channel, identifiable by the edge of FSYNC. The word length is fixed.

TLDR; Multiple channels are communicated across the same wire by transferring one word from each channel one after the other until all channels have been represented.

[A more general explaination](https://gab.wallawalla.edu/~larry.aamodt/engr432/cirrus_logic_TDM_AN301.pdf).

## I2S

The I2S protocol is similar to TDM, but is only defined for two channels: left and right. Rather than using a pulse of FSYNC to indicate the start of a new channel word, the channel is indicated by the level of FSYNC. Low indicates the left channel, and high indicates the right channel. Data is still sent MSB first, and the only other limitation is that transitions to FSYNC must occur one cycle before the MSB of the following word is transmitted.

Sample timing diagrams available in section 6.3.1.2.2 figures 6-5 through 6-8 of the [TAA3040 datasheet](https://www.ti.com/lit/ds/symlink/taa3040.pdf)

[A more general explaination](https://www.digikey.com/en/maker/tutorials/2023/what-is-the-i2s-communication-protocol).

## LJ

Like the other sections, the [TAA3040 datasheet](https://www.ti.com/lit/ds/symlink/taa3040.pdf) has timing diagrams for this protocol in section 6.3.1.2.3 figures 6-9 through 6-12.

To my eye, this protocol only differs from I2S in two ways: the polarity of FSYNC is swapped, and the single cycle of delay between the transition of FSYNC and the data being transmitted is removed. That is to say, the MSB is transmitted on the same cycle as the FSYNC edge, rather than the cycle after.
