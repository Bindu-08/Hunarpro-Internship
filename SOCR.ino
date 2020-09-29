#include <LiquidCrystal.h> // includes the LiquidCrystal Library 
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Servo.h>
#include <NewPing.h>
#include<FirebaseArduino.h>
#define FIREBASE_HOST "cleaningrobot-59c21.firebaseio.com"                     //(to be changed) Your Firebase Project URL goes here without "http:" , "\" and "/"
#define FIREBASE_AUTH "Qi33lGVG97ucl8GajPre5TxTrFarzhkVPKEnsxAF"       //(to be changed) Your Firebase Database Secret goes here
LiquidCrystal lcd(1, 2, 4, 5, 6, 7); // Creates an LC object. Parameters: (rs, enable, d4, d5, d6, d7)

// wifi object
WiFiClient client;

//servo motor pins

Servo myservo1;  // create servo object to control a servo
Servo myservo2;  // create servo object to control a servo

int pos = 0;

//ultrasonic setup
int trig_pin = 39;
int echo_pin = 40;

#define maximum_distance 300
boolean goesForward = false;
int distance = 50;

NewPing sonar(trig_pin, echo_pin, maximum_distance); //sensor function

int distanceRight = 0;
int distanceLeft = 0;

//motor driver pins

int LW1 = 24;   //left wheel pins
int LW2 = 26;
int RW1 = 28;   //right wheel pins
int RW2 = 30;

int leftPWM = 25;
int rightPWM = 31;

//relay pins

int vacuum = 32;    //vacuum pump  pin
int spray = 34;     //sprayer pin
int uv = 36;        //UV light pin

int brush = 38;     //dust brush pin (two motors shorted in opp direction)

//sensor pins

int pir = 27;       //obstacle detector pin
int pirVal = 0;
int pirState = LOW;


//for firebase response.......S tags
int MNAT;    //to get command for manual control
int DWVal;  //to get command for dry cleaning
int CoV;    //to get command for combo cleaning
int ScV;    //to get command to stop cleaning
int FdV;    //to get command for forward movement
int BkV;    //to get command for backward movement
int RtV;    //to get command for right movement
int LtV;    //to get command for left movement

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2); // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display }
  pinMode(LW1, OUTPUT);
  pinMode(LW2, OUTPUT);
  pinMode(RW1, OUTPUT);
  pinMode(RW2, OUTPUT);
  pinMode(vacuum, OUTPUT);
  pinMode(spray, OUTPUT);
  pinMode(uv, OUTPUT);
  pinMode(brush, OUTPUT);
  pinMode(rightPWM, OUTPUT);
  pinMode(leftPWM, OUTPUT);
  pinMode(pir, INPUT);

  digitalWrite(RW2, LOW);    //RIGHT NO ROTN
  digitalWrite(RW1, LOW);
  digitalWrite(LW1, LOW);    //LEFT NO ROTN
  digitalWrite(LW2, LOW);

  digitalWrite(vacuum, LOW);
  digitalWrite(spray, LOW);
  digitalWrite(uv, LOW);
  digitalWrite(brush, LOW);

  myservo1.attach(27);  // attaches the servo on pin 9 to the servo object
  myservo2.attach(29);  // attaches the servo on pin 9 to the servo object

  myservo1.write(45);   //brush remains lifted up

  myservo2.write(115);  //Ultrasonic servo
  delay(2000);
  distance = readPing();   //for checking distance 
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);
  distance = readPing();
  delay(100);

  //CONNECTION WITH AVAILABLE WIFI

  //WiFiManager

  WiFiManager wifiManager;

  //reset saved settings
  //wifiManager.resetSettings();

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");

  Serial.println("START");
  while ((!(WiFi.status() == WL_CONNECTED)))
  {
    delay(300);
    Serial.print("..");

  }
  Serial.println("Connected");
  Serial.println("Your IP is");
  Serial.println((WiFi.localIP().toString()));
  Serial.println("Your SSID is:");
  Serial.println(WiFi.SSID());
  Serial.println("Your password is:");
  Serial.println(WiFi.psk());




  //SETTING UP FIREBASE
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.setInt("S1", 0);                    //Here the variables which needs to be the one which is used in our Firebase and MIT App Inventor
  Firebase.setInt("S2", 0);
  Firebase.setInt("S3", 0);
  Firebase.setInt("S4", 0);
  Firebase.setInt("S5", 0);
  Firebase.setInt("S6", 0);
  Firebase.setInt("S7", 0);
  Firebase.setInt("S8", 0);

}

void firebasereconnect()
{
  Serial.println("Trying to reconnect");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}


void brushUp()    //lift the dust brush while wet cleaning
{
  myservo1.write(45);
}

void brushDown()  //put down the dust brush while dry cleaning
{
  myservo1.write(135);
}


void forward()
{
  digitalWrite(RW1, HIGH);    //RIGHT CW ROTN
  digitalWrite(RW2, LOW);
  digitalWrite(LW1, HIGH);    //LEFT CCW ROTN
  digitalWrite(LW2, LOW);
  analogWrite(leftPWM, 255);    //setting speed of left motor
  analogWrite(rightPWM, 255);   //setting speed of right motor


  lcd.setCursor(1, 2); // Sets the location at which subsequent text written to the LCD will be displayed
  lcd.print("GOING STRAIGHT");
}

void backward()
{
  digitalWrite(RW2, HIGH);    //RIGHT CCW ROTN
  digitalWrite(RW1, LOW);
  digitalWrite(LW2, HIGH);    //LEFT CW ROTN
  digitalWrite(LW1, LOW);

  analogWrite(leftPWM, 255);    //setting speed of left motor
  analogWrite(rightPWM, 255);   //setting speed of right motor


  lcd.setCursor(1, 2); // Sets the location at which subsequent text written to the LCD will be displayed
  lcd.print("GOING BACK");
}


void left()
{
  digitalWrite(RW1, HIGH);    //RIGHT CW ROTN
  digitalWrite(RW2, LOW);
  digitalWrite(LW2, HIGH);    //LEFT CW ROTN
  digitalWrite(LW1, LOW);

  analogWrite(leftPWM, 180);    //setting speed of left motor
  analogWrite(rightPWM, 255);   //setting speed of right motor


  lcd.setCursor(1, 2); // Sets the location at which subsequent text written to the LCD will be displayed
  lcd.print("TURNING LEFT");
}


void right()
{
  digitalWrite(RW2, HIGH);    //RIGHT CCW ROTN
  digitalWrite(RW1, LOW);
  digitalWrite(LW1, HIGH);    //LEFT CCW ROTN
  digitalWrite(LW2, LOW);

  analogWrite(leftPWM, 255);    //setting speed of left motor
  analogWrite(rightPWM, 180);   //setting speed of right motor

  lcd.setCursor(1, 2); // Sets the location at which subsequent text written to the LCD will be displayed
  lcd.print("TURNING RIGHT");
}

void stopp()
{

  // NO MOVEMENT
  
  digitalWrite(RW2, LOW);    //RIGHT NO ROTN
  digitalWrite(RW1, LOW);
  digitalWrite(LW1, LOW;    //LEFT NO ROTN
  digitalWrite(LW2, LOW);
  
  lcd.setCursor(1, 2); // Sets the location at which subsequent text written to the LCD will be displayed
  lcd.print("STOPPED");

}

void dryClean()   // brush, vacuum cleaner, UV ON, water sprayer OFF
{
  digitalWrite(brush, HIGH);
  brushDown();
  digitalWrite(vacuum, HIGH);
  digitalWrite(uv, HIGH);
  digitalWrite(spray, LOW);

  lcd.setCursor(1, 1); // Sets the location at which subsequent text written to the LCD will be displayed
  lcd.print("DRY CLEANING");

}

void wetClean()   // brush, vacuum cleaner OFF, water sprayer, UV ON
{
  digitalWrite(brush, LOW);
  brushUp();
  digitalWrite(vacuum, LOW);
  digitalWrite(uv, HIGH);
  digitalWrite(spray, HIGH);

  lcd.setCursor(1, 1); // Sets the location at which subsequent text written to the LCD will be displayed
  lcd.print("WET CLEANING");

}

void comboClean()   // brush, vacuum cleaner, UV ON, water sprayer OFF
{
  digitalWrite(brush, HIGH);
  brushDown();
  digitalWrite(vacuum, HIGH);
  digitalWrite(uv, HIGH);
  digitalWrite(spray, HIGH);

  lcd.setCursor(1, 1); // Sets the location at which subsequent text written to the LCD will be displayed
  lcd.print("COMBO CLEANING");

}

void noClean()    // all operations off
{
  //NO CLEANING
  digitalWrite(brush, LOW);
  brushUp();
  digitalWrite(vacuum, LOW);
  digitalWrite(uv, LOW);
  digitalWrite(spray, LOW);

  lcd.setCursor(1, 1); // Sets the location at which subsequent text written to the LCD will be displayed
  lcd.print("NOT CLEANING");

}


void fireValues()     //Reading the value of the respective variable Status from the firebase
{
  MNAT = Firebase.getString("S1").toInt();
  DWVal = Firebase.getString("S2").toInt();
  CoV = Firebase.getString("S3").toInt();
  ScV = Firebase.getString("S4").toInt();
  FdV = Firebase.getString("S5").toInt();
  BkV = Firebase.getString("S6").toInt();
  RtV = Firebase.getString("S7").toInt();
  LtV = Firebase.getString("S8").toInt();
}

void moveforward()   //to control forward movement
{
  if (FdV == 1)
  {
    forward();
  }
  else if (FdV == 0)
  {
    stopp();
  }
}


void movebackward()   //to control backward movement
{
  if (BkV == 1)
  {
    backward();
  }
  else if (BkV == 0)
  {
    stopp();
  }
}


void moveRight()   //to control right movement
{
  if (RtV == 1)
  {
    right();
  }
  else if (RtV == 0)
  {
    stopp();
  }
}

void moveLeft()   //to control left movement
{
  if (LtV == 1)
  {
    Left();
  }
  else if (LtV == 0)
  {
    stopp();
  }
}

void doClean()    //to set the cleaning operation mode
{
  if (DWVal == 1)   //for dry cleaning
  {
    dryClean();
  }
  esle if (DWVal == 0)   //for wet cleaning
  {
    wetClean();
  }
  if (CoV == 1)   //for combo cleaning
  {
    comboClean();
  }
  else if (CoV == 0)   //to stop combo cleaning
  {
    noClean();
  }
  if (ScV == 1)   //to stop cleaning
  {
    noClean();
  }
  if (ScV == 0)   //to start cleaning
  {
    doClean();
  }
}


void manual()   //for manual control
{
  stopp();
  moveforward();
  movebackward();
  moveRight();
  moveLeft();
  doClean();
}



void atmov()    //for auto movement
{


  delay(50);

  if (distance <= 10)
  {
    stopp();
    delay(300);
    backward();
    delay(500);
    stopp();
    delay(300);
    distanceRight = lookRight();
    delay(300);
    distanceLeft = lookLeft();
    delay(300);

    if (distance >= distanceLeft)
    {
      right();
      stopp();
    }
    else
    {
      left();
      stopp();
    }
  }
  else {
    forward();
  }
  distance = readPing();
}

int lookRight() {     //turning USS servo to check left environment
  myservo2.write(50);
  delay(500);
  int distance = readPing();
  delay(100);
  myservo2.write(115);
  return distance;
}

int lookLeft() {    //turning USS servo to check left environment
  myservo2.write(170);
  delay(500);
  int distance = readPing();
  delay(100);
  myservo2.write(115);
  return distance;
  delay(100);
}

int readPing() {    //unit conversion of signals
  delay(70);
  int cm = sonar.ping_cm();
  if (cm == 0) {
    cm = 250;
  }
  return cm;
}


void loop()
{


  if (Firebase.failed())
  {
    Serial.print("setting number failed:");
    Serial.println(Firebase.error());
    firebasereconnect();
    return;
  }

  if (MNAT == 1)    //if manual mode is activated
  {
    stopp();
    noClean();
    manual();
  }
  else if (MNAT == 0)   //if auto mode is activated
  {
    stopp();
    noClean();
    delay(2000);
    atmov();
    doClean();
  }

}
