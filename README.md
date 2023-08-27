## Ardu14: An Arduino-Based Emulator for the Science of Cambridge MK14 Micro Computer
<img src="https://raw.githubusercontent.com/dadecoza/Ardu14/main/Images/Ardu14.jpg" alt="Ardu14" width="480" height="auto">

This project is based on the "Portable MK14 emulator in 'C'". While I'm not certain of the exact origins, I discovered it in Doug Rice's [Virtual MK14](https://github.com/doug-h-rice/virtual_mk14) GitHub repository. [Doug](http://www.dougrice.plus.com/) and [Hein Pragt's](https://www.heinpragt.com/english/software_development/ins8060_or_scmp_processor.html) websites were valuable resources while working on this project.

### Dependencies
* [Keypad Library](https://www.arduino.cc/reference/en/libraries/keypad/)
* [LiquidCrystal Library](https://www.arduino.cc/reference/en/libraries/liquidcrystal/)

### Schematics
None! Well, kind of. Just follow the guides for the above-mentioned libraries and configure the GPIO pins accordingly in the Ardu14 sketch.

### Programs
I have included the example programs from the "Mk14 Micro Computer Training Manual". For descriptions and start addresses, I'll direct you to [Doug's webpage](http://www.dougrice.plus.com/dev/seg_mk14.htm).

### Loading Programs
Ardu14 can accept Intel Hex files uploaded over the serial port at 9600 baud.

<img src="https://raw.githubusercontent.com/dadecoza/Ardu14/main/Images/TeraTerm.PNG" alt="TeraTerm" width="480" height="auto">


### Limitations / TODO
* SIO instructions have not been implemented yet. It would be cool to enable writing to the Arduino's serial port using the SIO instructions.
* Using the inexpensive 16x2 LCD might not have been the best choice, as it does not handle rapidly changing characters very well. For instance, the scrolling text of the "SC message" program or the flying ducks in "Duck Shoot". Turning the potentiometer to lower the contrast does help make it a bit more acceptable.

***EDIT:*** It have been pointed out by Retro Phil on the [Science of Cambridge MK14 Microcomputer & NatSemi SC/MP](https://www.facebook.com/groups/1330313924562216) facebook group, that using the OLED version of this display should resolve the issue.

<img src="https://raw.githubusercontent.com/dadecoza/Ardu14/main/Images/LowContrast.gif" alt="Low Contrast" width="300" height="auto">

Example of scrolling text with low contrast.

### PIC14
a Similar and popular PIC based emulator from Karen Orton can be found at [Karen's Corner](http://techlib.com/area_50/Readers/Karen/micro.htm).


