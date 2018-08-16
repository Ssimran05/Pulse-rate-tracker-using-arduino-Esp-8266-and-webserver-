//Program to know the heartrate of a person and transfer the data to a global server i.e things speak and reading the BPM in the lcd(16x30
//defin all the libraries
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
int contrast=50;  

SoftwareSerial ser(9,10);  // RX to pin 9 of arduino, TX to pin 10 of arduino
LiquidCrystal lcd(12,11,5,4,3,2);  //define pins of lcd
int sensor_pin = 0;                //define sensor pin    
String apiKey = "EQCKIYCC602CT976"; //apikey of things speak 
                
//define some of the variables to convert the heartrate to BPM(Blood pressure) 
int led_pin = 13;                  

volatile int heart_rate;          

volatile int analog_data;              

volatile int time_between_beats = 600;            

volatile boolean pulse_signal = false;    

volatile int beat[10];         //heartbeat values will be sotred in this array    

volatile int peak_value = 512;          

volatile int trough_value = 512;        

volatile int thresh = 525;              

volatile int amplitude = 100;                 

volatile boolean first_heartpulse = true;      

volatile boolean second_heartpulse = false;    

volatile unsigned long samplecounter = 0;   //This counter will tell us the pulse timing

volatile unsigned long lastBeatTime = 0;



void setup()
{
  pinMode(led_pin,OUTPUT); //set led pin as output        
  ser.begin(9600);         //to start esp communication
  ser.println("AT");       //connect the esp module using "AT" comments
  delay(1000);
  ser.println("AT+CWMODE=1"); //set the esp as hotspot
  delay(1000);
  ser.println("AT+CWJAP=\"simran\",\"nononono\"");  //hotspot & password name
  delay(1000);
  ser.println("AT+CIPMUX=0");   
  delay(1000);           

  interruptSetup();  
  analogWrite(6,contrast);  
  lcd.begin(16,2);          //start lcd working      

}



void loop()
{ 
 lcd.setCursor(0,0);
 lcd.print("BPM: ");
 int a =heart_rate;
 lcd.print(a);
 String cmd = "AT+CIPSTART=\"TCP\",\"";
 cmd += "184.106.153.149"; //ip of things speak
 cmd += "\",80";
 ser.println(cmd);
   
  if(ser.find("Error"))
  {
    return;
  }
  
  // prepare GET string
  String getStr = "GET /update?api_key=";
  getStr += apiKey;
  getStr +="&field1=";
  getStr += String(a);
  getStr += "\r\n\r\n";

  // send data length
  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  ser.println(cmd);

  if(ser.find(">"))
  {
    ser.print(getStr);
  }
 
    
  // thingspeak needs 15 sec delay between updates
  for(int i=0;i<=15;i++)
  {
  delay(1000);    //take a break
  }
 // Serial.print('\n');
 // Serial.println("Data Sent");
      

      delay(200); //  take a break
      lcd.setCursor(0,1);
      lcd.print("By Simram Gupta ");

}



void interruptSetup()

{    

  TCCR2A = 0x02;  // This will disable the PWM on pin 3 and 11

  OCR2A = 0X7C;   // This will set the top of count to 124 for the 500Hz sample rate

  TCCR2B = 0x06;  // DON'T FORCE COMPARE, 256 PRESCALER

  TIMSK2 = 0x02;  // This will enable interrupt on match between OCR2A and Timer

  sei();          // This will make sure that the global interrupts are enable

}


ISR(TIMER2_COMPA_vect)

{ 

  cli();                                     

  analog_data = analogRead(sensor_pin);            

  samplecounter += 2;                        

  int N = samplecounter - lastBeatTime;      


  if(analog_data < thresh && N > (time_between_beats/5)*3)

    {     

      if (analog_data < trough_value)

      {                       

        trough_value = analog_data;

      }

    }


  if(analog_data > thresh && analog_data > peak_value)

    {        

      peak_value = analog_data;

    }                          



   if (N > 250)

  {                            

    if ( (analog_data > thresh) && (pulse_signal == false) && (N > (time_between_beats/5)*3) )

      {       

        pulse_signal = true;          

        digitalWrite(led_pin,HIGH);

        time_between_beats = samplecounter - lastBeatTime;

        lastBeatTime = samplecounter;     



       if(second_heartpulse)

        {                        

          second_heartpulse = false;   

          for(int i=0; i<=9; i++)    

          {            

            beat[i] = time_between_beats; //Filling the array with the heart beat values                    

          }

        }


        if(first_heartpulse)

        {                        

          first_heartpulse = false;

          second_heartpulse = true;

          sei();            

          return;           

        }  


      word runningTotal = 0;  


      for(int i=0; i<=8; i++)

        {               

          beat[i] = beat[i+1];

          runningTotal += beat[i];

        }


      beat[9] = time_between_beats;             

      runningTotal += beat[9];   

      runningTotal /= 10;        

      heart_rate = 60000/runningTotal;

    }                      

  }




  if (analog_data < thresh && pulse_signal == true)

    {  

      digitalWrite(led_pin,LOW); 

      pulse_signal = false;             

      amplitude = peak_value - trough_value;

      thresh = amplitude/2 + trough_value; 

      peak_value = thresh;           

      trough_value = thresh;

    }


  if (N > 2500)

    {                          

      thresh = 512;                     

      peak_value = 512;                 

      trough_value = 512;               

      lastBeatTime = samplecounter;     

      first_heartpulse = true;                 

      second_heartpulse = false;               

    }


  sei();                                

}


