/* Uses potentiometers and buttons on a master arm to control
 *  the slave arm on the rover.
 * This program is just for collecting the position from the master
 * The data is sent over USB serial to a computer (probably an UPboard)
 * 
 * ORDERING:
 * START | ID | shoulderPosition | elbowPosition | pitchDown | pitchUP | rollLeft/CCW | rollRight/CW | closeHand | openHand | average of all values
 * 
 * Notes:
 * -Buttons just report if the button is pressed or not, movement must be controlled at the master
 * -ID must be known by the serial master (computer)
 * -Uses 9600 baud
 * -If the average is disagreed by serial master and slave something went wrong
 * 
 * Protocol Used:
 *
 *  (-127) | USERID | DATA | DATA | ... | Average of all sent values |
 *  -127 is the start message 
 *  USERID is an int used to identify which serial device is running the program
 *  Average of all sent values is to ensure no data loss and for debugging
 *  
 *  Potentiometer Values:
 *  The output to serial is in units of degrees and is sent as a 16 bit integer (your programming language should handle the bits you just need to know it's an integer)
 *  
 *  Buttons:
 *  The buttons are stored in the 7th byte of the message. (data[6])
 *  The byte has this structure (button 3)(button 2)(Button 1)(Button 0)(Button 3)(Button 2)(Button 1)(Button 0) 
 *  where 0 is unpressed and 1 is pressed. 
 *  
 *  To parse this byte I'd recomend using this method:
 *  To test if a specific bit is 0 or 1 the formula (byte) XOR (2**n) where n = 0 is the least significant bit
 *  will return an integer 2**n smaller than the original integer if the bit is 0. If the output of this XOR is the same number then that bit is not a 1.
 */

//Serial Communication ID so the computer knows what device this is.
int ID = 1; 



//Port Numbers 
int buttons[4] = {0,1,2,3}; //rollLeft (CCW from the rover's perspective), rollRight (CW from the rover's perspective), let go, grab (digital ports connected to buttons)
int joints[4] = {0,1,2,3}; //yaw, shoulder, elbow, wrist pitch (the analog ports the potentiometers are connected to)

void setup() {
  //Start talking to the computer
  Serial.begin(9600);
}

void loop() {
  
  //this doesn't have to be in a seperate function I did this in case we want to add stuff not taking data later.
  sendData(); 
}

void sendData(){
  /*
  data is used as a "packet" for when we send it over serial (don't worry about the floating point math the int typecast automatically converts it after but it will throw a warning oh well)
  0.263671875 is the value 270°/1024 to convert the potentiometer output to degrees (5v/1024 represents 270°)
  NOTE: The converstion to a int restricts our accuracy to a minimum of ~4° of accuracy and to type on a keyboard we need 1.2°)
  */
  int data[7];
  data[0] = -127; //Start command to tell the computer we are sending data
  data[1] = ID; //ID number to identify the arduino this is coming from.
  data[2] = (int)analogRead(joints[0])*0.263671875; //Get the data from the Yaw Pot. (rotate arm horizontally)
  data[3] = (int)analogRead(joints[1])*0.263671875; //Get the data from the Shoulder Pot. (the bottom joint)
  data[4] = (int)analogRead(joints[2])*0.263671875; //Get the data from the Elbow Pot. (the second from the bottom joint)
  data[5] = (int)analogRead(joints[3])*0.263671875; //Get the data from the Wrist Pitch Pot. (rotate the finger/hand pitch)
  data[6] = 0; //Set all the button data to default to 0 (low)
  

  //Figure out if the buttons are being pressed or not
  for(int i = 0; i<4; i++){
    
    //Set the digital port i to pinmode of input so we can read data from it.
    pinMode(buttons[i], INPUT);
    
    //If the button is pressed run this if statement
    if(digitalRead(buttons[i] = 0) == HIGH){
      
      /*
       * Make data[6] a byte with values:
        (button 3)(button 2)(Button 1)(Button 0)(Button 3)(Button 2)(Button 1)(Button 0) and each value is 1 (high) or 0 (low)
        data[6] starts as 00000000 and by bitwise XOR'ing the value we store the button values as 1 or 0
        There are 4 buttons and since we have an extra 4 bits the sequence repeats for redundancy
      */
      data[6]=data[6]^(int)pow(2,i)^(int)pow(2,4+i); //pow(2,i) edits the least 4 significant bits and pow(2,4+i) edits the 4 most significant bits
    }
  }
  
  //Set this variable to 0 so we can average it at the end
  int sum = 0;
  
  //Cycle throught all elements of the message and send
  for(int z = 0; z<7; z++){
    
    //Send the data array element by element (will be seen by reader as a series of integers)
    Serial.write(data[z]);
    
    //Increase the sum by the data value so we can average it out at the end
    sum = sum + data[z];
  }
  Serial.write(sum/7);
}