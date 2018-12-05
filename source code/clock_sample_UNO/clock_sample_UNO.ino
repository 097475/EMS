#include <LedControl.h>  // LED matrix library
#include <SimpleDHT.h>  // DHT library
#include <SevenSegmentExtended.h> //7-seg libraries
#include <SevenSegmentTM1637.h>
#include <TimerOne.h> //internal timer
#include <RTClib.h> //RTC clock
#include <Wire.h> //allows I2C communication 
#include <Adafruit_GFX.h> // graphics library
#include <Adafruit_PCD8544.h> //nokia display library
#include <Entropy.h> //for random numbers
#include "pitches.h" //provides all the notes played by the buzzer

//states
#define IDLS (1<<0) // IDLe Status 00000001  
#define EHRS (1<<1) // Editing HouRS 00000010
#define EMIN (1<<2) // Editing MINutes 00000100
#define EAHR (1<<3) // Editing Alarm HouRs
#define EAMI (1<<4) // Editing Alarm MInutes
#define EYRS (1<<5) // Editing YeaRS
#define EMTH (1<<6) // Editing MonTHs
#define EDAY (1<<7) // Editing DAYs
#define RING (1<<8) // RINGing 
// buttons
#define BB1 0 //black button 1
#define BB2 1 //black button 2
#define BR 3  //red button
#define BL 2  //switch
#define BUTTONS 3 //polled buttons
// 7-segments
#define CLK 4  
#define DIO 5
#define OFF 0x80 //turns off a digit while leaving the semi-colon on
//buzzer
#define BUZZER 6 
#define MELODY_LENGTH 20 
// DHT sensor
#define DHTPIN A0
// animations
#define IDLSANIM 0
#define ETANIM 1
#define EYRANIM 2
#define EMTHANIM 3
#define EDAYANIM 4
#define RINGANIM 5
//animation settings
#define MATRIXLEN 8
#define ANIMLEN 4
#define REPS 3
//photoresistor pin
#define PHOTORES A3
//LED matrix pins
#define LDIN 7
#define CS 8
#define LCLK 9
//nokia display pins
#define RST 10
#define DC 11
#define DIN 12
#define SCLK 13
//joystick pins
#define OX A2
#define OY A1
//game settings
#define HOWMANY 4 //how many arrows
#define TIMEFORGOODANSWER 200 // in milisecound time when one dot so for move you have TIMEFORGOODANSWER*3

SevenSegmentExtended    display(CLK, DIO); 
LedControl lc = LedControl(LDIN, LCLK, CS, 1);
SimpleDHT11 dht11(DHTPIN);
RTC_DS1307 RTC;  
DateTime now;  
Adafruit_PCD8544 nokiaDisplay = Adafruit_PCD8544(SCLK, DIN, DC, RST);
byte dateArray[] = {18, 1, 1}; //buffer for current date values being edited
byte timeArray[] = {0, 0}; // buffer for current time values being edited
byte alarmArray[] = {0, 0}; // data structure that holds the alarm time
boolean showAlarm = false;  // when false show time, when true show the alarm
volatile boolean armed = false;  // when false the alarm is off, when true the alarm is on; volatile because of interrupts
short state = IDLS; //current state, here idle is default
boolean noBlink = false;  //when false is blinking, when true - stop blinking for a second
unsigned long blinkTimer = 0; // time since the blinking was stopped
byte currentNote = 0;//note to be played in the current call
byte rep = REPS; // number of animation repetitions elapsed, when equal to REPS no animation is played
byte currImage = 0;
byte currentAnimation = IDLSANIM;
//animations
const byte animations[][4][8] = {
  {{0x0, 0xe4, 0xa4, 0xee, 0x84, 0x87, 0x0, 0x0}, {0x0, 0xe4, 0xa4, 0xee, 0x84, 0x87, 0x0, 0xc0}, {0x0, 0xe4, 0xa4, 0xee, 0x84, 0x87, 0x0, 0xd8}, {0x0, 0xe4, 0xa4, 0xee, 0x84, 0x87, 0x0, 0xdb}},
  {{0x3c, 0x42, 0x91, 0x91, 0x9f, 0x81, 0x42, 0x3c}, {0x3c, 0x42, 0x91, 0x91, 0x91, 0x91, 0x52, 0x3c}, {0x3c, 0x42, 0x91, 0x91, 0xf1, 0x81, 0x42, 0x3c}, {0x3c, 0x52, 0x91, 0x91, 0x91, 0x81, 0x42, 0x3c}},
  {{0xae, 0xa8, 0xa8, 0xee, 0x48, 0x48, 0x48, 0x4e}, {0x4e, 0xaa, 0xaa, 0xee, 0xac, 0xaa, 0xaa, 0xaa}, {0x64, 0x24, 0xa5, 0x0, 0xff, 0xff, 0x7e, 0x3c}, {0x88, 0xeb, 0x2a, 0x0, 0xff, 0xff, 0x7e, 0x3c}},
  {{0x0, 0x0, 0x6c, 0x54, 0x44, 0x44, 0x44, 0x0}, {0x0, 0x0, 0x7c, 0x44, 0x44, 0x44, 0x7c, 0x0}, {0x0, 0x0, 0x44, 0x64, 0x54, 0x4c, 0x44, 0x0}, {0x0, 0x0, 0x0, 0xea, 0x4e, 0x4a, 0x4a, 0x0}},
  {{0x0, 0x38, 0x24, 0x24, 0x24, 0x38, 0x0, 0x0}, {0x0, 0x3c, 0x24, 0x3c, 0x24, 0x24, 0x0, 0x0}, {0x0, 0x24, 0x24, 0x3c, 0x4, 0x3c, 0x0, 0x0}, {0x89, 0x5a, 0x24, 0x42, 0x42, 0x24, 0x5a, 0x81}},
  {{0x0, 0x0, 0x24, 0x0, 0x18, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x24, 0x0, 0x3c, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x24, 0x0, 0x3c, 0x24, 0x3c, 0x0}, {0x0, 0x0, 0x24, 0x0, 0x7e, 0x42, 0x7e, 0x0}}
};
// the notes to be played, as defined in pitches.h
const int melody[] = {
    NOTE_C4, NOTE_D4, NOTE_F4, NOTE_D4, NOTE_A4, PAUSE, NOTE_A4, NOTE_G4, PAUSE, NOTE_C4, NOTE_D4, NOTE_F4, NOTE_D4, NOTE_G4, PAUSE, NOTE_G4, NOTE_F4, PAUSE, PAUSE, PAUSE
};

//the length of each note
const byte noteDurations[] = {EIGHT_NOTE, EIGHT_NOTE, EIGHT_NOTE, EIGHT_NOTE, EIGHT_NOTE, EIGHT_NOTE, HALF_NOTE, HALF_NOTE, QUARTER_NOTE, EIGHT_NOTE,
                                EIGHT_NOTE, EIGHT_NOTE, EIGHT_NOTE, EIGHT_NOTE, EIGHT_NOTE, HALF_NOTE, HALF_NOTE, QUARTER_NOTE, QUARTER_NOTE, QUARTER_NOTE
};
byte currentArrow = 0; //which arrow is processed right now
byte numberDots = 3; //how many dots there gonna be
unsigned long timeChecker; //timer
bool comeBackToZero = true; //after every good answer you have to come back to 0 position
bool winTheGame  = false; // global
byte arrows[HOWMANY]; // generaly arrows are representet by numbers 0 up 1 down 2 left 3 right

// prints an 8bit image on the LED matrix
void writeByteArray(byte *src){
  for (int i = 0; i < MATRIXLEN; i++) {
    lc.setRow(0, i, src[i]);
  }
}

//starts the animations by setting the elapsed repetitions to 0 - the animation is repeated till REPS
void startAnimation(){
  rep = 0;
  currImage = 0;
  lc.shutdown(0,false);
}

// runs the next image in the animation
void animate(byte (*src)[8]){
  writeByteArray(src[currImage]);
  currImage = (++currImage) % ANIMLEN;
  if (currImage == 0){
    rep++;
  }
}

//here we put the melody that will be played when the alarm is on
void playMelody() {
  static long notePause = 0, betweenPause = 0;
  static bool isPlaying = false, isWaiting = false;
  int noteDuration,pauseBetweenNotes,actualPause,curr;
  
   if (!isPlaying){ //running a new note
        noteDuration = 1000 / noteDurations[currentNote];  //length of the note
        pauseBetweenNotes = noteDuration * 1.30;         //calculating an adequate pause prior the next note
        actualPause = pauseBetweenNotes - noteDuration;
        curr = melody[currentNote];
    if (curr != PAUSE)
      tone(BUZZER, melody[currentNote], noteDuration);      // playing the note on the buzzer pin
    notePause = millis();
    isPlaying = true;
  }
  
  if (notePause + noteDuration > millis()) //waiting for the note to finish
      return; 
        
  noTone(BUZZER); //shutting off the buzzer                                    
  digitalWrite(BUZZER, HIGH);
  
  if (!isWaiting) { //after the note is finished, we wait a pause
    betweenPause = millis();
    isWaiting = true;
  }
  
  if (betweenPause + actualPause > millis()) //waiting the minimum pause between two notes
      return;


  currentNote = (++currentNote) % MELODY_LENGTH;    //change index to the next note (the melody will replay using modulo math)
  isPlaying = false;
  isWaiting = false;

}

//interrupt service routine on PIN 2
// sets the armed flag to enable or disable the alarm
void BLHandler() {
  if (digitalRead(BL) == LOW) {
    armed = true;
  }
  else {
    armed = false;
  }
}

//function that sets the time on the RTC; notice that for now the date isn't set, and that the second is always 0
void adjustClock() {
  //DateTime (uint16_t year, uint8_t month, uint8_t day,uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
  RTC.adjust(DateTime(dateArray[0] + 2000, dateArray[1], dateArray[2], timeArray[0], timeArray[1], 1));
}

//interrupt service routine on Timer1 - executed every 0.5s
void displayTime() {
  static byte status = 0; //used for blinking, if it's 0 - the selected segments are off, when 1 the segments are on
  switch (state) {
    // in IDLE state, we either show the current time or the alarm time, depending on the showAlarm flag
    // in current time view, the semicolon is blinking using the status variable
    case IDLS:
    case RING:
      if (showAlarm) {
        display.printTime(alarmArray[0], alarmArray[1], true);
      } else {
        display.printTime(now.hour(), now.minute(), status);
      }
      break;
    // in editing alarm hours or current time hours, we blink the 0th and 1st digit from the right
    //the first instruction prints the whole alarm or current time
    // then if status is true and the noBlink flag is false, the first 2 digits are turned off
    case EAHR:
      display.printTime(alarmArray[0], alarmArray[1], true);
      if (status && !noBlink) {
        display.printRaw((uint8_t)OFF, 0);
        display.printRaw((uint8_t)OFF, 1);
      }
      break;
    case EHRS:
      display.printTime(timeArray[0], timeArray[1], true);
      if (status && !noBlink) {
        display.printRaw((uint8_t)OFF, 0);
        display.printRaw((uint8_t)OFF, 1);
      }
      break;
    //likewise, but for the 2nd and 3rd digit
    case EAMI:
      display.printTime(alarmArray[0], alarmArray[1], true);
      if (status && !noBlink) {
        display.printRaw((uint8_t)OFF, 2);
        display.printRaw((uint8_t)OFF, 3);
      }
      break;
    case EMIN:
      display.printTime(timeArray[0], timeArray[1], true);
      if (status && !noBlink) {
        display.printRaw((uint8_t)OFF, 2);
        display.printRaw((uint8_t)OFF, 3);
      }
      break;
    default:   display.printTime(timeArray[0], timeArray[1], true); break;

  }
  status = (++status) % 2;  //status is being toggled
}


//handles the big red button, receives as argument the reading on the connected PIN (HIGH or LOW) - the button is active HIGH
//this button is tasked to execute state transitions
void BRHandler(byte reading){
  static long startpressed = 0, endpressed = 0; //timers to count how long it has been pressed

  if ( reading == HIGH) {
    startpressed = millis();      //button has been pressed; we record the time with millis()
  }
  else
  {
    if (startpressed + 1000 < millis()) { // if the difference between timings is over 1000 msec, then it's a long press
      if (state == IDLS) {                  //long presses are valid only if we're in IDLS state
        timeArray[0] = now.hour();          //we record the current time on the timeArray; this is the starting value to start editing current time
        timeArray[1] = now.minute();
        dateArray[0] = now.year() - 2000;
        dateArray[1] = now.month();
        dateArray[2] = now.day();
        state = EHRS;
        currentAnimation = ETANIM;
        startAnimation();
      }
    }
    else {
      //state transitions
      switch (state) {
        case IDLS:  state = EAHR; currentAnimation = ETANIM; break;
        case EHRS:  state = EMIN; currentAnimation = ETANIM; break;
        case EMIN:  state = EYRS; currentAnimation = EYRANIM;break; 
        case EYRS:  state = EMTH; currentAnimation = EMTHANIM;break;
        case EMTH:  state = EDAY; currentAnimation = EDAYANIM;break;
        case EDAY:  adjustClock(); state = IDLS; currentAnimation = IDLSANIM; break; //here we also write the edited time in timeArray to the RTC clock
        case EAHR:  state = EAMI; currentAnimation = ETANIM; break;
        case EAMI:  state = IDLS;  currentAnimation = IDLSANIM; break;
        case RING: return; //RING has no associated transitions
      }
      startAnimation();
    }
  }
}

//handles the small black button, receives as argument the reading on the connected PIN (HIGH or LOW) - the button is active LOW
//the button increases numbers - current time or alarm time or toggles between alarm and current time in IDLS state
void BBHandler(byte reading, char direction){
  if (reading == HIGH) 
      return; //ignore HIGH readings
  byte *toEdit;       // byte pointer used for editing
  byte mod;                     //byte variable to do modulo math
  byte fromOne = 0;
  switch (state) {
    case IDLS:
    case RING: showAlarm = (showAlarm + 1) % 2; return; //toggle alarm and current time, then return
    case EHRS: toEdit = &timeArray[0]; mod = 24;  break; // the pointer will now reference the value we want to edit
    case EMIN: toEdit = &timeArray[1]; mod = 60;   break;
    case EAHR: toEdit = &alarmArray[0]; mod = 24;  break;
    case EAMI: toEdit = &alarmArray[1]; mod = 60;  break;
    case EYRS: toEdit = &dateArray[0]; mod = 100; break;
    case EMTH: toEdit = &dateArray[1]; mod = 12; fromOne = 1; break;
    case EDAY: toEdit = &dateArray[2]; mod = 31; fromOne = 1; break;//additional checks
  }
  //correct month handling
  if (state == EDAY) {
    switch (dateArray[1]) {
      case 2: !((dateArray[0] + 2000) % 4) && !(!((dateArray[0] + 2000) % 100) && ((dateArray[0] + 2000) % 400)) ?  mod = 29 : mod = 28; break;
      case 4:
      case 6:
      case 9:
      case 11: mod = 30; break;
      default: break;
    }
  }
  // to make modulo math consistent backwards
  if (direction == -1 && *toEdit == 0){
    *toEdit = mod;
  }else if(fromOne && direction == -1 && *toEdit == 1){
    *toEdit = mod+1;
  }

  *toEdit = (((*toEdit) + direction - fromOne) % mod) + fromOne ; //increase the value referenced by the pointer and apply the modulo
  noBlink = 1;                    // set the noBlink flag to 1; we don't want to blink the digits we are editing for a while
  blinkTimer = millis();     // start tracking how much time since the last digit editing
  displayTime();                  // display the new values immediately, not waiting till the next interrupt
  displayDate();
}

//button polling functions
void pollButtons() {
  static long debounce[BUTTONS] = {0, 0, 0};
  byte tmpRead;  //temporary value to store button readings
  const byte buttons[] = {BB1, BB2, BR}; // button pins to read
  static byte prevButtonStates[] = {HIGH, HIGH, LOW}; // previous states of buttons, defaulted to inactive values
  static byte prevProcStates[] = {HIGH, HIGH, LOW}; // previous states of buttons, defaulted to inactive values
  for (int i = 0; i < BUTTONS; i++) {   //loop each button
    tmpRead = digitalRead(buttons[i]); //read the button pin
    if (tmpRead != prevButtonStates[i]) { //proceed only if the current reading is different than the previous state of button HIGH->LOW or LOW -> HIGH
      debounce[i] = millis();
      prevButtonStates[i] = tmpRead;    //update the previous button states array
    }
    if (debounce[i] + 25 < millis() && tmpRead != prevProcStates[i]){
      switch (i) {                     //call the appropriate handles passing the reading
        case 0 : BBHandler(tmpRead, 1); break;
        case 1 : BBHandler(tmpRead, -1); break;
        case 2 : BRHandler(tmpRead); break;
      }
      prevProcStates[i] = tmpRead;
    }
  }
}


void giveValues() { //besically we create random numbers (0 1 2 3) that gonna be representet by arrows
  for (int i = 0 ; i < HOWMANY; i++){
    arrows[i] = random(0, 4); // four signs <- -> ^ .
  }
}


void drawSequence(int y) { //to display 4 generater arrows on the screen
  int x = 12;
  for (int i = 0; i < HOWMANY; i++) {
    if (arrows[i] == 0)
      drawArrow(x, y, 0, 4, 0); //drow arrow 0 means up 4 is a size and 0 means arrow is black
    if (arrows[i] == 1)
      drawArrow(x, y, 1, 4, 0); //drow arrow 1 means down 4 is a size and 0 means arrow is black
    if (arrows[i] == 2)
      drawArrow(x, y, 2, 4, 0); //drow arrow 2 means left 4 is a size and 0 means arrow is black
    if (arrows[i] == 3)
      drawArrow(x, y, 3, 4, 0); //drow arrow 3 means roght 4 is a size and 0 means arrow is black
    x += 15; // width and legth of the arrow is 9 so we bassicaly making some gap beetwen them
  }
}

void clearArrow() { // clear arrow that is currently proccessed
  drawArrow(12 + 15 * currentArrow, 20, arrows[currentArrow] , 4, 1);
}
void redrawArrow() { //if user fails to make good move in proper time draw again arrow 
  drawArrow(12 + 15 * currentArrow, 20, arrows[currentArrow] , 4, 0);
}

void drawArrow(int x, int y, int direction_, int size_, int color_) { // function responsible for drowing arrows on the screen
  byte color = 1;
  if (color_ == 0)
    color = 0xFFFF; //BLACK
  else
    color = 0x0000; //WHITE

  if (direction_ == 0 || direction_ == 1) {
    nokiaDisplay.writeLine(x, y - size_, x, y + size_, color);
    if (direction_ == 0) {
      nokiaDisplay.writeLine(x, y - size_, x + size_ , y, color);
      nokiaDisplay.writeLine(x, y - size_, x - size_, y, color);
    }
    else {
      nokiaDisplay.writeLine(x, y + size_, x + size_ , y, color);
      nokiaDisplay.writeLine(x, y + size_, x - size_, y, color);
    }
  }
  else {
    nokiaDisplay.writeLine(x - size_, y, x + size_, y, color);
    if (direction_ == 2){
      nokiaDisplay.writeLine(x - size_, y , x, y - size_, color);
      nokiaDisplay.writeLine(x - size_ , y , x , y + size_, color);
    }
    else{
      nokiaDisplay.writeLine(x + size_, y, x , y + size_, color);
      nokiaDisplay.writeLine(x + size_, y, x, y - size_, color);
    }
  }
  nokiaDisplay.display();
}



void dotsDisplay(int numberDots) { //its responsible for drowing dots
  for (int i = 0; i < numberDots; i++) { //drows dots that left in "black"
    nokiaDisplay.drawRect(53 + 12 * i , 41, 3, 3, 0xFFF);
    nokiaDisplay.fillRect(53 + 12 * i , 41, 3, 3, 0xFFF);
  }
  for (int i = numberDots; i < 3; i++) { //rest of the dors make "white"
    nokiaDisplay.drawRect(53 + 12 * i , 41, 3, 3, 0x0000);
    nokiaDisplay.fillRect(53 + 12 * i , 41, 3, 3, 0x0000);
  }
  nokiaDisplay.display();
}

void changeNumberDots() { // if dots number gonna be less or equal zero change current arrow and come back one step not 
  if (currentArrow > 0){ // disactive in case of first arrow becouse we can move anywhere
    if (numberDots <= 0) {
      currentArrow--;
      getCurrentArrow();
      redrawArrow();
      timeChecker = millis();
      numberDots = 3;
      dotsDisplay(numberDots);
    }
  }
}


bool checkWin() { // check if winning conditions are held
  if (currentArrow > 3 && winTheGame == false) {
    dotsDisplay(0);
    nokiaDisplay.clearDisplay();
    printText(" You win! ", 11, 17, 1);
    winTheGame = true;
    return true;
  }
  if (winTheGame == false)
    return false;
}


void changeOX() { // check if there was a proper movement by joystick
  if (arrows[currentArrow] == 2) { // if currnet arrow is up and you make proper movement
    if (analogRead(OX) < 24) { //we dont have zero becouse someone can no ideally go to zero postition so it's error margin
      timeChecker = millis(); // we restart time after which first dot disapper
      numberDots = 3; // we restart number of dots player once again has three
      dotsDisplay(numberDots); // we display 3 dots
      clearArrow(); //we blank the arrow becouse we make coorect movement
      currentArrow++; // we go to the next arrow
      if (currentArrow < 4) // if there are any arrow left
        getCurrentArrow(); //display the proper text on the screen so for example "left"
      comeBackToZero = false; //so bassically you have to go to zero position to have chance to ansewr to another arrow
    }
  }
  if (arrows[currentArrow] == 3) {
    if (analogRead(OX) > 1000) {
      timeChecker = millis();
      numberDots = 3;
      dotsDisplay(numberDots);
      clearArrow();
      currentArrow++;
      if (currentArrow < 4)
        getCurrentArrow();
      comeBackToZero = false;
    }
  }
}
void changeOY() {
  if (arrows[currentArrow] == 0) {
    if (analogRead(OY) > 1000) {
      timeChecker = millis();
      numberDots = 3;
      dotsDisplay(numberDots);
      clearArrow();
      currentArrow++;
      if (currentArrow < 4)
        getCurrentArrow();
      comeBackToZero = false;
    }

  }
  if (arrows[currentArrow] == 1) {
    if (analogRead(OY) < 24) {
      timeChecker = millis();
      numberDots = 3;
      dotsDisplay(numberDots);
      clearArrow();
      currentArrow++;
      if (currentArrow < 4)
        getCurrentArrow();
      comeBackToZero = false;
    }
  }
}

void zeroState() { //so if user is in more less zero position in error margin we set flag to true and he can make onther move 
  if (analogRead(OX) < 600 && analogRead(OX) > 400 && analogRead(OY) < 600 && analogRead(OY) > 400) {
    comeBackToZero = true;
  }
}

void prepareText(int x_, int y_, int end_x, int end_y) { // becouse text is white we need to have black space behinf them
  nokiaDisplay.drawRect(x_, y_, end_x - x_, end_y - y_, 1);
  nokiaDisplay.fillRect(x_, y_, end_x - x_,  end_y - y_, 1);
}


void getCurrentArrow() { // print current arrow for instance "left" and so n
  if ( arrows[currentArrow] == 0) {
    printText(" UP   ", 2, 40, 1);
  }
  if ( arrows[currentArrow] == 1) {
    printText(" DOWN ", 2, 40, 1);
  }
  if ( arrows[currentArrow] == 2) {
    printText(" LEFT ", 2, 40, 1);
  }
  if ( arrows[currentArrow] == 3) {
    printText(" RIGHT ", 2, 40, 1);
  }
}


void drawAPixel(int x, int y) { //draw one black pixel
  nokiaDisplay.drawPixel(x, y, BLACK);
}



void printArena() { //we just display the frame for the game
  for (int temp_y = 10; temp_y < 48; temp_y += 37) {
    for (int temp_x = 0; temp_x < 84; temp_x++)
    {
      drawAPixel(temp_x, temp_y);
      if (temp_y == 0)
        drawAPixel(temp_x, temp_y + 1);
      else
        drawAPixel(temp_x, temp_y - 1);
    }
  }
  for (int temp_x_ = 0; temp_x_ < 84; temp_x_ += 83) {
    for (int temp_y_ = 10; temp_y_ < 48; temp_y_++)
    {
      drawAPixel(temp_x_, temp_y_);
      if (temp_x_ == 0)
        drawAPixel(temp_x_ + 1, temp_y_);
      else
        drawAPixel(temp_x_ - 1, temp_y_);
    }
  }
  nokiaDisplay.display();
}


void printText(String text, int x, int y, int color) { // we print the text 
  if (color == 0)
    color = 0xFFFF; //BLACK
  else
    color = 0x0000; //WHITE
  const unsigned int len = text.length();
  char table_text[len];
  text.toCharArray(table_text, text.length()); // this dislay only display char
  for (int i = 0; i < text.length(); i++) {
    nokiaDisplay.drawChar(x + 6 * i , y, table_text[i], color, 1, 1);
  }
  nokiaDisplay.display();
}

void initGame(){
  currentArrow = 0; //which arow is proccessed right now
  comeBackToZero = true; //after every good answer you have to come back to 0 position
  winTheGame  = false; // global
  nokiaDisplay.clearDisplay();
  printArena();
  giveValues();
  drawSequence(20); // y = 20
  prepareText(0, 0, 83, 9); // make black under text
  printText(" make seq ", 2, 2, 1); //print text
  prepareText(0, 38, 50, 47); // this is black box on the bottom
  getCurrentArrow(); // print message about direction of the arrow
  timeChecker = millis(); // time checker for dots if this is time for disapper
  dotsDisplay(numberDots); // display 3 dots
}

void runGame()
{
  if (winTheGame == false) {
    if (comeBackToZero == true) { //if there is zero state we can answer
      changeOX();
      changeOY();
    }
    zeroState();//check if zero state
    changeNumberDots(); 
    if (!checkWin()) {
      if (millis() - timeChecker > TIMEFORGOODANSWER) { //if time left dot dissaper
        if (numberDots > 0 && currentArrow > 0){ //for first arrow nothing happens to the dots
          numberDots--; 
          dotsDisplay(numberDots);
        }
        timeChecker = millis(); //we reset the timer
      }
    }
  }
}

//checks whether to start blinking digits again
void checkblinkTimer() {
  noBlink = 0;
}
//checks whether to emit noise
void checkAlarm() {
  // if the armed flag is enabled, the state is IDLS ( if you are editing, or it is already RING state, it will not change state to RING), and the current hour and minute matches the
  // values in the alarm array, the state transitions into RING
  if (armed && state == IDLS && (now.hour() == alarmArray[0] && now.minute() == alarmArray[1])) {
    state = RING; //state transition
    currentNote = 0; //start melody from first note
    currentAnimation = RINGANIM; //run animation
    startAnimation();
    initGame(); //initialize the game
  } else if (!armed && winTheGame && state == RING) { //if the armed flag is disable, but the state is RING (i.e. we are making noise) we turn off the noise
    noTone(BUZZER);                  //stop the tone on the buzzer
    digitalWrite(BUZZER, HIGH);     //the passive buzzer is active LOW, so we also need to write HIGH
    state = IDLS;                  //transition into IDLS once again
    nokiaDisplay.clearDisplay();   //clean the display
    displayDate();                //put the typical display values
    readDHT();
    nokiaDisplay.display();
  }
  // if the state is RING, play sound (one note)
  if (state == RING) {
    playMelody(); //run melody
    runGame();    // run game
  }
}
void displayDate()
{
  if (state == RING)
      return;
  char buf [4];
  nokiaDisplay.setCursor(12, 0);
  nokiaDisplay.print("                              ");
  nokiaDisplay.setCursor(12, 0);
  if (state == IDLS || state == EAHR || state == EAMI || state == RING) {
    nokiaDisplay.print(now.year());
    nokiaDisplay.print("/");
    sprintf(buf, "%02d", now.month());
    nokiaDisplay.print(buf);
    nokiaDisplay.print("/");
    sprintf(buf, "%02d", now.day());
    nokiaDisplay.print(buf);
  }
  else
  {
    nokiaDisplay.print(dateArray[0] + 2000);
    nokiaDisplay.print("/");
    sprintf(buf, "%02d", dateArray[1]);
    nokiaDisplay.print(buf);
    nokiaDisplay.print("/");
    sprintf(buf, "%02d", dateArray[2]);
    nokiaDisplay.print(buf);
  }
  nokiaDisplay.display();
}
void readDHT() {
  if (state == RING)
    return;
  byte temp, hum;
  dht11.read(&temp, &hum, NULL);
  nokiaDisplay.setCursor(12, 24);
  nokiaDisplay.print("                              ");
  nokiaDisplay.setCursor(12, 24);
  nokiaDisplay.print(temp);
  nokiaDisplay.print("*C ");
  nokiaDisplay.print(hum);
  nokiaDisplay.print("%RH");
  nokiaDisplay.display();
}

//run once, at first
void setup() {
  pinMode(BB1, INPUT_PULLUP);            //set pins as input, with PULLUP resistor if necessary
  pinMode(BL, INPUT_PULLUP);
  pinMode(BB2, INPUT_PULLUP);
  display.begin();                     // initializes the display
  display.setBacklight(100);           // set the brightness to 100 %
  Timer1.initialize(500000);           //set the timer1 period to 500.000 microseconds
  Timer1.attachInterrupt(displayTime); //attach displayTime as interrupt service routine to Timer1
  Wire.begin();                        // initialize the i2c library
  RTC.begin();                         // start the clock
  RTC.adjust(DateTime(__DATE__, __TIME__)); //initialize the clock with the compilation date and time
  now = RTC.now();                      //get initial clock values
  attachInterrupt(digitalPinToInterrupt(2), BLHandler, CHANGE); //attach interrupt on digital PIN 2, use BLHandler as interrupt service routine, trigger the interrupt on every state change
  BLHandler();                          //read initial status of the DPDT button, to record if the alarm is already on
  nokiaDisplay.begin();                 //initialize nokia display
  nokiaDisplay.setContrast(30);
  nokiaDisplay.setTextColor(0xFFFF, 0x0000);
  nokiaDisplay.clearDisplay();
  nokiaDisplay.display();
  displayDate();  
  readDHT();
  Entropy.initialize(); //initialize random number generator
  randomSeed( Entropy.random());
  delay(2000); //run a short delay
  startAnimation();
}

void loop() {
  static long sequenceDelay = 0;
  static long secondPeriod = 0;
  static bool screenUpdated = false;
  now = RTC.now(); //record the current time from the clock
  byte second = now.second(); //read current second
  long curr = millis(); //read current millisecond
  
  pollButtons();  //polls the buttons
  checkAlarm();   //check if it's time to play the alarm
  
  // animation handler
  if (sequenceDelay + 500 < curr){
    if( rep < REPS){
      animate(animations[currentAnimation]);
      sequenceDelay = curr;
    }
    else{
      lc.shutdown(0,true);
    }
  }
  // blinking handler
  if (blinkTimer + 1000 < curr){
    noBlink = 0;
  }
  //photoresistor handler, also resets screenUpdated flag
  if (secondPeriod + 1000 < curr){
    short phread = analogRead(PHOTORES);
    lc.setIntensity(0, map(phread, 0, 1023, 0, 15)); //0 - 15
    display.setBacklight(map(phread, 0, 1023, 10, 100)); // 0 -100
    secondPeriod = curr;
    if (second > 0) {
      screenUpdated = false;
    }
  }
  //nokia display refresh
  if (screenUpdated == false && second == 0){
    if(now.hour() == 0 && now.minute() == 0){
       displayDate();
    }
    readDHT();
    screenUpdated = true;
  }
}
