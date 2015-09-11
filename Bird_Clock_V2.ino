#include <Servo.h> 
#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 RTC;


// START USER SETUP //
// TO SET THE CLOCK, Go below to USER CLOCK SET
// TO DEBUG - uncomment all the serial print statements!

int DSTfactor = 1;      // DST converstion Factor - (1) add an hour or (-1) subtract an hour

int door_open = 5;      // Range of motion - Min
int door_closed = 157;  // Range of motion - Max
int door_speed = 2;     // Lower numbers are faster (0-9)

int bird_in = 5;        // Range of motion - Min
int bird_out = 135;     // Range of motion - Max
int bird_speed = 1;     // Lower numbers are faster (0-9)
int bird_hold = 1000;   // How long bird is out, in ms.

int clock_min = 4;      // Range of motion - Min
int clock_max = 131;    // Range of motion - Max
int clock_speed = 4;    // Lower numbers are faster (0-9)

// END USER SETUP //

//  PIN ASSIGMNMENTS //

//Wire Library Needs:
//SDA (data line) is on analog input pin 4
//SCL (clock line) is on analog input pin 5

Servo clock; // D3
Servo door;  // D5
Servo bird;  // D6
const int switch_dst = A0;
const int switch_demo = A1; 
const int secondsLED = A2; 



// START tracking variables //
int birdPosition = 0;  
int doorPosition = 0; 
int clockPosition = 0;
boolean chirp = false;
int reps = 0; // used to do math to convert 24 hrs to 12 hrs
int hourMin = 1; // used to find current minutes elapsed in a 12 hour period
int currentHour=0; // used as the current hour afer DST calc
int hourMinPrev;
int clockPositionPrev=(clock_min+1);
// END tracking variables //


void setup() 
{ 
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  
  clock.attach(3);      // Attach to Digital Pin
  door.attach(5);       // Attach to Digital Pin
  bird.attach(6);       // Attach to Digital Pin
  pinMode(switch_dst, INPUT); 
  pinMode(switch_demo, INPUT);  
  pinMode(secondsLED, OUTPUT);


/////// USER SET CLOCK /////////////
//  Remove the comment line below, upload the sketch, then comment and upload sketch again
//RTC.adjust(DateTime(__DATE__, __TIME__));
/////// END USER SET CLOCK /////////////


  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running! Adjusting to compile time.");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  DateTime now = RTC.now();
  if (now.year()==2000 && now.month()==0 && now.day()==0) {
    // following lines give visual display that the RTC is running, but not set
    Serial.println("RTC is running BUT NOT SET");
    sweepClock (clock_min);
    sweepClock (clock_max);
    sweepClock (clock_min);
    sweepClock (clock_max);
    sweepClock (clock_min);
    sweepClock (clock_max);
  }

  
  else {
    Serial.println("RTC is running - be cool");
    printTime();
  }
    
  //  This puts all objects at starting positions //
  Serial.println("START - Setting Start Positions");
  sweepBird(bird_out);      
  birdPosition = bird_out;
  delay (200);
  sweepDoor(door_closed); 
  doorPosition = door_closed;
  sweepClock (clock_min);
  Serial.println("END - Setting Start Positions");

} // END SETUP


void loop() 
{ 
  DateTime now = RTC.now();  
  
  checkDemo ();

  if (now.minute() !=  hourMinPrev) {
    Serial.println("START - Begining of Each Minute");
    DSTconvert ();
    HourMinute ();
    if (now.minute()!=0) {
      chirp=false;
    }
  }
  
  if (now.minute()==0) {
    Serial.println("Top of the hour.");
    if (chirp==false) {
      Serial.println("Making the clock dance.");
      CooCooClock ();
    }
  }
   
  keepSeconds ();

} // END of LOOP




void checkDemo () {
  if (digitalRead(switch_demo)==HIGH) {
    while (digitalRead(switch_demo)==HIGH) {
      Serial.println("Demo Switch IS HIGH");
      sweepClock (clock_max);
      CooCooClock ();
    }
    DSTconvert ();
    HourMinute ();
  }
}




// START TIME DISPLATY FUNCTIONS //

int HourMinute () {
  DateTime now = RTC.now();
  if (currentHour <= 12) {
    hourMin=(currentHour*60)+now.minute();
    //Serial.print("hourMin is ");
    //Serial.print(hourMin, DEC);
    //Serial.println(" with case 1.");
  }
  else {
     hourMin=((currentHour-12)*60)+now.minute();
    //Serial.print("hourMin is ");
    //Serial.print(hourMin, DEC);
    //Serial.println(" with case 2.");
  }
  clockPosition = map(hourMin, 0, 720, clock_min, clock_max);
  //Serial.print("Clock Position is MAPPED TO ");
  //Serial.print(clockPosition, DEC);
  //Serial.println(".");
  
  if (clockPosition != clockPositionPrev) {
    sweepClock (clockPosition);
  }
  else {
    //Serial.println("no need to move the clock.");
  }
  hourMinPrev=now.minute();
  clockPositionPrev=clockPosition;
  
  printTime();
  
}


void keepSeconds () {
 DateTime now = RTC.now();
 if ( now.second() % 2 == 0) {
   digitalWrite(secondsLED, HIGH);
 }
 else {
   digitalWrite(secondsLED, LOW);
 }
}

void sweepClock (int ending){
  
  //Serial.print("Clock is in moton - starting at ");
  //Serial.print(clockPositionPrev, DEC);
  //Serial.print(" and going to ");
  //Serial.print(ending, DEC);
  //Serial.println(".");
  
  for ((clockPositionPrev-ending)>0; clockPositionPrev > ending; clockPositionPrev--) {   //move backwards
    clock.write(clockPositionPrev);   
    delay(clock_speed); 
  } 
  for ((clockPositionPrev-ending)<0; clockPositionPrev < ending; clockPositionPrev++) {   //move forwards
    clock.write(clockPositionPrev);    
    delay(clock_speed); 
  }

  //Serial.print("Clock is stopped - clockPositionPrev is ");
  //Serial.print(clockPositionPrev, DEC);
  //Serial.print(" and ending is ");
  //Serial.print(ending, DEC);
  //Serial.println(".");
  
}


// END TIME DISPLATY FUNCTIONS //

// START DST FUNCTION //

void DSTconvert () {
  DateTime now = RTC.now();
  if (digitalRead(switch_dst)==HIGH) {
    Serial.println("DST is HIGH");
    currentHour=now.hour()+DSTfactor;
    if (currentHour > 23) {
      currentHour =0;
    }
    if (currentHour < 0) {
      currentHour=23;
    }
  }
  else {
    currentHour=now.hour();
  }
    //Serial.print("currentHour is ");
    //Serial.print(currentHour, DEC);
    //Serial.println(" with the DST convert funct.");
}

// END DST FUNCTION //

// START Coo Coo Clock Functions //
void CooCooClock () {
  if (currentHour<= 12 && currentHour > 0) {
    //Serial.print("Bird will move "); 
    //Serial.print( currentHour, DEC);
    //Serial.println( " times.");
    CooCoo(currentHour,250);
  }
  else if (currentHour>12 && currentHour<24) {
    reps = currentHour-12;
    //Serial.print("Bird will move "); 
    //Serial.print( reps, DEC);
    //Serial.println( " times.");
    CooCoo(reps,250);
  }
  else if (currentHour==0){
    //Serial.print("Bird will move "); 
    //Serial.print( "12");
    //Serial.println( " times - special math.");
    CooCoo(12,250);
  }
  else {
    Serial.println( "my time math is fucked");
  }
  chirp=true;
}

void CooCoo (int reps, int timer) {
 sweepDoor(door_open);
 delay (timer);
 for (reps > 0; reps >0; reps--) {
   sweepBird(bird_in);
   delay (bird_hold);
   sweepBird(bird_out);
   delay (timer);
   Serial.println("CHIRP!");
 }
 sweepDoor(door_closed); 
 chirp=true;
}

void sweepDoor(int ending){
  
  //Serial.print("Door is in motion - starting at ");
  //Serial.print(doorPosition, DEC);
  //Serial.print(" and going to ");
  //Serial.print(ending, DEC);
  //Serial.println(".");
  
  for ((doorPosition-ending)>0; doorPosition > ending; doorPosition--) {   //move backwards
    door.write(doorPosition);    
    delay(door_speed); 
  } 
  for ((doorPosition-ending)<0; doorPosition < ending; doorPosition++) {   //move forwards
    door.write(doorPosition);    
    delay(door_speed); 
    
  }
  
  //Serial.print("Door is stopped - doorPosition is ");
  //Serial.print(doorPosition, DEC);
  //Serial.print(" and ending is ");
  //Serial.print(ending, DEC);
  //Serial.println(".");
  
}

void sweepBird(int ending){
  
  //Serial.print("Bird is in motion - starting at ");
  //Serial.print(birdPosition, DEC);
  //Serial.print(" and going to ");
  //Serial.print(ending, DEC);
  //Serial.println(".");
  
  for ((birdPosition-ending)>0; birdPosition > ending; birdPosition--) {   //move backwards
    bird.write(birdPosition);        
    delay(bird_speed); 
  } 
  for ((birdPosition-ending)<0; birdPosition < ending; birdPosition++) {   //move forwards
    bird.write(birdPosition);    
    delay(bird_speed); 
  }
  
  //Serial.print("Bird is stopped - birdPosition is ");
  //Serial.print(birdPosition, DEC);
  //Serial.print(" and ending is ");
  //Serial.print(ending, DEC);
  //Serial.println(".");
}


// END Coo Coo Clock Functions //


void printTime () {
  DateTime now = RTC.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}
