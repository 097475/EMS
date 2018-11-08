# EMS

Everyone needs to know :
- loop()
- setup()

stuff needed to know for Buzzer:
- playMelody()
- checkAlarm()
- pitches.h 

stuff needed to know for 7-segment (aka TM1637)
- displayTime()
- setup() (even more than usual)
- checkBlinkTimer()
- BBHandler() (only the blinktimer part)
- the OFF macro

stuff needed to know for RTC clock:
- setup() and loop() (even more)
- adjustTime()
- BRHandler, BBHandler and displayTime() (only for the parts where the clock is used and why)
- realize that it uses I2C
