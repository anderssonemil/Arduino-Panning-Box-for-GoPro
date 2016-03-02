
/*
  Arduino Pan cube by Emil Andersson, 02-29-16

  Day mode (Constant speed)
   - Start/Endpoint set Manually or by degrees
   - Run time
   - Direction
  
  Nigh mode (Only run when picture is saved)
   - Only 360 degrees (should get manual or by degrees later)
   - Run Time
   - Direction
  
  Panorama mode
   - Run a full 360 degrees in 30 seconds for the gopro to snap some photos that can be stitched

  Lever
   - 2 axis lever
*/

// Including library for Nokia LCD display
#include <LCD5110_Graph.h>
LCD5110 LCD(8,9,10,12,11); // sck, mosi, dc, rst, cs

// Including library for controlling Stepper motor
#include <AccelStepper.h>

// Setting up the Stepper motor
AccelStepper stepper(AccelStepper::HALF4WIRE, 5 ,3, 4, 2); // Set to half steps on pin 5, 3, 4, 2

// Including library for SPI and I2C
#include <Wire.h>
#include <SPI.h>

// Including library for Acceleromter
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

#define LIS3DH_CLK 8
#define LIS3DH_MISO 7
#define LIS3DH_MOSI 9
#define LIS3DH_CS 13
Adafruit_LIS3DH lis = Adafruit_LIS3DH(LIS3DH_CS, LIS3DH_MOSI, LIS3DH_MISO, LIS3DH_CLK);

// Nokia LCD fonts
extern unsigned char SmallFont[];
extern unsigned char TinyFont[];

// LCD Variables
boolean lcdBackLightStatus = 0; // 0 = LCD backlight ON, 1= LCD backlight OFF
const char lcdBackLightPin = 6; // Pin controlling backlight

// Keyboard Variables
const char buttonPin = A0; // Analog pin for keyboard
const int shortDelay = 500; // Mostly used when entering a menu to allow user to release button

// Some panning variables
float panSteps;
int panSpeed; // The speed for the stepper motor
int panDirection; // Direction, 1=CW, -1=CCW
float panEndPosition; // End position of the pannning
int panRunTime;  // Time to panning
const char goProPin = A1;  // analog pin for reading GoPro LED blink
const int panRevolutionSteps = 19000;  // Steps for a full 360

// Menu parameters
const char row[5] = {0, 12, 21, 30, 39}; // Row positions for menus
const char column[3] = {0, 11, 20}; // Columns for menus

void setup()
{
  //Serial.begin(9600);

  // Starting up the accelerometer
  lis.begin(0x18);
  lis.setRange(LIS3DH_RANGE_8_G);   // 2, 4, 8 or 16 G!
 
  // Splash Screen
  pinMode(lcdBackLightPin, OUTPUT);
  digitalWrite(lcdBackLightPin, lcdBackLightStatus);
  LCD.InitLCD(68);
  LCD.drawRoundRect(0,0,83,47);
  LCD.drawRoundRect(3,3,80,44);
  LCD.setFont(SmallFont);
  LCD.print("Arduino", CENTER, row[1]);
  LCD.print("Pan Cube", CENTER, row[3]);
  LCD.update();
  waitForKey();
}

void loop()
{
while(1)
  switch(menuMain())
  {
    case 1: // Day mode
      switch(endPointMethod())
      {
        case 1: // // Manual  ------------------------- Run Day -> Manual ------------------------------------------
        panSteps = setEndPointManually();
        panRunTime = pickRunTime()*60;
        panSpeed = panSteps/panRunTime; // Calculate runtime (steps/seconds)
        panEndPosition = stepper.currentPosition()+(panSteps) ;   //Add this calcualtion
        runPan();   
        break; // ------------------------------------ End ---------------------------------------------------------

        case 2: // Degrees  ------------------------- Run Day -> Degrees -------------------------------------------
        panSteps = pickStepsByDegrees(); // Get steps to run
        panRunTime = pickRunTime()*60;
        panDirection = pickDirection();// Get run direction
        panSteps = panSteps*panDirection;
        panSpeed = panSteps/panRunTime; // Calculate runtime (steps/seconds)
        panEndPosition = stepper.currentPosition()+(panSteps) ;   //Add this calcualtion
        runPan();
        break;  // ------------------------------------ End --------------------------------------------------------
      }
     break;
    
    case 2: // Night mode
      nightPanning();
    break;
    
    case 3: // Panorama  --------------------------- Panorama mode  ------------------------------------------------
     stepper.setCurrentPosition(0);
     panDirection = pickDirection();// Get run direction
     panSpeed = 633*panDirection;
     panEndPosition = panRevolutionSteps*panDirection;
     panSteps = panRevolutionSteps*panDirection;
     panRunTime = 30;
     LCD.clrScr(); 
     LCD.print("PANORAMA", CENTER,row[0]);
     LCD.drawLine(0,9,83,9);
     LCD.print("2 or 5 sec", CENTER, row[1]);
     LCD.print("goPro",CENTER, row[2]);
     LCD.print("time lapse",CENTER, row[3]);
     LCD.update();
     waitForKey();
     runPan();
    break;  // ------------------------------------ End --------------------------------------------------------
    
    case 4: // Lever checking
     level();
    break;
  }
  delay(shortDelay); 
}

/*   various functions
menuMain
nightPanning
setEndPointManually
waitForKey
waitForKey
pickNightRunTime
pickRunTime
pickDirection
pickStepsByDegrees
endPointMethod
buttonCheck
runPan
*/

int menuMain() // Return 1=Day, 2=Night, 3=Panorama, 4=Lever
 {
   // Variables
   char mainScenario = 1; // 1=Day, 2=Night, 3=Panorama, 4=Lever
   int button;
  // Print LCD
  LCD.clrScr(); 
  LCD.print("MODE", CENTER,row[0]);
  LCD.drawLine(0,9,83,9);
  LCD.print("DAY", column[1], row[1]);
  LCD.print("NIGHT",column[1], row[2]);
  LCD.print("PANORAMA",column[1], row[3]);
  LCD.print("LEVEL",column[1], row[4]);
  delay(shortDelay);
  while(1) // Run until SET is pressed
  {
     for (char i = 1; i<=4; i++) // Populate the check boxes and update LCD
     {
      if (i== mainScenario)
       LCD.print("*", column[0], row[i]);
      else
       LCD.print("#", column[0], row[i]);
     }
     LCD.update();

     button = buttonCheck(); // Check for key press
      if (button != 0)  // If any key is pressed
       {
        if (button == 1)  // Set pressed, leave menu
        {
        return(mainScenario);
        }
      if (button  == 3 && mainScenario < 4) //Move checked box UP
        mainScenario += 1;
      if (button == 4 && mainScenario > 1) // Move checked box DOWN
        mainScenario -= 1;
     delay(300);
    }
  }
}

void nightPanning()
// Run the Stepper 360 degrees over set time.
//  Only step forward while GoPro saves picture(sensing GoPro LED blink).
  {
     // Variables for the function
     int goProLedRead = 0; // GoPro LED status
     float lapseTime = 0;  // Timer variable
     int nightLapseSteps = 0;  // Steps to go between each GoPro exposure
     int panNextStop = 0;  // Counter for next stop between blinks
     int button;
     long exitTimer;
     int longPressTime = 2000;
     // Set steps per second needed for a 360 degrees rotation for set time
     nightLapseSteps = panRevolutionSteps/(pickNightRunTime()*60); 
     // Get direction 1 for CW, -1 for CCW
     panDirection = pickDirection();  
     LCD.clrScr(); 
     LCD.print("NIGHT PANNING", CENTER,row[0]);
     LCD.drawLine(0,9,83,9);
     LCD.print("Start", CENTER, row[1]);
     LCD.print("Night Lapse",CENTER, row[2]);
     LCD.print("and press SET",CENTER, row[3]);
     LCD.update();
     waitForKey(); // Pause until button pressed
     LCD.clrScr(); 
     LCD.print("NIGHT PANNING", CENTER,row[0]);
     LCD.drawLine(0,9,83,9);
     LCD.print("Checking", CENTER, row[1]);
     LCD.print("Exposure",CENTER, row[2]);
     LCD.print("Time",CENTER, row[3]);
     LCD.update();
     // Checking the shutter speed by messure time between the first two LED blinks
     while(analogRead(goProPin)>700); // Trigger on first blink
     lapseTime = millis(); // Set first LED blink time
     delay(shortDelay); // Wait shortly for the LED to go dark
     while(analogRead(goProPin)>700); // Trigger on second blink 
     lapseTime = (millis()-lapseTime)/1000; // Calculate the GoPro Exposure time
     // Calculate steps with direction needed per exposure to get full 360 in set time
     nightLapseSteps = nightLapseSteps * lapseTime * panDirection; 
     LCD.clrScr(); 
     LCD.print("NIGHT PANNING", CENTER,row[0]);
     LCD.drawLine(0,9,83,9);
     LCD.print("Exposure", CENTER, row[1]);
     LCD.print("time is",CENTER, row[2]);
     LCD.print(String (lapseTime),CENTER, row[3]);
     LCD.update();
     // Setting up the Stepper motor
     stepper.setCurrentPosition(0);  // Start point to 0
     stepper.setMaxSpeed(900);  // Set max speed
     stepper.setSpeed(1);
     stepper.setSpeed(600*panDirection);  // Set speed to run and it's direction
     lcdBackLightStatus = 1; // Turns off the LED to save power
     digitalWrite(lcdBackLightPin, lcdBackLightStatus);
    while(1)  // Burst of steps after every exposure until 360 degrees is reached
    {
      button = analogRead(buttonPin);
        if (button > 1000)
         exitTimer = millis();
        if (button <1000 && ((millis()-exitTimer) > longPressTime))
         {
          lcdBackLightStatus = 0; // Turns on the LED when done
          digitalWrite(lcdBackLightPin, lcdBackLightStatus);
          delay(shortDelay*2);
          break;
         }
      if (stepper.currentPosition()*panDirection > panRevolutionSteps) 
       {
       lcdBackLightStatus = 0; // Turns on the LED when done
        digitalWrite(lcdBackLightPin, lcdBackLightStatus);
       break;
       }
      goProLedRead = analogRead(goProPin);    
       if (goProLedRead < 700) // If GoPro LED blinks
       {
        panNextStop += nightLapseSteps;  // Steps to go at LED blink
          while (stepper.currentPosition() != panNextStop)
          stepper.runSpeed();
         stepper.disableOutputs(); // Turn off Stepper driver to save power until next move
         delay(500); // A short delay for LED to go dark, then wait for next GoPro save blink
       }
    }
}

int setEndPointManually()
 {
  boolean panStartPointsReady = 0;
  boolean panEndPointRead = 0;
  int button;
  long startTime;
  int endPosition;
  stepper.setCurrentPosition(0);
  stepper.setMaxSpeed(900);
  stepper.setAcceleration(1000);
  LCD.clrScr();
  LCD.print("SET END POINTS", CENTER, row[0]);
  LCD.drawLine(0,9,84,9);
  LCD.print("Adjust start", CENTER, row[1]);
  LCD.print("position", CENTER, row[2]);
  LCD.update();
  delay(shortDelay);
  while (panStartPointsReady == LOW) //Run until both endpoints are set
  { 
    button = buttonCheck(); // Chek if and what key is pressed
    switch(button)
     {
     case 2: // Right Key
       stepper.setSpeed(1);
       stepper.setSpeed(900); // Set speed and direction
       startTime = millis()+100;
        while (millis() < startTime) // Move Right for a while
          stepper.runSpeed();
       break; 
     case 5: // Left Key
       stepper.setSpeed(1);
       stepper.setSpeed(-900);
       startTime = millis()+100;
         while (millis() < startTime) // Move Left for a while
           {
             stepper.runSpeed();
           }
       break;
     case 1: // Set Key
       if (panEndPointRead == LOW) //Set start endpoint to HIGH and move on to set Right endpoint
          {
           panEndPointRead = HIGH;
           stepper.setCurrentPosition(0);
           LCD.print(" Adjust end ", CENTER, row[1]); //Change LCD to ask for Right endpoint
           LCD.update();
           delay(shortDelay);
          }
       else 
          {
          if (stepper.currentPosition() == 0) // If Start position and End position is the same
           {
            LCD.print("End needed!", CENTER, row[4]); //Ask for Right endpoint
            LCD.update();
            delay(shortDelay);
            LCD.print("           ", CENTER, row[4]); //Change LCD to ask for Right endpoint
            LCD.update();
            break;
           }
           endPosition = stepper.currentPosition();
            if (stepper.currentPosition() > 0)
                panDirection = 1;
            else
                panDirection = -1;
           stepper.moveTo(0);
           stepper.setSpeed(1);
           stepper.setSpeed(900*panDirection);
           stepper.setAcceleration(10000);
           LCD.print("  Moving  ", CENTER, row[1]); //Change LCD to ask for Right endpoint
           LCD.print("to start", CENTER, row[2]);
           LCD.update();
           while(stepper.currentPosition() !=0)
           stepper.run();
           return(endPosition);
          }
      }
   }
 }

void waitForKey()
// Wait for a short moment and then returns whenever a button on the analog keyboard is pressed
  {
    delay(shortDelay);  // Short delay
    while (analogRead(buttonPin) > 1000); // Wait for any button to be pressed before run starts
  }

void level()
// Shows a simple level on the LCD so that a tripod can be adjusted. Pressing setup button returns.
{
// Accelerometer Smooting Variables
const char numReadings = 30;  // Numbers of readings used for an average array
const char updateSpeed = 10; // Time between each update
// Variables
int readingsX[numReadings];      // the readings from the analog input
int readingsY[numReadings];      // the readings from the analog input
int readIndex = 0;               // the index of the current reading
int totalX = 0;                  // the running total
int averageX = 0;                // the average
int totalY = 0;                  // the running total
int averageY = 0;                // the average
int xx;  // X Coordinate on the the LCD
int yy;  // Y Coordinate on the the LCD
const int xCorrection = -124;  // Correction value for X zero
const int yCorrection = + 43;  // Correction value for y zero
const int xLimit = 300; // Max value used for X
const int yLimit = 500; // Max value used for y
 delay(shortDelay);  //  Short delay
 LCD.clrScr();
 // Setting up X and Y arrays with zeros
 for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readingsX[thisReading] = 0;
 for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readingsY[thisReading] = 0; 
  while(buttonCheck()!= 1) // run until setup button is pressed
  {
  lis.read();      // get X Y and Z data at once
   totalX = totalX - readingsX[readIndex];
   totalY = totalY - readingsY[readIndex];
   readingsX[readIndex] = lis.x - xCorrection;
   readingsY[readIndex] = lis.y + yCorrection;
   totalX = totalX + readingsX[readIndex];
   totalY = totalY + readingsY[readIndex];
   readIndex = readIndex + 1;
   // When array is filled to last array position, update LCD and start filling the array att the beginning
   if (readIndex >= numReadings) 
    {
     readIndex = 0;
     LCD.clrScr();
     LCD.drawCircle(42, 24, 8);  // Draw Circle in center
     xx = constrain(map((averageX),xLimit, -xLimit,84,0),5,78);  // Convert average accelerometer X value to LCD coordiinates
     yy = constrain(map((averageY),yLimit, -yLimit,0,48),5,42);  // Convert average accelerometer y value to LCD coordiinates
     for (int i = 2; i<7; i++)
     LCD.drawCircle(xx, yy, i);  // Draw donaut for lever
     LCD.update();
    }
   averageX = totalX / numReadings;  // Calculate X average
   averageY = totalY / numReadings;  // Calculate Y average
   delay(updateSpeed); // delay in between reads for stability
  }
}

int pickNightRunTime()  // Get desired run time
{
  // Variables
  const int menuMax = 1000;  // Max Time
  const boolean menuMin = 0; // Min Time
  int countFactor = 1;
  char button;  // For analog Button value
  int runNightTime = 1;
  LCD.clrScr();
  LCD.print("RUN TIME", CENTER,row[0]);
  LCD.drawLine(0,9,83,9);
  LCD.print("minutes", column[2],row[2]);
  LCD.print(String(runNightTime), column[0], row[2]);
  LCD.update();
  delay(shortDelay);
  while(1)
    {
     button = buttonCheck(); //Check if button is pressed
      if (button != 0)
       {
        if (button == 1) // SET Button
         {
          LCD.clrScr();
          LCD.update();
          return(runNightTime);
         }
        if (button == 4) // UP Button
         {
          if ((runNightTime + countFactor) < menuMax)
           runNightTime += countFactor;
         }
        if (button == 3) // DOWN Button
         {
          if ((runNightTime - countFactor) > menuMin) 
            {
              runNightTime -= countFactor;
            }
          else
           {
           countFactor = runNightTime / 2;
           runNightTime -= countFactor;
           }
          }
       LCD.print("   ", column[0], row[2]);
       LCD.print(String(runNightTime), column[0], row[2]);
       LCD.update();
       delay(300);
       countFactor += 5; // If button still pressed, Increase increments
      }
     else 
       countFactor = 1; // Decrease speed if button is let go
    } 
 }

 int pickRunTime()  // Get desired run time
{
  // Variables
  const int menuMax = 1000;  // Longest time allowed in minutes
  const boolean menuMin = 0; // Shortest time allowed
  int countFactor = 1;
  int button;
  int runTime = 1;
  LCD.clrScr();
  LCD.print("RUN TIME", CENTER,row[0]);
  LCD.drawLine(0,9,83,9);
  LCD.print("minutes", column[2],row[2]);
  LCD.print(String(runTime), column[0], row[2]);
  LCD.update();
  delay(shortDelay);
  while(1)
    {
     button = buttonCheck(); //Check if Button is pressed
      if (button != 0)
       {
        if (button == 1) // SET Button
         {
          LCD.clrScr();
          LCD.update();
          return(runTime);
         }
        if (button == 4) // UP Button
         {
          if ((runTime + countFactor) < menuMax)
           runTime += countFactor;
         }
        if (button == 3) // DOWN Button
         {
          if ((runTime - countFactor) > menuMin) 
            {
              runTime -= countFactor;
            }
          else
           {
           countFactor = runTime / 2;
           runTime -= countFactor;
           }
          }
       LCD.print("   ", column[0], row[2]);
       LCD.print(String(runTime), column[0], row[2]);
       LCD.update();
       delay(300);
       countFactor += 10; // Increase speed if still holdeing button
      }
     else 
       countFactor = 1; // Decrease speed if button is let go
    } 
 }
 int pickDirection() // Return -1=CCW and 1=CW 
{
  // Variables
  int button;
  const int animationSpeed = 500; // Milliseconds between each update
  int animationDirection = 1;  // 1 CW, -1 CCW
  int animationStep = 0;  // Store current animation step
  long animationTimer = (millis()+animationSpeed);
  const char rotationCenterX = 42;
  const char rotationCenterY = 34;
  int panDirection = 1;
  //  Array of each circle animation step
  const char rotationCircle[8][2] = {
  {0,-7}, //0
  {5,-5}, //45
  {7,0}, //90
  {5,5}, //135
  {0,7}, //180
  {-5,5}, //225
  {-7,0}, //270
  {-5,-5}, //315
        };
  // Print LCD
  LCD.clrScr();
  LCD.print("DIRECTION", CENTER,row[0]);
  LCD.drawLine(0,9,83,9);   
  LCD.drawCircle(rotationCenterX,rotationCenterY,4);
  delay(shortDelay);
  while(1)
  {
    if (animationTimer < millis())
    {
         animationTimer = millis()+animationSpeed;
         LCD.clrCircle(rotationCircle[animationStep][0]+rotationCenterX, rotationCircle[animationStep][1]+rotationCenterY, 2);
         LCD.clrCircle(rotationCircle[animationStep][0]+rotationCenterX, rotationCircle[animationStep][1]+rotationCenterY, 1);
         animationStep += animationDirection;
      if (animationStep < 0)
       animationStep = 7;
      if (animationStep > 7)
       animationStep = 0;
       LCD.drawCircle(rotationCircle[animationStep][0]+rotationCenterX, rotationCircle[animationStep][1]+rotationCenterY, 2);
       LCD.drawCircle(rotationCircle[animationStep][0]+rotationCenterX, rotationCircle[animationStep][1]+rotationCenterY, 1);
       LCD.update();
    }
    for (char i = 1; i<=2; i++)
     {
      if (i== panDirection)
       {
        LCD.print("LEFT -> RIGHT", CENTER, row[1]);
        animationDirection = -1;
       }
      else
       {
        LCD.print("RIGHT -> LEFT", CENTER, row[1]);
        animationDirection = 1;
       }
     }
     LCD.update();
   button = buttonCheck();
    if (button != 0)
     {
      if (button == 1)  // Set Button
        {
          LCD.clrScr();
          LCD.update();
          return(animationDirection);
        }
      if (button  == 5 && panDirection == 1) //Left Button for CCW
        panDirection = 2;
      if (button == 2 && panDirection == 2) // Right Button for CW
        panDirection = 1;
    }
  }
}

int pickStepsByDegrees() // Returns degrees to run
{
  // Variables
  int maxDegrees = 355;
  int countFactor = 5;
  int endDegrees = 180;
  int button;
  //Print LCD
  LCD.clrScr();
  LCD.print("ENDPOINT", CENTER,row[0]);
  LCD.drawLine(0,9,83,9);
  LCD.print("DEGREES", column[2],row[2]);
  LCD.print(String(endDegrees), column[0], row[2]);
  LCD.update();
  delay(shortDelay);
   while(1) // Run until SET is pressed
    {
     button = buttonCheck();
      if (button != 0)
       {
       if (button == 1)
         {
          LCD.clrScr();
          LCD.update();
          return(endDegrees*53);
        }
       if (button  == 4 && endDegrees < maxDegrees)
        endDegrees += countFactor;
       if (button == 3 && (endDegrees + countFactor) > (countFactor+5))
        {
          endDegrees -= countFactor;
          if (endDegrees < 5)
             endDegrees = 5;
          if (endDegrees > 355)
             endDegrees = 360;
        }
       LCD.print("   ", column[0], row[2]); // Print variables to LCD
       LCD.print(String(endDegrees), column[0], row[2]);
       LCD.update();
       delay(200);
       countFactor = 30;
       }
      else  
       countFactor = 5;
    } 
}

char endPointMethod() // Return 1=Manual, 2=Degrees
{
  // Variables
  char method = 1;
  int button;
  // Print LCD
  LCD.clrScr();
  LCD.print("ENDPOINT", CENTER,row[0]);
  LCD.drawLine(0,9,83,9);
  LCD.print("MANUAL", column[1], row[1]);
  LCD.print("DEGREES",column[1], row[2]);
  delay(shortDelay);
  while(1)
  {
    for (char i = 1; i<=2; i++) // Populate checkboxes
     {
      if (i == method)
       LCD.print("*", column[0], row[i]);
      else
       LCD.print("#", column[0], row[i]);
     }
     LCD.update();
   button = buttonCheck();
    if (button != 0)
     {
      if (button == 1)  // Set
        {
          LCD.clrScr();
          LCD.update();
          return(method);
        }
      if (button  == 4 && method == 2) //Up
        method = 1;
      if (button == 3 && method == 1) // Down
        method = 2;
    }
  }
}

char buttonCheck()  // Returns button number
{
 int keyNumber = 0; // Start value
 int keyFirstRead; // First analog read
 int keySecondRead; // Second analog read
 int j = 1; // integer used in scanning the array designating column number
 //int keyNumber; // Number returned from key Check
 int Button[5][3] = {{1, 734, 744}, // button Set
                    {2, 498, 508}, // button Right
                    {3, 321, 331}, // button Down
                    {4, 137, 147}, // button Up
                    {5, 0, 5}}; // button Left
  keyFirstRead = analogRead(buttonPin); // Check analog pin twice
  if (keyFirstRead < 1000) // Key is pressed
    {
     delay(5);
     keySecondRead = analogRead(buttonPin);
    if (max(keyFirstRead, keySecondRead) - min(keyFirstRead, keySecondRead)<5) // check if the read data is same
     {
       for(char i = 0; i <= 4; i++) // loop for scanning the button array.
      {
       // checks the ButtonVal against the high and low vales in the array
       if(keySecondRead >= Button[i][j] && keySecondRead <= Button[i][j+1])
        {
          keyNumber = Button[i][0]; //// stores the button number to a variable
        }
       }
      }
     }
    else
     keyNumber = 0; 
  return keyNumber;
}

void runPan()  // Run the stepper motor
{
  // Variables
  int button;  // To start when pressed
  long exitTimer; // to check if long exit press is happeing
  int longPressTime = 2000; // Milliseconds for a long press
  long stepperStartTime;
  long lcdUpdateTime ; //Timer for LCD screen update every 1000 ms
  long lcdBacklightToggleTime; // Timer for Backlight toggle
  const int lcdBacklightTimeON = 3000; //Milliseconds the LCD stays on
  const long lcdBacklightTimeOFF = 57000; //Milliseconds the LCD stays off
  stepper.setMaxSpeed(800);
  LCD.clrScr();
  LCD.print("READY",CENTER,0);
  LCD.drawLine(0,9,84,9);
  LCD.print("Press Key",CENTER,row[1]);
  LCD.print("to Run",CENTER,row[2]);
  LCD.update();
  delay(shortDelay); // Short pause
  button = analogRead(buttonPin);
  while (button > 1000)              //Wait for key to be pressed before run starts
   button = analogRead(buttonPin);
  lcdBackLightStatus = 1;
  digitalWrite(lcdBackLightPin, lcdBackLightStatus);  // Turn off the LCD display to save power
  LCD.clrScr();
  LCD.print("RUNNING", CENTER, 0);
  LCD.drawLine(0,9,84,9);
  LCD.print("Seconds left",CENTER,12);
  LCD.setFont(TinyFont);
  LCD.print("0%",LEFT, 39);
  LCD.print("100%",RIGHT, 39);
  LCD.setFont(SmallFont);
  LCD.update();
    // Setting up the Stepper
    stepper.moveTo(panEndPosition); 
    stepper.setSpeed(panSpeed);
    // Setting up  timers
    stepperStartTime = millis();  // Panning start time set
    lcdUpdateTime = millis(); //Stating point for the LCD(should update every 1 second)
    lcdBacklightToggleTime = millis()+lcdBacklightTimeOFF;  // Time until LCD Backlight toggle on
    // Turn off the LCD light to save power
    lcdBackLightStatus = 1;
    digitalWrite(lcdBackLightPin, lcdBackLightStatus);
    exitTimer = millis(); // Starting the exit timer
    while (stepper.distanceToGo() != 0)
       {
        // Stopping panning if any key is long pressed
        button = analogRead(buttonPin);
        if (button > 1000)
         exitTimer = millis();
        if (button <1000 && ((millis()-exitTimer) > longPressTime))
         break;
        stepper.runSpeed();
        if (millis()> lcdBacklightToggleTime)
         {
          if (lcdBackLightStatus == 0)  // turnes off the LCD
          {
           lcdBackLightStatus = 1;
           digitalWrite(lcdBackLightPin, lcdBackLightStatus);
           lcdBacklightToggleTime = millis() + lcdBacklightTimeOFF; // How long to stay Off
          }
          else //Turns on the LCD
          {
           lcdBackLightStatus = 0;
           digitalWrite(lcdBackLightPin, lcdBackLightStatus);
           lcdBacklightToggleTime = millis() + lcdBacklightTimeON;  // How long to stay on
          }
         }   
         if (millis()> (lcdUpdateTime+1000)) //check if a second has gone sence last LCD update
         {
          LCD.drawLine(0,46,((panSteps-stepper.distanceToGo())/panSteps*84),46);
          LCD.drawLine(0,47,((panSteps-stepper.distanceToGo())/panSteps*84),47);
          LCD.print("      ", CENTER, 24);
          LCD.print(String((panRunTime)-(lcdUpdateTime/1000)+(stepperStartTime/1000)-1, DEC), CENTER, 24);
          LCD.update();
          lcdUpdateTime = millis(); //reset the timer
         }
       }   
      // Turns on the LCD and heads back to the main menu
      lcdBackLightStatus = 0; 
      digitalWrite(lcdBackLightPin, lcdBackLightStatus);
      stepper.disableOutputs();
      stepper.setCurrentPosition(0);  // Start point to 0
      delay(shortDelay*2);
   }
