# Arduino Panning Box for GoPro with zero motion blure NightLapse option
Arduino Panning Box with Day panning, Night panning, Pannorama photo panning, and Digital lever

This is my first bigger project and I know that the code is not super clean.

New feature? Night Lapse with zero motion blure!
There are manny GoPro panners on the web but I added a new feature I yet have seen, NIGHT PANNING with zero motion blure (even on 30 seconds time lapse). I added a photo resistor to the back cover of the GoPro that let the Arduino know each time a photo is saved. The Arduino then quickly runs the stepper motor and then wait again for next picture getting saved.

https://youtu.be/CbgUIIvogrI

<iframe width="560" height="315" src="https://www.youtube.com/embed/CbgUIIvogrI" frameborder="0" allowfullscreen></iframe>



Summary
Software:
Main menu
  Day Lapse
    - Manual set endpoints or endpoints by degrees
    - Runtime
    - Direction
  Night Lapse
    - Run Time
     (When started the Arduino meassure the exposure time, time between blinks,
      and there after calcualate steps needed per what time to reach full turn)
    - Direction
     (Currently only runs 360 degrees, will implement day lapse settings)
  Panorama
    - Direction
    Run a full turn in 30 seconds, works good for 2 or 5 seconds interval timelapse.
    Those photos are then easy to stitch in Photoshop or other softwares.
  Lever
    - Allow to adjust the level of the box in both axies
  
  Hardware:
  Lasercut Lexan box with tripod thread base
  Ballbering supported center axel with RC car gears
  Stepper motor
  Stepper motor driver
  Nokia LCD
  Arduino Nano
  Accelerometer
  Analog keyboard
  Photo resistor
  
