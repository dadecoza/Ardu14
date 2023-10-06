## Ardu14: An Arduino-Based Emulator for the Science of Cambridge MK14 Micro Computer
<img src="https://raw.githubusercontent.com/dadecoza/Ardu14/main/Images/Ardu14PCB.jpg" alt="Ardu14" width="480" height="auto">

This project is based on the "Portable MK14 emulator in 'C'". While I'm not certain of the exact origins, I discovered it in Doug Rice's [Virtual MK14](https://github.com/doug-h-rice/virtual_mk14) GitHub repository. [Doug](http://www.dougrice.plus.com/) and [Hein Pragt's](https://www.heinpragt.com/english/software_development/ins8060_or_scmp_processor.html) websites were valuable resources while working on this project.

### Dependencies
* [Keypad Library](https://www.arduino.cc/reference/en/libraries/keypad/)
* [SPI](https://www.arduino.cc/reference/en/language/functions/communication/spi/)

### Components
 * 1x Arduino Nano
 * 1x MAX7219 7-Segment display module
 * 21x 6x6mm Tactile switches

### Schematics
You can find the schematics [here](https://github.com/dadecoza/Ardu14/blob/main/Images/Schematic.png).

***NOTE:*** The schematics includes an optional speaker and resistor ... In future I would like to implement emulation of the Ferranti ZN425E DAC to produce sound.

### Programs
I have included the example programs from the "Mk14 Micro Computer Training Manual". For descriptions and start addresses, I'll direct you to [Doug's webpage](http://www.dougrice.plus.com/dev/seg_mk14.htm).

### Loading Programs
Ardu14 can accept Intel Hex files uploaded over the serial port at 9600 baud.

<img src="https://raw.githubusercontent.com/dadecoza/Ardu14/main/Images/TeraTerm.PNG" alt="TeraTerm" width="480" height="auto">


### Limitations / TODO
* SIO instructions have not been implemented yet. It would be cool to enable writing to the Arduino's serial port using the SIO instructions.
* Using the inexpensive 16x2 LCD might not have been the best choice, as it does not handle rapidly changing characters very well. For instance, the scrolling text of the "SC message" program or the flying ducks in "Duck Shoot". Turning the potentiometer to lower the contrast does help make it a bit more acceptable.
 ***EDIT:*** It have been pointed out by Retro Phil on the [Science of Cambridge MK14 Microcomputer & NatSemi SC/MP](https://www.facebook.com/groups/1330313924562216) facebook group, that using the OLED version of the 16x2 display should resolve the issue.

### PIC14
a Similar and popular PIC based emulator from Karen Orton can be found at [Karen's Corner](http://techlib.com/area_50/Readers/Karen/micro.htm).


