#include <ZumoMotors.h>
#include <NewPing.h>
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>
#define THROTTLE_PIN   4 // throttle channel from RC receiver
#define STEERING_PIN   5 // steering channel from RC receiver
#define LED_PIN       13 // user LED pin

#define MAX_SPEED             400 // max motor speed
#define PULSE_WIDTH_DEADBAND   25 // pulse width difference from 1500 us (microseconds) to ignore (to compensate for control centering offset)
#define PULSE_WIDTH_RANGE     350 // pulse width difference from 1500 us to be treated as full scale input (for example, a value of 350 means
                                  //   any pulse width <= 1150 us or >= 1850 us is considered full scale)

#define TRIGGER_PIN  6  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     2  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
#define SENSOR_THRESHOLD 300
#define ABOVE_LINE(sensor)((sensor) > SENSOR_THRESHOLD)
#define INCHES_TO_ZUNITS 17142.0
#define TURN_SPEED 200

//Functions
void autoControl(); //Sets up Out of Bounds and Corner detection
void roomBehaviour(); //Records room data and entry point
void endCorridorBehaviour(); //Dictates behaviour once end of sub corridor is reached
void corridorBehaviour(); //Dictates behaviour up to end of subcorridor
void scanningProtocol(); //Scanning procedure inside rooms


ZumoReflectanceSensorArray reflectanceSensors;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
ZumoMotors motors;
int roomCount = 0; //Keeps track of total number of rooms
int pathCount = 0; //Keeps track of total number of turns
int corridorCount = 0; //Keeps track of total number of corridors
char path[50]; //An array of all turns
String rooms[50]; //An array of all rooms and their data
String currentRoomString; //Stores the data about the current room in form of a string
int currentCorridor; //the corridor you are currently in

unsigned char path_length = 0; // the length of the path
bool humanController = true; //Turns on autopilote
bool isRoom = false; //Tell zumo whether or not it's going into a room
bool isCorridor = false; //Tell zumo whether or not it's in a sub corridor
bool isCorridorEnd = false; //Tell zumo whether sub corridor has ended
bool cornerFlag = false; //Tell zumo it has hit a corner
bool goingBack = false; //Tell zumo it is going back to the start
bool scanEnd = false; //Tell zumo scan is over
char lastTurn; //Hold the last turn made before entering room or corridor

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  //Ensure that Serial is running at the same rate as Processing
  Serial.begin(9600);
  //Initialise the reflectance sensors and make sure speed is 0
  reflectanceSensors.init();
  motors.setSpeeds(0,0);
}

void loop()
{
  //An array of reflectance sensors, reset every loop to give fresh data
  unsigned int sensors[6];
  //Ensure the Serial buffer is empty
  Serial.flush();
  //Get results and push into array
  reflectanceSensors.read(sensors);

  //Testing UC purposes
  //int check = sonar.ping_cm();
  //Serial.println(check);

  //Ensures switch only occurs when a new command has been recieved
  if (Serial.available()>0) {
    char direct = (char) Serial.read();
   
    switch(direct)
    {
      case 'w': case 'W': motors.setSpeeds(65, 50); 
                          Serial.println("Going Forward");
                          humanController = false;  //Sets on autopilot
                          break;
      case 's': case 'S': motors.setSpeeds(-50, -50); 
                          Serial.println("Going Backwards");
                          break;
      case 'd': case 'D': if(scanEnd == true) //If a scan has just ended, check to see if turn is allowed
                          {
                            if(lastTurn == 'R')
                              Serial.println("Cannot go this way");
                            scanEnd = false;
                          }

                          else
                          {
                            motors.setSpeeds(100, -100); 
                            Serial.println("Going Right");
                            delay(700);
                            motors.setSpeeds(0,0);
                            if(cornerFlag == true) //Record the turn at a corner for pathing
                            {
                              path[pathCount] = 'R';
                              Serial.println("Pushing right");
                              pathCount++;
                              cornerFlag = false;
                            }
                          }
                          break;
      case 'a': case 'A': if(scanEnd == true) //If a scan has just ended, check to see if turn is allowed
                          {
                            if(lastTurn == 'L')
                              Serial.println("Cannot go this way");
                            scanEnd = false;
                          }

                          else
                          {
                            motors.setSpeeds(-100, 100); 
                            Serial.println("Going Left");
                            delay(700);
                            if(cornerFlag == true) //Record the turn at a corner for pathing
                            {
                              path[pathCount] = 'L';
                              Serial.println("Pushing left");
                              pathCount++;
                              cornerFlag = false;
                            }
                           
                            motors.setSpeeds(0,0);
                          }
      case 'h': case 'H': motors.setSpeeds(0, 0);
                          humanController = true; 
                          Serial.println("Stopping...");
                          break;
      case 't': case 'T': motors.setSpeeds(65, 55);
                          Serial.println("Resuming Automatic Control"); //Completed turn button
                          humanController = false;
                          break;
      case 'r': case 'R': isRoom = true;
                          Serial.println("Room Entry Acknowledged");
                          break;
     case 'c': case 'C':
                          Serial.println("Corridor Entry Acknowledged");
                          corridorBehaviour();
                          break;
     case 'p': case 'P': scanningProtocol();
                         isRoom = false;
                         break;
     case 'e': case 'E': goingBack = true;
                         humanController = false;
                         motors.setSpeeds(65, 50); 
                         break;
    }
    //Serial.println(direct);
    
    } 

    //If you're not a room or at a corner, and auto pilote is on, allow automatic controls
    if (humanController == false && isRoom == false && cornerFlag == false)
    {
      delay(100);
      autoControl();
    }
    
    if (isRoom == true)
      roomBehaviour();

    //For backtracking
    if(goingBack == true && cornerFlag == true)
    {
      for(int i = 50; i > 0; i--)
        if(path[i] != NULL)
        {
          if(path[i] == 'R')
          {
             motors.setSpeeds(-100, 100); 
             Serial.println("Going Left");
             delay(1000);
             motors.setSpeeds(0,0);
          }
        

        else if(path[i] == 'L')
        {
           motors.setSpeeds(100, -100); 
           Serial.println("Going Left");
           delay(1000);
           motors.setSpeeds(0,0);
        }
        }
       cornerFlag = false;
    }

    

}  

void autoControl()
{
   unsigned int sensors[6];
   reflectanceSensors.read(sensors);
   if(sensors[0] > 400 && sensors[5] > 400 && humanController == false) 
    {
          motors.setSpeeds(0, 0);
          //If you're in a sub corridor, this indicates the end
          if(isCorridor == true)
          {
            Serial.println("End detected");
            humanController = true;
            delay(1000);
            isCorridor = false;
            isCorridorEnd = true;
          }

          //If you're exiting a sub corridor, then use different behaviour
          else if(isCorridorEnd == true)
            endCorridorBehaviour();

          //Else if you're not backtracking, act as normal
          else if (goingBack != true)
          {
            Serial.println("Corner detected");
            humanController = true;
            motors.setSpeeds(0, 0);
            cornerFlag = true;
          }

          else
          {
             Serial.println("Corner detected");
             motors.setSpeeds(0, 0);
             cornerFlag = true;
          }
    }

    //Checks out of bounds logic, only if you're not a corner already
    else if(cornerFlag == false && humanController == false)
    {
      if(sensors[0] > 400)
      {
        motors.setSpeeds(-100, 100);
        delay(50);
        motors.setSpeeds(100, -100);
        delay(500);
        motors.setSpeeds(65, 55);
        Serial.println("Out of Bounds");
      }

      else if (sensors[5] > 400)
      {
        motors.setSpeeds(-100, -100);
        delay(50);
        motors.setSpeeds(-100, 100);
        delay(500);
        motors.setSpeeds(65, 55);
        Serial.println("Out of Bounds");
        
      }
    }

}

void roomBehaviour()
{
  
 Serial.println("waiting...");
 Serial.flush();
 
 while (Serial.available() == 0){}
 char direct = (char) Serial.read();

 //Takes a turn, stores it, acts on turn and provides room data
 if (direct == 'D' | direct == 'd')
  {
    lastTurn = 'R';
    roomCount++;
    currentRoomString = "L" + roomCount + corridorCount; 
    String toPrint = (String)("Entering Room ") + (String)(roomCount) + (String)(" , right , Corridor ") + (String)(corridorCount);
    Serial.println(toPrint);
    motors.setSpeeds(100, -100);
    delay(700);
    motors.setSpeeds(0, 0);
  }

  //Takes a turn, stores it, acts on turn and provides room data
  else if (direct == 'A' | direct == 'a')
  { 
    lastTurn = 'L';
    roomCount++;
    currentRoomString = "R" + roomCount + corridorCount; 
    String toPrint = (String)("Entering Room ") + (String)(roomCount) + (String)(" , left , Corridor ") + (String)(corridorCount);
    Serial.println(toPrint);
    motors.setSpeeds(-100, 100); 
    delay(700);
    motors.setSpeeds(0, 0);
  }
  isRoom = false;
}

void endCorridorBehaviour()
{
    unsigned int sensors[6];
    delay(50);
    reflectanceSensors.read(sensors);
    Serial.println("End of Subcorridor detected");
    
    bool detected = false;
    motors.setSpeeds(-100, -100);
    delay(300);
    motors.setSpeeds(-100, 100);
    delay(1000);
    
    //If either side sensors detect nothing, then this is the end 
    //of the sub corridor
    if((sensors[1] < 300) | (sensors[4] < 300))
    {
      Serial.println("End of Subcorridor detected");
      detected = true;
    }
    motors.setSpeeds(100, -100);
    delay(1000);
    motors.setSpeeds(100, -100);
    delay(1000);
    
    if(((sensors[1] < 300) | (sensors[4] < 300)) && detected == false)
    {
      Serial.println("End of Subcorridor");
      detected = true;
    }
    
    currentCorridor = corridorCount-1; //You are back to your original corridor, one minus the ID of this one
    motors.setSpeeds(0,0);

    //Once you know you're at the end, check to see next instruction
    //If next instruction is the opposite as the turn you used to enter
    //Then you are backtracking and this is not allowed
    while (Serial.available() == 0){}
    char direct = (char) Serial.read();
    if (direct == 'D' | direct == 'd')
    {
        if(lastTurn = 'L')
          Serial.println("Cannot go this way");
        else
        {
          motors.setSpeeds(-100, 100); 
          Serial.println("Going right");
          delay(700);
          motors.setSpeeds(0,0);
        }
          
    }

    else if (direct == 'A' | direct == 'a')
    {
        if(lastTurn = 'R')
          Serial.println("Cannot go this way");
        else
        {
          motors.setSpeeds(100, -100); 
          Serial.println("Going left");
          delay(700);
          motors.setSpeeds(0,0);
        }
          
    }

    //Stop corridor behaviours
    isCorridorEnd = false;
}

void corridorBehaviour()
{
 Serial.println("waiting...");
 Serial.flush();
 
 while (Serial.available() == 0){}
 char direct = (char) Serial.read();
 corridorCount++;
 currentCorridor = corridorCount;
 //Store info on the corridor, as well as the turn before going into the corridor
 if (direct == 'D' | direct == 'd')
  {
    lastTurn = 'R';
    String toPrint = ("Corridor " + (String)corridorCount);
    Serial.println(toPrint);
    motors.setSpeeds(100, -100);
    delay(700);
    motors.setSpeeds(0, 0);
  }
  
  else if (direct == 'A' | direct == 'a')
  {
    lastTurn = 'L';
    String toPrint = ("Corridor " + (String)corridorCount);
    Serial.println(toPrint);
    motors.setSpeeds(-100, 100); 
    delay(700);
    motors.setSpeeds(0, 0);
  }

  Serial.flush();
  //Trigger corridor behaviour
  isCorridor = true;
}

void scanningProtocol()
{
  bool detected = false;
  motors.setSpeeds(65, 50);
  delay(1000);

  //Sweep left for objects
  Serial.println("Scanning left...");
  motors.setSpeeds(100, -100);
  if (sonar.ping_cm() > 0 && sonar.ping_cm() < 15)
  {
       detected = true;
       currentRoomString = currentRoomString + "F"; 
       delay(1000);
       motors.setSpeeds(0,0);
       Serial.println("Survivor Detected");
       delay(1000);
       Serial.println(currentRoomString);
       delay(2000);
  } 
  delay(1000);
  motors.setSpeeds(-100, 100);
  delay(1000);

  //Sweep right for objects
  motors.setSpeeds(-100, 100);
  Serial.println("Scanning right...");
  //If object is picked up, inform user and store information
  if (sonar.ping_cm() > 0 && sonar.ping_cm() < 15 && detected == false)
  {
       Serial.println("Survivor Detected");
       delay(1000);
       currentRoomString = currentRoomString + "F"; 
       rooms[roomCount] = currentRoomString;
       Serial.println(currentRoomString);
       delay(2000);
  }
  delay(1000);
  motors.setSpeeds(100, -100);
  delay(1000);
  motors.setSpeeds(0,0);
  
  //In case UC still does not return values
  //Use button to indicate whether there is a survivor
  Serial.flush();
  if (detected == false)
  {     
        
        while (Serial.available() == 0){}
        char cheat = (char) Serial.read();
           Serial.println("F");
           if(cheat == 'X')
           {
              Serial.println("Survivor Detected");
              delay(500);
              currentRoomString = (currentRoomString + "F");
              rooms[roomCount] = currentRoomString;
           }
           else
           {
              Serial.println("Survivor Not Detected");
              delay(500);
              currentRoomString = (currentRoomString + "N");
              rooms[roomCount] = currentRoomString;
           }      
  }

  //Move back out a little bit
  motors.setSpeeds(-65, -50);
  delay(1000);
  motors.setSpeeds(0, 0);
  scanEnd = true;
}


