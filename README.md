# EMS

Documentation general guidelines : 

- Exactness, e.g. statements like "appropriate", "small/large", "optimal", etc. should be changed to indicate the actual values, parameter range, criterion, etc.
- Completeness, e.g. all project functions are described
- Verifiability, e.g. avoid expressions like "works better", "it is well adjusted", etc.
- Consistency, e.g. the same aspect should not be described in different way in various parts of documentation
- Modifiability, e.g. since programs "live", a good documentation should reflect correctly the program last version
- References, e.g. documentation must provide references or attachments to all the related documents (datasheets, RFC documents, legal acts, source codes and open libraries, and executable software"


Even more general tasks:

- Authors' names, student ID numbers, indicated team leader.
- Each Author's responsibility (who did what?) together with tasks delegated by the team leader.
- User guide.
- Algorithm description - one should also cover the issues related to accuracy, timing choices, message format, etc. 
- Proof of the correct program operation (printscreens, pictures, packet analyzer logs, etc.).
- In the documentation one should provide FMEA/FMECA (Failure Mode and Effect Analysis/Failure Mode and Criticality Analysis).
- The description of extra hardware incorporated by the group together with datasheets (in separate files) and explanation of design choices.
- The functionality description of incorporated elements. The description should cover MCU and peripheral registers, not the library functions.
- Clear specification of the incorporated functionalities.


for FMEA/FMECA, see: http://skl.it.p.lodz.pl/~ignaciuk/pliki/materialy/ES/documentation.html

for sample report, see: http://skl.it.p.lodz.pl/~ignaciuk/pliki/materialy/ES/Report_1.pdf

http://skl.it.p.lodz.pl/~ignaciuk/pliki/materialy/ES/es_lab.html


Specific tasks:
- Introduction
- Who did what
- User guide
- Description of each component and each interface (e.g RTC and I2C, matrix and SPI, etc.) + interrupts
  - Buttons + how does arduino reads buttons, debouncing, etc.
  - Led display + pseudo i2c
  - RTC + i2c
  - led matrix (max7219) + spi
  - DHT + Single Wire Two Way protocol
  - nokia display + its protocol (??)
  - joystick + its protocol (???)
  - light sensor + its protocol
  - Interrupts (timed and non-timed)
- Description of the Finite State Machine
- Description of the complete algorithm (with subsections of course)
- FMEA
- Further improvements and/or limitations

https://manytools.org/hacker-tools/image-to-byte-array/
https://forum.arduino.cc/index.php?topic=346356.0
