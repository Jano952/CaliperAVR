# CaliperAVR
This is Caliper readout tool for ATMega8A written in Arduino with absolute minimum parts, only 1x ATMega8A and 1x capacitor for VCC ! No other parts are needed, only UART to USB converter for data reading to PC.
Level shifting is done using internal comparator in AVR, which saves external level shifting circuit.

Connection:     

ADC0 (PC0) pin - Caliper Clock pin

ADC1 (PC1) pin - Caliper Data pin

<img width="595" alt="Schematic" src="https://github.com/Jano952/CaliperAVR/assets/104054039/bf42f70d-75cd-47eb-94e8-c2b835e60f8d">


Command set is following:

|Command  |Explanation        |
|---------|-------------------|
|r        | toggle Raw and mm reading|
| a       | toggle Absolute and Relative recieving|
| c       | toggle Continuous mode (send only changes or send periodicaly)|
| pXXXXX  |  Continous mode update period in ms|
| s       | Supress Timeouts|

I used internal oscillator at 8MHz. Built using MiniCore for Arduino. Pins PB0 and PB1 (8 and 9) are used as debug, they mirror the decoded bits from Caliper.
You can change baud rate as you wish. 





TODO: Clean the code, create better readme :)
