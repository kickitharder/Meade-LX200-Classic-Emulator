# Meade LX200 Classic Emulator

This sketch will work on any Arduino with at least 32K Flash ROM, 2K RAM and 512 byte EEPROM.  It has been tested on the Uno, Nano and Mega.

When an Arduino with the code loaded onto it is plugged into a PC, it will appear as a COM port which can then be picked up by any application which can communicate with a Meade LX200 Classic SCT telescope (LX200).  This includes planetarium software and other telescope control programs which can natively talk to the LX200.  Additionally, ASCOM drivers which have been written for these LX200s, such as Keith Rickard’s Meade LX200 Classic ASCOM Driver, will also work.  The Arduino's onboard LED will light to indicate when the telescope is 'moving' and flash rapidly if in LAND mode.

The Emulator emulates the slewing of the telescope, mount pulse and tracking frequency, and nearly all other commands (see Limitations below).  Settings which are normally remembered by a real LX200 are stored in the Arduino’s EEPROM.

Its main use it to test the development of ASCOM drivers and whether software will work with an LX200 without actually being connected to a real LX200.

The Emulator is still in development – please advise me of any issues you may come across.

Limitations

1)	Currently, the 16” LX200 commands regarding Home Position are emulated but untested with regard to how they work with a real LX200.  These commands are :hS#, :hF#, :hP# and :h#?.  I have not experienced what a real 16” LX200 does in this regard.

2)	The emulator emulates the LX200’s library, but it will not return the same objects in the real library.  Instead, it randomly produces objects randomly (seeded).  There simply is not the space to incorporate data for 110 Messier, 7840 NGC, 5386 IC, and 12921 UGC objects, etc!  The planets are incorporated and their RA and DEC coordinates will be shown randomly (seeded) near the ecliptic.

3)	There is no onboard clock so the emulator’s clock and calendar will have to be updated before use on start-up, if important.

4)	The :Lf# command, which performs a field operation to find objects closest to the centre of field on a real LX200 does not work in the same way on the Emulator. Instead, the emulator will always return the message “Objects:  0”.

5)	The emulator assumes a max slew rate is 8 degrees per second.  For 12” and 16” LX200s, this is 6 and 4 degrees per second respectively, so please bare that in mind.

I have added a few additional commands for the emulator (they are not on the real LX200):

1)  :VE#    Returns the version of the emulator.
2)  :VZ#    Resets the emulator's settings.  The Arduino needs to be reset for this to take effect.
3)  :VR#    Soft reset the Arduino and restart the LX200 emulator
4)  :VS#    Make emulator act as a 7", 8" or 10" LX200
5)  :VM#    Make emulator act as a 12" LX200
6)  :VL#    Make emulator act as a 16" LX200
7)  :VT#    Enter Test Mode
8)  :VA#    Show key variable values

Other features include:

1) Emulation of the LX200's CCD port using pins A0 to A3 on the Arduino to read directional pulse (see source code)
2) Provision of a second serial port, just like on a real LX200's RS232 ports!

Keith Rickard
7 April 2024
