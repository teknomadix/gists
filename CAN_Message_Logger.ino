// Arduino DUE CANShield Logger
// EVTV: http://store.evtv.me/proddetail.php?prod=ESP32 
// Copyright (C) 2019  Teknomadix.com

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.


#include <SPI.h>
#include <Arduino.h>
#include <due_can.h>
#include <due_wire.h>
#include "variant.h"
#include <Wire_EEPROM.h>
#include <SD.h>


#define Serial SerialUSB
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } //Sets up serial streaming Serial<<someshit;

class EEPROMvariables {
   public:
	uint8_t CANdo;        //Use SERIAL port if 2, CAN0 if 0 or CAN1 if 1
	char logfile[80];
        uint16_t transmitime;
        boolean logger;
        uint32_t datarate;
        uint8_t goodEEPROM;
        CAN_FRAME outFrame;
};

EEPROMvariables myVars;
uint16_t page=200;
uint16_t dummy;
char cmdBuffer[100];
char logstring[200];
char msgBuff[100];
float Version=1.10;
int ptrBuffer;
short logcycle=0;
unsigned long elapsedtime, timestamp,startime, lastime;  //Variables to compare millis for timers
boolean handlingEvent;
boolean debug=1;
File myFile;
char sendfilebuff[70][40]{};
uint8_t nextsendframe=0;
uint8_t inverter=0;
uint8_t motor=0;
uint16_t rpm=0;
uint16_t current=1000;
short voltage=0;
CAN_FRAME inFrame;
 
void setup() {
  
    Wire.begin();   
    Serial.begin(115200);
   
    EEPROM.setWPPin(19); 
    EEPROM.read(page, myVars);
    if (myVars.goodEEPROM!=200)defaults();
    myVars.logger=false; 
    lastime=startime=timestamp=millis();  //Zero our timers
    if(myVars.CANdo==0) initializeCAN(0); //If CANdo is 0, initialize CAN0 
    if(myVars.CANdo==1) initializeCAN(1);    //If CANdo is 1, initialize CAN1 
     if(myVars.CANdo==2)
       { 
        initializeCAN(0);    //If CANdo is 2, initialize both CAN1 and CAN2
   	initializeCAN(1);    
   	}        
    initializeMicroSD();
    Serial<<"\n\n Startup successful. EVTV Motor Werks -  CANDue Logger Version: "<<Version<<"\n\n";
    printMenu();
     if(readSENDfile())getNextSENDFrame(nextsendframe++);
    Serial<<"Current logfile: "<< myVars.logfile<<"\n\n";
   
}



void loop() 
{
    if(millis()-lastime > myVars.transmitime)  //If we reach our transmitime, send a frame and print status
          {
            lastime=millis();
            if(myVars.outFrame.id>0)sendCAN(myVars.CANdo);
            if(logcycle++ > 20) 
              {  
               EEPROM.write(page, myVars);
                logcycle=0;
                if(sendfilebuff[nextsendframe][0]>0)
                  {
                    getNextSENDFrame(nextsendframe++);
                  }
                else{nextsendframe=0;}
              } 
          }     
     
     
}

void serialEventRun(void) 
{
   if (Serial.available())serialEvent(); //If serial event interrupt is on USB port, go check for keyboard input           
}

void serialEvent() {
	int incoming;
	incoming = Serial.read();
	if (incoming == -1) { //false alarm....
		return;
	}

	if (incoming == 10 || incoming == 13) { //command done. Parse it.
		handleConsoleCmd();
		ptrBuffer = 0; //reset line counter once the line has been processed
	} else {
		cmdBuffer[ptrBuffer++] = (unsigned char) incoming;
		if (ptrBuffer > 79)
			ptrBuffer = 79;
	}
}

void handleConsoleCmd() {
	handlingEvent = true;

	
		if (ptrBuffer < 5 ) { //command is a single ascii character
			handleShortCmd();
		} else { //if cmd over 1 char then assume (for now) that it is a config line
			handleConfigCmd();
		}
	
	handlingEvent = false;
}
void handleConfigCmd() {
	int i;
	int newValue;
	
	if (ptrBuffer < 6) return; //4 digit command, =, value is at least 6 characters
	cmdBuffer[ptrBuffer] = 0; //make sure to null terminate
	String cmdString = String();
	unsigned char whichEntry = '0';
	i = 0;

	while (cmdBuffer[i] != '=' && i < ptrBuffer) 
          {
	   cmdString.concat(String(cmdBuffer[i++]));
	   }
	i++; //skip the =
	if (i >= ptrBuffer)
	    {
		Serial<<("Command needs a value..ie FILE=myfile.log");
		return; //or, we could use this to display the parameter instead of setting
	    }

	// strtol() is able to parse also hex values (e.g. a string "0xCAFE"), useful for enable/disable by device id
	newValue = strtol((char *) (cmdBuffer + i), NULL, 0);
	cmdString.toUpperCase();

	 
       if (cmdString == String("FILE")) {
		String cmdString = String();
    	        cmdString.concat((char *)(cmdBuffer + i));
                cmdString.toCharArray(myVars.logfile,80);      
                Serial<<"Opening new log file: "<<myVars.logfile<<"\n";
                //makeDir();
                //logToFile("Opening New Logfile\n");
        } else if (cmdString == String("MARK")) {
		String cmdString = String();
    	        cmdString.concat((char *)(cmdBuffer + i));
                cmdString.concat("\n");
                cmdString.toCharArray(logstring,200);      
                 if(myVars.logger){
                    Serial<<"Logging:  "<<logstring<<"\n";
                    logToFile(logstring);
                    }
                else Serial<<"Logging disabled...\n\n";
	} else if (cmdString == String("SEND")) {
		String cmdString = String();
    	        cmdString.concat((char *)(cmdBuffer + i));
                char sendframe[80];
                cmdString.toCharArray(sendframe,80);      
                Serial<<"New SEND frame: "<<sendframe<<"\n";
                sscanf(sendframe,"%03X %02X %02X %02X %02X %02X %02X %02X %02X", &myVars.outFrame.id, 
                 &myVars.outFrame.data.bytes[0], &myVars.outFrame.data.bytes[1], 
          &myVars.outFrame.data.bytes[2], &myVars.outFrame.data.bytes[3], &myVars.outFrame.data.bytes[4], 
          &myVars.outFrame.data.bytes[5], &myVars.outFrame.data.bytes[6], &myVars.outFrame.data.bytes[7]);
          Serial<<myVars.outFrame.id<<"  "<<myVars.outFrame.data.bytes[0]<<"\n";
          myVars.outFrame.extended=0;
	 } else if (cmdString == String("SENDL")) {
		String cmdString = String();
    	        cmdString.concat((char *)(cmdBuffer + i));
                char sendframe[100]="";
                cmdString.toCharArray(sendframe,100);      
                Serial<<"New SEND frame: "<<sendframe<<"\n";
                sscanf(sendframe,"%08X %02X %02X %02X %02X %02X %02X %02X %02X", &myVars.outFrame.id, 
                 &myVars.outFrame.data.bytes[0], &myVars.outFrame.data.bytes[1], 
          &myVars.outFrame.data.bytes[2], &myVars.outFrame.data.bytes[3], &myVars.outFrame.data.bytes[4], 
          &myVars.outFrame.data.bytes[5], &myVars.outFrame.data.bytes[6], &myVars.outFrame.data.bytes[7]);
          Serial<<myVars.outFrame.id<<"  "<<myVars.outFrame.data.bytes[0]<<"\n";
          myVars.outFrame.extended=1;
	} else if (cmdString == String("RATE") ) {
		Serial<<"Setting CAN to send frame every: "<< newValue<<" msec\n\n";
		myVars.transmitime = newValue-1;
        } else if (cmdString == String("KBPS") ) {
		Serial<<"Setting CAN data rate to: "<< newValue<<" kbps\n\n";
		myVars.datarate = newValue*1000;
                initializeCAN(myVars.CANdo); //If CANdo is 0, initialize CAN0 
                 
    
	} else if (cmdString == String("LOGLEVEL")) {
		switch (newValue) {
		case 0:
			Serial<<("setting loglevel to 'debug'");
			break;
		case 1:
			Serial<<("setting loglevel to 'info'");
			break;
		case 2:
			Serial<<("setting loglevel to 'warning'");
			break;
		}
}
}
void handleShortCmd() 
{
	uint8_t val;
	
	switch (cmdBuffer[0]) 
        {
	  case 'h':
	  case '?':
	  case 'H':
		printMenu();
		break;
	  case 'L':
                if(myVars.logger)
                  {    
                    myVars.logger=false;
                    logToFile("============== END OF LOG SESSION ==============\n");
                    closeFile();
                    Serial<<"Setting data logger to OFF.."<<myVars.logfile<<"..\n";
                   }
                  else
                    { 
                    openFile();
                    logToFile("============== NEW LOG FILE BEGINS ==============\n");
                    Serial<<"Setting data logger to ON.."<<myVars.logfile<<"..\n";
                    myVars.logger=true;                    
                     }
		break;
	case 'l':
                if(myVars.logger)
                  {
                    myVars.logger=false;
                    logToFile("============== END OF LOG SESSION ==============\n");
                    closeFile();
                    Serial<<"Setting data logger to OFF.."<<myVars.logfile<<"..\n"  ;                                      
                  }
                  else
                    { 
                     
                    openFile();
                    logToFile("============== NEW LOG FILE BEGINS ==============\n"); 
                    myVars.logger=true; 
                     Serial<<"Setting data logger to ON.."<<myVars.logfile<<"..\n";  
                    }
                      
		break;
	case 'U':
		break;
        case 'D':
      		debug=(!debug);
                Serial<<"Debug= "<<debug<<"\n";
      		break;
	case 'd':
      		debug=(!debug);
                Serial<<"Debug= "<<debug<<"\n";
      		break;
	
      	case 'c':
      		getPort();
      		break;
      	case 'C':
      		getPort();
      		break;
      	
      	case 'p':
      		printLog();
      		break;
      	case 'P':
      		printLog();
      		break;
      
      	case 'i':
                
      		cardInfo();
      		break;
      	case 'I':
      		cardInfo();
      		break;
      	
      	
	}
      cmdBuffer[0]=0;
}


void getPort()

{
	        int P = atoi(cmdBuffer+1);	
		myVars.CANdo=P;
		if(myVars.CANdo>2) Serial<<"Entry out of range, enter 0, 1 or 2 \n";
                  else 
                  {
                    if(myVars.CANdo==0)
                     {
                       Can1.disable();
                       Can0.enable();
                       initializeCAN(0); //If CANdo is 0, initialize CAN0 
                     }
                    if(myVars.CANdo==1)
                     {
                       Can0.disable();
                       Can1.enable();
                       initializeCAN(1);    //If CANdo is 1, initialize CAN1
                     }
                    if(myVars.CANdo==2)
                     { 
                        Can0.enable();
                        Can1.enable();
                        initializeCAN(0);    //If CANdo is 2, initialize both CAN1 and CAN2
   		        initializeCAN(1);    
   		     }           
                  }
}



void initializeCAN(int which)
{
  if(which)  //If 1, initialize Can1.  If 0, initialize Can0.
    {
      pinMode(48,OUTPUT);
      Can1.enable();
      if (Can1.begin(myVars.datarate,48)) 
        {
          Can1.setRXFilter(1,0,0, false);
          Can1.setRXFilter(2,0,0, true);
           
          Can1.setGeneralCallback(handleFrame);
          Serial<<"\n\nUsing CAN1 - initialization completed at "<<myVars.datarate<<" \n";
        }
        else Serial.println("\nCAN1 initialization (sync) ERROR\n");
    } 
  else
    {
      pinMode(50,OUTPUT);
      Can0.enable();
     if (Can0.begin(myVars.datarate,50)) 
        {  
          Can0.setRXFilter(3,0,0, false);
          Can0.setRXFilter(4,0,0, true);
          Can0.setGeneralCallback(handleFrame);
          Serial<<"\n\nUsing CAN0 - initialization completed at "<<myVars.datarate<<" \n";      
         }
        else Serial.println("\nCAN0 initialization (sync) ERROR\n");
    }
 
}   

void initializeMicroSD()
{
   pinMode(SS, OUTPUT);
   
    if (!SD.begin(SS)) 
      {
      Serial.println("MicroSD card failed, or not present...\n");
    	return;
      }
    else 
      {
        Serial.println("MicroSD card initialized....\n");
        
      }	
}
void printLog()
{
   myFile = SD.open(myVars.logfile);
    if (myFile) 
      {
       while (myFile.available()) 
          {
            Serial.write(myFile.read());
    	  }
      }
    
  myFile.close();
}
void openFile()
{
    myFile=SD.open(myVars.logfile,FILE_WRITE);
      if (myFile)Serial<<"Success...microSD file open...\n";
      else Serial<<"Could not open microSD file...\n";
}


void closeFile()
{
  myFile.close();

}

    
void logToFile(char *logstring)
{
   
    if (myFile) 
      {
         myFile.print(logstring);
      }
      else Serial<<"MicroSD file is not open...\n\n";
      
}

void handleFrame(CAN_FRAME *frame)

{
// Serial<<"Got one... \n";
 
    int milliseconds = (int) (millis()/1) %1000 ;
    int seconds = (int) (millis() / 1000) % 60 ;
    int minutes = (int) ((millis() / (1000*60)) % 60);
    int hours   = (int) ((millis() / (1000*60*60)) % 24);
    
    //Serial<<"%03x"<<frame->id<<"\n";
  
    
     sprintf(logstring,"Rcvd msgID: 0x%03X; %02X; %02X; %02X; %02X; %02X; %02X; %02X; %02X  %02d:%02d:%02d.%03d\n", frame->id, frame->data.bytes[0], 
     frame->data.bytes[1],frame->data.bytes[2], frame->data.bytes[3], frame->data.bytes[4], frame->data.bytes[5], frame->data.bytes[6],
     frame->data.bytes[7], hours, minutes, seconds, milliseconds);
  
  if(debug)Serial<<logstring;
   
   if(myVars.logger)logToFile(logstring);
}

void printMenu()
{
  Serial<<"\f\n=========== CANDue Logger Program Version "<<Version<<" ==============\n************ List of Available Commands ************\n\n";
  Serial<<"  C1  - CAN bus selector\n         C0=CAN0\n         C1=CAN1\n         C2=CAN0 and CAN1\n";
  Serial<<"  L - toggles CAN data logging between ON and OFF\n";
  Serial<<"  D - toggles DISPLAY FRAMES off/on to print recieved and sent CAN data traffic\n";
  Serial<<"  FILE=logfile.csv    sets the microSD file name to log to\n";
  Serial<<"  SEND=050 FF 10 01 CA 02 BB 01 77         send standard ID CAN frame - hexadecimal format\n";
  Serial<<"  SENDL=18FF30CC FF 10 01 CA 02 BB 01 77   send extended ID CAN frame - hexadecimal format\n";
  Serial<<"  RATE=500   The frequency in milliseconds to send stored CAN message \n";
  Serial<<"  KBPS=250   Change CAN bus data rate in kbps \n";
  Serial<<"  MARK=Turned on Air Conditioner Here    - Copies string to log file as a marker\n";
  Serial<<"  P - prints current logfile to screen\n";
  Serial<<"  I - displays microSD card operating paramters\n";
  Serial<<"  MOSI:"<<MOSI<<" MISO:"<<MISO<<" SCK:"<<SCK<<"  SS:"<<SS<<"\n\n";  
  Serial<<"**************************************************************\n==============================================================\n\n";
  
  
  
}


void sendCAN(int which)

{
	
        int milliseconds = (int) (millis()/1) %1000 ;
        int seconds = (int) (millis() / 1000) % 60 ;
        int minutes = (int) ((millis() / (1000*60)) % 60);
        int hours   = (int) ((millis() / (1000*60*60)) % 24);
        
        char buffer[140];
    
        sprintf(buffer,"Sent msgID 0x%03X; %02X; %02X; %02X; %02X; %02X; %02X; %02X; %02X  %02d:%02d:%02d.%03d \n", myVars.outFrame.id, 
        myVars.outFrame.data.bytes[0], myVars.outFrame.data.bytes[1], myVars.outFrame.data.bytes[2], 
        myVars.outFrame.data.bytes[3], myVars.outFrame.data.bytes[4], myVars.outFrame.data.bytes[5], 
        myVars.outFrame.data.bytes[6], myVars.outFrame.data.bytes[7], hours, minutes, seconds, milliseconds);
   
       myVars.outFrame.length=8;
       myVars.outFrame.fid=0;
       myVars.outFrame.rtr=0;
       myVars.outFrame.priority=0;
   
   
        if(debug)Serial<<buffer;
           
        if(myVars.CANdo==0) Can0.sendFrame(myVars.outFrame);    //Mail it
         else Can1.sendFrame(myVars.outFrame);  
       if(myVars.logger)logToFile(buffer); 
      //countup(); 
}

void cardInfo()
{
  Sd2Card card;
SdVolume volume;
SdFile root;


const int chipSelect = 10;    

   Serial.print("\nInitializing SD card...");
   pinMode(10, OUTPUT);     // change this to 53 on a mega
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card is inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
   Serial.println("Wiring is correct and a card is present."); 
  }

  // print the type of card
  Serial.print("\nCard type: ");
  switch(card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }


  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  Serial.println();
  
  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize *= 512;                            // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);

  
  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);
  
  // list all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);


}

void makeDir()
{
  if( !SD.exists(myVars.logfile) ) 
    {
      if( SD.mkdir(myVars.logfile) ) 
        {
          Serial.print("File directory created.... \n");
        } 
      else Serial.print("Unable to create file directory...\n");
    }

  else Serial.print("File directory already exists... \n");
}

void defaults()
{
   
  sprintf(myVars.logfile,"default.txt");
  Serial<<"Setting default logfile: "<<myVars.logfile<<"\n";
  myVars.CANdo=0; 
  myVars.transmitime=500;  
  myVars.logger=false;
  myVars.outFrame.length = 8;  // Data payload 8 bytes
  myVars.outFrame.rtr = 0;  // Data payload 8 bytes
  myVars.goodEEPROM=200;
  myVars.datarate=500000;

}

bool readSENDfile()
{
   myFile=SD.open("SEND.txt");
      if (myFile)
      {
       Serial<<"SEND file detected and open...\n";    
       int i=0;
       int j=0;
       while (myFile.available()) 
          {
            sendfilebuff[i][j++]=myFile.read();
            Serial<<sendfilebuff[i][j-1];
            if(sendfilebuff[i][j-1]==10)
              {
                i++;
                j=0;
              }
    	  }
        myFile.close();
        return true;
       }
   else 
   {
     Serial<<"No SEND file detected...\n";
      return false;
   }

}

void getNextSENDFrame(int next)
{
 sscanf(sendfilebuff[next],"%03X %02X %02X %02X %02X %02X %02X %02X %02X", &myVars.outFrame.id, 
                 &myVars.outFrame.data.bytes[0], &myVars.outFrame.data.bytes[1], 
          &myVars.outFrame.data.bytes[2], &myVars.outFrame.data.bytes[3], &myVars.outFrame.data.bytes[4], 
          &myVars.outFrame.data.bytes[5], &myVars.outFrame.data.bytes[6], &myVars.outFrame.data.bytes[7]);
          //Serial<<myVars.outFrame.id<<"  "<<myVars.outFrame.data.bytes[0]<<"\n";
          myVars.outFrame.extended=0;  
}

void countup()
{
  
  myVars.outFrame.data.bytes[0]=highByte(rpm);
  myVars.outFrame.data.bytes[1]=lowByte(rpm++);
  myVars.outFrame.data.bytes[2]=inverter++;
  myVars.outFrame.data.bytes[3]=motor++;
   myVars.outFrame.data.bytes[4]=highByte(current);
  myVars.outFrame.data.bytes[5]=lowByte(current--);
   myVars.outFrame.data.bytes[6]=highByte(voltage);
  myVars.outFrame.data.bytes[7]=lowByte(voltage++);
}
