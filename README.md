# ProgrammingThingsAssignment
 - Search and Rescue

In this assignment I programmed a Arduino board with a Zumo shield to run a search and rescue program

The scenario motivating this assignment is to imagine that the robot is trying to find/rescue people trapped in a building, the robot will initially be aided by human input through corners and rooms and detect if there are any civilians to be rescued.
It will then remember it's route and go back, optimally avoiding empty rooms and checking others to see if there is anyone trapped.

For the Zumo this meant that we needed to make use of its line sensor (to stay within the maze), add an ultrasonic sensor (to detect objects in rooms) as well Xbee Bluetooth device to communicate with a remote laptop acting as the ‘base’.
The ‘building’ consist of a corridor with corners and adjoining rooms. The boarders of the building are set out with black lines on a white background. 

## Video of Robot in Action
https://drive.google.com/file/d/1L2UD52781hccR_ZKhagTQc6duNPfSp-r/view?usp=sharing

## Quick Operation Overview
The way in which my robot completes the course, is by using the line detector to make small adjustments to it's position. If the detector only senses a line on ONE the far sensors
it knows that it is not at a corner but hitting an edge and so it adjusts. If it detects a line on two outer senses, it knows it is at a corner and adjusts.
I use flags to set behaviours with the zumo, using a GUI of buttons in programming. It then pushed each of the rooms and corner data onto a stack, in order to remember the route back.

| Positon 		    | Behaviour                                                                                                                                               |
|---------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------|
| In Corridor               | Allow user to set position, then move automatically through until you hit a dead end. At this point, the user turns you back around. When it hits a     |
|                           | second dead-end, it will check left and right to see if it is at an intersection, if so, it knows the sub corridor has ended.                           |
| In Room                   | Log and display the room and corridor number, wait for the command to move in a search. Sweeps left and right to check for objects/people inside.       |                                                                   
| End of Maze               | Turn around and retrace path, ignoring rooms without citizens                                                                                           |
| At a corner               | Use right and left sensors to detect corner, stop, note the turn into the path array and allow manual repositioning.                                    |
With these estimates the robot is fully equipped with instructions of how to handle
each situation it may encounter in the maze ­ fulfilling its job of going through the
entire maze and finding people in rooms.

##Key Issues / Challenges
###Staying within the walls: 
This issue is key to being able to complete the course at
all, with the main struggle defining between a corner and a side. However using the outer most
sensors allows you to get a semi-reliable behaviour in this regard.

After some tinkering, I ensured that any adjustment the Zumo made was small enough to not throw it off course completely
and the code detecting the line can often rectify it's own mistakes.


###Detecting 'people' using the UC sensor 
Due to a wiring fault, my UC didn't work after a short period of time. However, I managed to predict 
the coding required for it and map the behaviour anyway, however in case it wouldn't work still without a wire
I also implemented a button to provide whether or not a citizen is in the room. This way the map provided would still be accurate.

###Mapping back 
This proved extremely difficult. Due to my map design, it was incredibly difficult for the zumo to detect 
rooms and corridors without constantly turning left and right. Due to the small difference between the doorway
sizes, I ended up being unable to fully implement this. Instead my backtrack code simply followed the corners taken
with the user having to manual do room searches. In the end, I left the code in but decided not to demonstrate the backtracking.

###Preventing backtracking during stage 1: 
In order to remember not to turn back on itself, I had to create a global variable "lastTurn" to hold the last turn the zumo made after I indicated
it was in a room or corridor. This was held until another room declared itself.

###Setting behaviours
I used booleans flags extensively to set different behaviours when the zumo was in a sub corridor or room. These flags allowed me to easily turn off
and on certain features while avoiding complicated compare statements to detect whether they were still in a room or corridor.

## Acknowledgement and Sources
Zumo and Arduino Library files


## License

Copyright © 2017 Maria Khan