#include <SevenSegmentExtended.h> //the library with functions that print the time
#include <SevenSegmentTM1637.h> 

#include <TimerOne.h> //internal timer
#include "RTClib.h" //RTC clock
#include <Wire.h> //allows I2C communication (used by RTC clock)
#include "pitches.h" //provides all the notes played by the buzzer

//states
#define IDLS (1<<0) // IDLe Status 00000001  
#define EHRS (1<<1) // Editing HouRS 00000010
#define EMIN (1<<2) // Editing MINutes 00000100
#define EAHR (1<<3) // Editing Alarm HouRs
#define EAMI (1<<4) // Editing Alarm MInutes
#define RING (1<<5) // RINGing - disables all buttons except BL
// buttons
#define BB 6 //black button
#define BR 3  //red button
#define BL 2  //switch
#define BUTTONS 2 //polled buttons
// 7-segments
#define CLK 4//pins definitions for TM1637 and can be changed to other ports    
#define DIO 5
#define OFF 0x80 //turns off a digit while leaving the semi-colon on

//buzzer
#define BUZZER 7 //buzzer input pin
#define MELODY_LENGTH 20 //length of the melody in number of notes


byte time_array[] = {0,0};  //read  ISR; buffer for current time values being edited
byte alarm_array[] = {0,0};  // read in ISR, data structure that holds the alarm time
byte showAlarm = 0;  //read in ISR, when 0 show time, when 1 show the alarm
volatile byte armed = 0;  // used and edited in ISR, when 0 the alarm is off, when 1 the alarm is on; volatile because of interrupts 
unsigned char state = IDLS; //current state, here idle is default
byte noblink = 0;  //when 0 is blinking, when 1 - stop blinking for a second
byte blinktimer = 0; // time since the blinking was stopped



SevenSegmentExtended    display(CLK, DIO);  //used in ISR, the display object
RTC_DS1307 RTC;  //used in loop and setup, the RTC clock object
DateTime now;  // read in ISR, the date object

//here we put the melody that will be played when the alarm is on
void playMelody(){
  
  // the notes to be played, as defined in pitches.h
  const int melody[] = {
  NOTE_C4, NOTE_D4, NOTE_F4, NOTE_D4, NOTE_A4, PAUSE, NOTE_A4, NOTE_G4,PAUSE,NOTE_C4,NOTE_D4,NOTE_F4,NOTE_D4,NOTE_G4,PAUSE,NOTE_G4,NOTE_F4,PAUSE,PAUSE,PAUSE
  };

  //the length of each note
  const byte noteDurations[] = {EIGHT_NOTE,EIGHT_NOTE,EIGHT_NOTE,EIGHT_NOTE,EIGHT_NOTE,EIGHT_NOTE,HALF_NOTE,HALF_NOTE,QUARTER_NOTE,EIGHT_NOTE,
  EIGHT_NOTE,EIGHT_NOTE,EIGHT_NOTE,EIGHT_NOTE,EIGHT_NOTE,HALF_NOTE,HALF_NOTE,QUARTER_NOTE,QUARTER_NOTE,QUARTER_NOTE};


  static byte currentNote = 0;                            //note to be played in the current call
  int noteDuration = 1000 / noteDurations[currentNote];  //length of the note
  tone(BUZZER, melody[currentNote], noteDuration);      // playing the note on the buzzer pin
  int pauseBetweenNotes = noteDuration * 1.30;         //calculating an adequate pause prior the next note
  int actualPause = pauseBetweenNotes - noteDuration;
  delay(noteDuration);                           //executing the pause
  noTone(BUZZER);                                    //turning off the buzzer
  digitalWrite(BUZZER, HIGH);
  delay(actualPause);
  currentNote = (++currentNote) % MELODY_LENGTH;    //change index to the next note (the melody will replay using modulo math)

}

//interrupt service routine on PIN 2
// sets the armed flag to enable or disable the alarm
void BLHandler(){
  if(digitalRead(BL) == LOW){
    armed = 1;
  }
  else{
    armed = 0;
  }
}

//function that sets the time on the RTC; notice that for now the date isn't set, and that the second is always 0
void adjustClock(){
  //DateTime (uint16_t year, uint8_t month, uint8_t day,uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
  RTC.adjust(DateTime(2018,1,1,time_array[0],time_array[1],0));
}

//interrupt service routine on Timer1 - executed every 0.5s
void displayTime(){
  
  static int status = 0; //used for blinking, if it's 0 - the selected segments are off, when 1 the segments are on

  switch(state){
  // in IDLE state, we either show the current time or the alarm time, depending on the showAlarm flag
  // in current time view, the semicolon is blinking using the status variable
      case IDLS:
      case RING:
                  if(showAlarm){
                     display.printTime(alarm_array[0],alarm_array[1],true);
                  }else{
                     display.printTime(now.hour(),now.minute(),status);
                  }                              
                  break;
  // in editing alarm hours or current time hours, we blink the 0th and 1st digit from the right
  //the first instruction prints the whole alarm or current time
  // then if status is true and the noblink flag is false, the first 2 digits are turned off
      case EAHR:
                 display.printTime(alarm_array[0],alarm_array[1],true);
                 if(status && !noblink){
                    display.printRaw((uint8_t)OFF,0);
                    display.printRaw((uint8_t)OFF,1);
                 }
                 break;
      case EHRS: 
                 display.printTime(time_array[0],time_array[1],true);
                 if(status && !noblink){
                    display.printRaw((uint8_t)OFF,0);
                    display.printRaw((uint8_t)OFF,1);
                 }
                 break;
    //likewise, but for the 2nd and 3rd digit
      case EAMI:
                 display.printTime(alarm_array[0],alarm_array[1],true);
                 if(status && !noblink){
                    display.printRaw((uint8_t)OFF,2);
                    display.printRaw((uint8_t)OFF,3);
                  }
                  break;
      case EMIN:
                 display.printTime(time_array[0],time_array[1],true);
                 if(status && !noblink){
                    display.printRaw((uint8_t)OFF,2);
                    display.printRaw((uint8_t)OFF,3);
                 }
                 break;
          
  }

  status = (++status) % 2;  //status is being toggled

}


//handles the big red button, receives as argument the reading on the connected PIN (HIGH or LOW) - the button is active HIGH
//this button is tasked to execute state transitions
void BRHandler(byte reading)
{
  static int startpressed = 0,endpressed = 0;  //timers to count how long it has been pressed
  
    if( reading == HIGH){  
       startpressed = millis();      //button has been pressed; we record the time with millis()
    }
    else
    {
      endpressed = millis();        //button has been released; we record the time with millis()
      if(endpressed-startpressed > 1000){  // if the difference between timings is over 1000 msec, then it's a long press
        if(state == IDLS){                    //long presses are valid only if we're in IDLS state
          time_array[0]= now.hour();          //we record the current time on the time_array; this is the starting value to start editing current time
          time_array[1] = now.minute();
          state = EHRS;                      
          } 
      }
      else{
        //state transitions
        switch(state){
          case IDLS:  state = EAHR;  break; 
          case EHRS:  state = EMIN;  break; 
          case EMIN:  adjustClock(); state = IDLS;  break; //here we also write the edited time in time_array to the RTC clock
          case EAHR:  state = EAMI; break; 
          case EAMI:  state = IDLS;  break; 
          case RING: break; //RING has no associated transitions
        }       
      }
    }
    
}

//handles the small black button, receives as argument the reading on the connected PIN (HIGH or LOW) - the button is active LOW
//the button increases numbers - current time or alarm time or toggles between alarm and current time in IDLS state
void BBHandler(byte reading)
{
  if(reading == HIGH) return;  //ignore HIGH readings
  byte *toEdit;       // byte pointer used for editing
  byte mod;                     //byte variable to do modulo math
    switch(state){
      case IDLS: showAlarm = (showAlarm+1) % 2; return;  //toggle alarm and current time, then return
      case EHRS: toEdit = &time_array[0]; mod = 24;  break; // the pointer will now reference the value we want to edit
      case EMIN: toEdit = &time_array[1]; mod = 60;   break; 
      case EAHR: toEdit = &alarm_array[0]; mod = 24;  break; 
      case EAMI: toEdit = &alarm_array[1]; mod = 60;  break; 
      case RING: return;
    }
    *toEdit = ((*toEdit)+1) % mod;  //increase the value referenced by the pointer and apply the modulo
    noblink = 1;                    // set the noblink flag to 1; we don't want to blink the digits we are editing for a while
    blinktimer = now.second();      // start tracking how much time since the last digit editing
    displayTime();                  // display the new values immediately, not waiting till the next interrupt
}

//button polling functions
void pollButtons(){
  byte tmpRead;  //temporary value to store button readings
  const byte buttons[] = {BB,BR}; // button pins to read
  static byte prevButtonStates[] = {HIGH,LOW}; // previous states of buttons, defaulted to inactive values
  for(int i = 0; i < BUTTONS; i++){     //loop each button
    tmpRead = digitalRead(buttons[i]); //read the button pin
    if(tmpRead != prevButtonStates[i]){  //proceed only if the current reading is different than the previous state of button HIGH->LOW or LOW -> HIGH
      switch(i){                       //call the appropriate handles passing the reading
        case 0 :  BBHandler(tmpRead); break;
        case 1 :  BRHandler(tmpRead); break;
      }
    }
    prevButtonStates[i] = tmpRead;    //update the previous button states array
  }
}

//run once, at first
void setup() {
  pinMode(2,INPUT_PULLUP);             //set pins as input, with PULLUP resistor if necessary
  pinMode(3,INPUT);
  pinMode(6,INPUT_PULLUP);
  display.begin();                     // initializes the display
  display.setBacklight(100);           // set the brightness to 100 %
  Timer1.initialize(500000);           //set the timer1 period to 500.000 microseconds
  Timer1.attachInterrupt(displayTime); //attach displayTime as interrupt service routine to Timer1
  Wire.begin();                        // initialize the i2c library
  RTC.begin();                         // start the clock
  RTC.adjust(DateTime(__DATE__, __TIME__)); //initialize the clock with the compilation date and time
  attachInterrupt(digitalPinToInterrupt(2),BLHandler,CHANGE); //attach interrupt on digital PIN 2, use BLHandler as interrupt service routine, trigger the interrupt on every state change
  BLHandler();                          //read initial status of the DPDT button, to record if the alarm is already on
}

//checks whether to start blinking digits again
void checkBlinkTimer(){
  if(blinktimer + 0.5 < now.second()){   //if more than half a second has passed, blink again
    noblink = 0;
  }
}
//checks whether to emit noise
void checkAlarm(){
  // if the armed flag is enabled, the state is IDLS ( if you are editing, or it is already RING state, it will not change state to RING), and the current hour and minute matches the
  // values in the alarm array, the state transitions into RING
 if(armed && state == IDLS && (now.hour() == alarm_array[0] && now.minute() == alarm_array[1])){
  state = RING;
 }else if(!armed && state == RING){  //if the armed flag is disable, but the state is RING (i.e. we are making noise) we turn off the noise
   noTone(BUZZER);                  //stop the tone on the buzzer
   digitalWrite(BUZZER, HIGH);     //the passive buzzer is active LOW, so we also need to write HIGH
   state = IDLS;                  //transition into IDLS once again
 }
  // if the state is RING, play sound (one note)
 if(state == RING){
  playMelody();
 }
}
void loop() {
  pollButtons();  //polls the buttons
  now = RTC.now(); //record the current time from the clock
  checkAlarm();   //check if it's time to play the alarm
  checkBlinkTimer(); //check if it's time to start blinking something again
}
