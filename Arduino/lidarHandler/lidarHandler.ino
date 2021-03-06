#include <SoftwareSerial.h>
#include <EtherCard.h>
#include <IPAddress.h>

//should limit the software serial buffer to 9 bytes.
//this will really help with speed and ram ussage
#define _SS_MAX_RX_BUFF 9

//IPaddress. lets use a static address
static byte ip[] = {10,0,0,9};
static byte gw[] = {192,168,1,1};//{10,0,0,1};
static byte dnsip[] = { 8,8,8,8 };
static byte netmask[] = { 255,255,255,0 };
static byte mac[] = {0x69,0xAD,0x45,0x92,0xFF,0x9F};
static byte mcIP[] = {192,168,1,102};//{10,0,0,103};
byte Ethernet::buffer[500];
int mcPort = 8989;
int localPort = 4400;

//need to figure out what the packet size we will be recieving
char ethernetBuffer[20];

//README
/*
 * Red    - 5v power
 * Black  - ground
 * Blue   - lidar output (arduino rx pin) 3.3v
 * White  - lidar input (arduino tx pin) 3.3v
 * 
 * This should be good to go just do not plug the
 * white wire into the arduino. We dont need to write
 * anything to it and if you do with out a voltage
 * divider, it will most likly break the lidar
 * 
 * Ethernet protocol
 * 
 * 0x01
 * far left - 2 bytes
 * left
 * middle
 * right
 * far right
 * checksum - 1 byte
 * 
 */

//total lidars to be used. index 4 is hardware
#define NUM_MODULES 5
//total samples to take for averaging. only 8 will be used
#define TOTAL_SAMPLES 10
unsigned int lowerByte = 0;
unsigned int upperByte = 0;
unsigned int lowerStrength = 0;
unsigned int upperStrength = 0;
//the order is what will be sent
/*
 * far left
 * left
 * middle
 * right
 * far right
 */
SoftwareSerial lidars[4] = {SoftwareSerial(2,3), SoftwareSerial(4,5), SoftwareSerial(6,7), SoftwareSerial(8,9)};
unsigned int distances[5];
unsigned int strengths[5];
int numSampled = 0;
unsigned int tmpDistances[5][10];

unsigned long lastSendTime = 0;

void setup() {
  lidars[0].begin(115200);
  lidars[1].begin(115200);
  lidars[2].begin(115200);
  lidars[3].begin(115200);
  Serial.begin(115200);

  if (ether.begin(sizeof Ethernet::buffer, mac, 10) == 0)
    Serial.println("Failed to access Ethernet controller");
  ether.staticSetup(ip, gw, dnsip, netmask);

  //this would allow us to listen to a local port
  //ether.udpServerListenOnPort(&handleMessage, localPort);
  
  lastSendTime = millis();
}

void sampleLidar(int lidarPos){
  bool gotSample = false;
  if(lidarPos == 4){
    //hardware serial port
    while(!gotSample && Serial.available()){
      if(Serial.read() == 0x59 && Serial.read() == 0x59){
        lowerByte = Serial.read();
        upperByte = Serial.read();
        lowerStrength = Serial.read();
        upperStrength = Serial.read();
        tmpDistances[4][numSampled] += (upperByte << 8) + lowerByte;
        //strength[4] += (upperStrength << 8) + lowerStrength;
        numSampled++;
        Serial.read();
        Serial.read();
        gotSample == true;
      }else{
        Serial.read();
      }
    }
  }else{
    while(!gotSample && lidars[lidarPos].available()){
      if(lidars[lidarPos].read() == 0x59 && lidars[lidarPos].read() == 0x59){
        lowerByte = lidars[lidarPos].read();
        upperByte = lidars[lidarPos].read();
        lowerStrength = lidars[lidarPos].read();
        upperStrength = lidars[lidarPos].read();
        tmpDistances[lidarPos][numSampled] += (upperByte << 8) + lowerByte;
        //strength[lidarPos] += (upperStrength << 8) + lowerStrength;
        numSampled++;
        lidars[lidarPos].read();
        lidars[lidarPos].read();
        gotSample == true;
      }else{
        lidars[lidarPos].read();
      }
    }
  }
}

void loop() {
  unsigned char buf[12];
  unsigned char checksum = 0;
  //samples each one 10 times then sends the data to the jetson
  for(int i = 0; i < 10; i++){
    //cycle through the lidars
    for(int lidarPos = 0; lidarPos < 5; lidarPos++){
      sampleLidar(lidarPos);
    }
  }

  for(int i = 0; i < NUM_MODULES; i++){
    int max = 0;
    int maxIndex = 0;
    int min = 999;
    int minIndex = 0;
    for(int j = 0; j < TOTAL_SAMPLES; j++){
      if(tmpDistances[i][j] > max){
        max = tmpDistances[i][j];
      }else if(tmpDistances[i][j] < min){
        min = tmpDistances[i][j];
      }
    }
    
    for(int j = 0; j < TOTAL_SAMPLES; j++){
        if(j == maxIndex || j == minIndex){
          //do nothing
        }else{
          distances[i] += tmpDistances[i][j];
        }
    }
  
    distances[i] /= 8;
  }

  //put data into buffer
  buf[0] = 0x01;
  for(int i = 0; i < 5; i++){
    buf[i+1] = distances[i] >> 8;
    buf[i+2] = distances[i];
    checksum += (distances[i] >> 8) + distances[i];
  }

  while(millis() - lastSendTime);

  //write the buffer
  ether.sendUdp(buf,sizeof(buf), localPort, mcIP, mcPort);
  lastSendTime = millis();
  
  ether.packetLoop(ether.packetReceive());
}
