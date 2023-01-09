# Meade LX200 Classic Simulator (Beta)

This sketch will work on any Arduino with at least 32K Flash ROM, 2K RAM and 512 byte EEPROM.  It has been tested on the Uno, Nano and Mega.

When an Arduino with the code loaded onto it is plugged into a PC, it will appear as a COM port which can then be picked up by any application which can communicate with a Meade LX200 Classic SCT telescope.  This includes planetarium software and other telescope control programs which can natively talk to the LX200.  Additionally, ASCOM drivers which have been written for these LX200s, such as Keith Rickard’s Meade LX200 Classic ASCOM Driver, will also work.

The simulator emulates the slewing of the telescope, mount pulse and tracking frequency.  Settings which are normally remembered by a real LX200 are stored in the Arduino’s EEPROM.

Its main use it to test the development of ASCOM drivers and whether software will work with an LX200 with actually being connected to the LX200.

This simulator is still in development – please advise me of any issues you may come across.

Limitations

1)	Currently, the 16” LX200 commands regarding Home Position are not simulated.  These commands are :hS#, :hF#, :hP# and :h#?.  I have not experience what an actual 16” LX200 does in regards to these command, but when I have I will probably incorporate them.

2)	The Simulator emulates the LX200’s library, but it will not return the same objects in the real library.  Instead, it randomly produces objects randomly (seeded).  There simply is not the space to incorporate data for 110 Messier, 7840 NGC, 5386 IC, and 12921 UGC objects, etc!  The planets are incorporated and their RA and DEC coordinates will be shown randomly (seeded) near the ecliptic.

3)	There is no onboard clock so simulator’s clock and calendar will have to be updated before use, if important.

4)	The :Lf# command, which performs a field operation to find objects closest to the centre of field on a real LX200 does not work on the Simulator. Instead, the Simulator will always return the message “Objects:  0”.

5)	The Simulator assumes a max slew rate is 8 degrees per second.  For 12” and 16” LX200s, this is 6 and 4 degrees per second, so please bare that in mind.

Keith Rickard
9 January 2023 
