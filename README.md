# gpio-segled
gpio-segled - driver for GPIO-based segmented LEDs

This is a Linux kernel mode driver for multi-digit simple segmented
Light-Emitting Diode (LED) devices driven directly from General Purpose
Input/Output (GPIO) pins and not through separate decoder or
controller hardware.

Simple segmented LED devices are common-anode or common-cathode in nature,
where for each digit all the anodes or all the cathodes are connected to
a single pin, which selects (turns on or off) that digit.  Other pins
select segments, where the same segment in all digits are tied together.
Thus every individual LED can be conceived as being a cell in a
two-dimensional matrix, with the rows and columns of the matrix being the
digits and the segments.

Due to the fact that pins are common between either segments or digits,
it is impossible to display different information on different digits
with a static set of levels applied to the pins.  Instead, typically,
only one digit at a time is turned on, and the driver will rapidly "scan"
the digits, cycling through each digit rapidly (typically on the order of
100Hz) in order to construct an illusion (via "persistence of vision")
that all the digits are on at the same time.  Typically, dedicated hardware
is used to perform this function.  This driver is useful when no such
hardware is present but the device can be driven directly through GPIO
pins instead.

As a side benefit of the scanning function of the driver, the brightness
of the LEDs can be easily changed on the fly by adjusting the duty cycle
of each digit.  In designs where current is limited at the digit selector
pins, the driver may also be configured to automatically adjust the
brightness of each digit to match those of other digits, by taking into
consideration how many segments are lit (in other words, a '4' may need
to be kept on twice as long as a '1', because only 2 segments are lit for
a '1' versus 4 segments for a '4').

Only seven-segment devices (really eight-segment, but the decimal point
segment is often not counted) are currently supported by this driver.
The LED segments are spatially arranged and labeled according to the
following, which is consistent with the de-facto standard described in
Wikipedia entry for "Seven-segment display"
(https://en.wikipedia.org/wiki/Seven-segment_display):

![figure showing standard naming and arrangement of segments](https://upload.wikimedia.org/wikipedia/commons/thumb/0/02/7_segment_display_labeled.svg/220px-7_segment_display_labeled.svg.png)

An example of this type of device is the package with model identifier
"SMA420564" etched in dot-matrix print on the side that can be found in
various electronic kits that are made to complement the Arduino and
Raspberry Pi.  Its datasheet, if one exists, has proven too elusive to
find online.  Nevertheless, the segment and digit pins are easily
determined through experimentation to be as follows:

  Pin |  Function
----- |  -------------------------------
   1  |  Segment E anode
   2  |  Segment D anode
   3  |  Segment P (decimal point) anode
   4  |  Segment C anode
   5  |  Segment G anode
   6  |  Digit 4 cathode
   7  |  Segment B anode
   8  |  Digit 3 cathode
   9  |  Digit 2 cathode
  10  |  Segment F anode
  11  |  Segment A anode
  12  |  Digit 1 cathode

Notes for hardware designers:
1. The component has no internal current limiters, and so requires
   resistors or other such current limiters in an any actual design.

2. If limiting current at the digit selector (common anode/cathode),
   add "seg-adjust;" as a property to the device in the device tree,
   in order to configure the driver to automatically adjust the duty
   cycle of each digit to even out brightness between different digits.

3. The common anode/cathode pins, as they collect the current from
   up to eight separate segments, may draw more current than a typical
   GPIO pin can drive.  NPN transistors can be used to provide the
   additional current.  For an example, see the following
     http://learn.parallax.com/4-digit-7-segment-led-display-arduino-demo
