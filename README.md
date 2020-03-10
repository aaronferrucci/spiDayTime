# spiDayTime
Purpose: provide date and time from an internet-connected Raspberry PI to
another subsystem which lacks internet, but which has an SPI slave.

Installation: depending on your raspbian distribution, you might need to do:
> sudo apt install wiringpi

You probably also need to enable the SPI interface (in raspi-config).

Build the software like so:
> make spiDayTime

