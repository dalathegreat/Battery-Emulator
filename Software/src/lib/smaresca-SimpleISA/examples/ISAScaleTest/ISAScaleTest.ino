


#include <due_can.h>
#include "variant.h"
#include <SimpleISA.h>

 #define Serial SerialUSB //Use native port
 template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } //Allow streaming

float Version=2.00;
uint16_t loopcount=0;
unsigned long startime=0;
unsigned long elapsedtime=0;
uint port=0;
uint16_t datarate=500;

 ISA Sensor;  //Instantiate ISA Module Sensor object to measure current and voltage 


void setup() 
{
  Serial.begin(115200);
  Sensor.begin(port,datarate);  //Start ISA object on CAN 0 at 500 kbps
   
  Serial<<"\nISA Scale Startup Successful \n";
 
  printMenu();
}

void loop()
{
  if(loopcount++==40000)
    {
        printStatus();
        loopcount-0;
    }    
        checkforinput(); //Check keyboard for user input 
}
 
 
void printStatus()
{
  char buffer[40];
   //printimestamp();  
  
   sprintf(buffer,"%4.2f",Sensor.Voltage); 
   Serial<<"Volt:"<<buffer<<"v ";
    sprintf(buffer,"%4.2f",Sensor.Voltage1); 
   Serial<<"V1:"<<buffer<<"v ";
   sprintf(buffer,"%4.2f",Sensor.Voltage2); 
   Serial<<"V2:"<<buffer<<"v ";
   sprintf(buffer,"%4.2f",Sensor.Voltage3); 
   Serial<<"V3:"<<buffer<<"v ";
  
  sprintf(buffer,"%4.3f",Sensor.Amperes); 
   Serial<<"Amps:"<<buffer<<"A ";

  sprintf(buffer,"%4.3f",Sensor.KW); 
   Serial<<buffer<<"kW ";
  
   sprintf(buffer,"%4.3f",Sensor.AH); 
   Serial<<buffer<<"Ah "; 

   sprintf(buffer,"%4.3f",Sensor.KWH); 
   Serial<<buffer<<"kWh"; 
   
   sprintf(buffer,"%4.0f",Sensor.Temperature); 
   Serial<<buffer<<"C "; 
   
   Serial<<"Frame:"<<Sensor.framecount<<" \n";
 }

 void printimestamp()
{
  //Prints a timestamp to the serial port
    elapsedtime=millis() - startime;
    
    int milliseconds = (elapsedtime/1) %1000 ;
    int seconds = (elapsedtime / 1000) % 60 ;
    int minutes = ((elapsedtime / (1000*60)) % 60);
    int hours   = ((elapsedtime / (1000*60*60)) % 24);
    char buffer[19]; 
    sprintf(buffer,"%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
    Serial<<buffer<<" ";
}

void printMenu()
{
   Serial<<"\f\n=========== ISA Scale  Sample Program Version "<<Version<<" ==============\n************ List of Available Commands ************\n\n";
   Serial<<"  ?  - Print this menu\n ";
   Serial<<"  d - toggles Debug off and on to print recieved CAN data traffic\n";
   Serial<<"  D - toggles Debug2 off and on to print derived values\n";
   Serial<<"  f  - zero frame count\n ";
   Serial<<"  i  - initialize new sensor\n ";
   Serial<<"  p  - Select new CAN port\n ";
   Serial<<"  r  - Set new datarate\n ";
   Serial<<"  z  - zero ampere-hours\n ";
   
   Serial<<"**************************************************************\n==============================================================\n\n";
   
}

void checkforinput()
{ 
  //Checks for keyboard input from Native port 
   if (Serial.available()) 
     {
      int inByte = Serial.read();
      switch (inByte)
       	{
       	  case 'z':    //Zeroes ampere-hours
              Sensor.KWH=0;
              Sensor.AH=0;
      		    Sensor.RESTART();
      		  break;
          
          case 'p':    
             getPort();
            break;
          
	        case 'r':    
             getRate();
            break;
          
  
	        case 'f':    
      		    Sensor.framecount=0;
      		  break;
        
          case 'd':     //Causes ISA object to print incoming CAN messages for debugging
              Sensor.debug=!Sensor.debug;
            break;
            
          case 'D':     //Causes ISA object to print derived values for debugging
                 Sensor.debug2=!Sensor.debug2;
      		  break;
          
          case 'i':     
              Sensor.initialize();
      		  break;
           
          case '?':     //Print a menu describing these functions
              printMenu();
      		  break;
            
          case '1':     
              Sensor.STOP();
            break; 
        
         case '3':     
             Sensor.START();
            break;
         
	        }    
      }
}


void getRate()
{
  Serial<<"\n Enter the Data Rate in Kbps you want for CAN : ";
    while(Serial.available() == 0){}               
    float V = Serial.parseFloat();  
    if(V>0)
      {
       Serial<<"Datarate:"<<V<<"\n\n";
       uint8_t rate=V;
       
       datarate=V*1000;
       
       Sensor.begin(port,datarate);
      }
}


void getPort()
{
  Serial<<"\n Enter port selection:  c0=CAN0 c1=CAN1 ";
  while(Serial.available() == 0){}               
  int P = Serial.parseInt();  
   if(P>1) Serial<<"Entry out of range, enter 0 or 1 \n";
  else 
     {
      port=P;
     Sensor.begin(port,datarate); 
       }           
}
