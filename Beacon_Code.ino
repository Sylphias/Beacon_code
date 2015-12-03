#include <XBee.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>
#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h> 
// XBee's DOUT (TX) is connected to pin 1 (Arduino's Software RX)
// XBee's DIN (RX) is connected to pin 9 (Arduino's Software TX)
/*
  This file is for the beacon algorithm and how it handles packets coming in and out of the system


  Establishing Message Structure
  Edit This code to change what is sent in payload
  Message Payload can only contain 256 Bytes, First few of which are reserved for key information
  M) Message Type (1,2,3) 1 - broadcast 2 - play help 3 - redirect but dont play
  H) Hop Number (In hexadecimal)
  I) Message ID 
  R) Receiver Chains (Ie addresses of the chips the message has passed through)
  E) End Packet
  Look at CRC Encoding
  

*/

//rgb_lcd lcd;

//DIVISION BY MULTIPLICATION ARRAY
/*Note:
    1. This array only works for long mathematics because of the range
    2. Multiplication table used for 32khz sampling rate
    3. Multiply summation of window by the appropriate index correlated number below
    4. then bit shift 11 bits to the right
*/

const int division[] PROGMEM = {
  4096,2048,1024,683,512,410,341,293,256,228,205,186, // 0 - 11
  171,158,146,137,128,120,114,108,102,98,93,89,85,82, //12 - 25
  79,76,73,71,68,66,64,62,60,59,57,55,54,53,51,50,49, //26 - 42
  48,47,46,45,44,43,42,41,40,39,39,38,37,37,36,35,35, //43 - 59
  34,34,33,33,32,32,31,31,30,30,29,29,28,28,28,27,27, //60 - 76
  27,26,26,26,25,25,25,24,24,24,24,23,23,23,23,22,22, //77 - 93
  22,22,21,21,21,21,20,20,20,20,20,20,19,19,19,19,19, //94 - 110
  18,18,18,18,18,18,18,17,17,17,17,17,17,17,16,16,16, //111 - 127
  16,16,16,16,16,15,15,15,15,15,15,15,15,15,14,14,14, //128 - 144
  14,14,14,14,14,14,14,13,13,13,13,13,13,13,13,13,13, //145 - 161
  13,13,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12, //162 - 178
  11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11, //179 - 195
  10,10,10,10,10,10,10,10,10,10,10,10,10,10,10        //196 - 210
};

//Pin Mapping
#define frequencyIn   A0
#define lcdD7         A1
#define lcdD6         A2
#define lcdD5         A3 
#define lcdD4          2
#define lcdRS          7
#define lcdEnable      4
#define lcdBackLt      1


TMRpcm tmrpcm; 
char mychar;
File root;
File entry;


const int chipSelect = 10;    
const int oldCard = SPI_HALF_SPEED;
const int newCard = SPI_QUARTER_SPEED;
int cardType = oldCard;
unsigned long timeDiff = 0;
unsigned long timePress = 0;

int count = 0;
int maxnum = 3;


//**************************************************************************************************
//----------------------------------->  USER DEFINED CONSTANTS  <-----------------------------------
//**************************************************************************************************
const int delay_index_low_limit = 11;    //min delay constant, which equates to ~3136hz
const int delay_index_high_limit = 102;  //max delay constant, which equates to ~196hz
const int sample_upper_limit = 210;      //must be twice the value of delay_index_high_limit
const byte loudness_limit = 10;           //minimum level of reliable loudness output (range 0-64)
const byte min_amdf_limit = 4;           //minimum amdf for a positive hit
const byte harmonic_offset = 4;          //offset for other harmonics of amdf if aliasing occurs
const byte adc_offset = 126;             //zero offset for adc


//Autocorrelation Mean Difference Function variables
int sample_number = 0;                   //current number of samples in the window available for processing
byte delay_index = 0;                    //pointer for calculating the amdf when window is big enough
char sample_array[sample_upper_limit];   //byte array for holding ADC values of the window
boolean process_window = true;           //allow MCU to process window when data is available
word amdf_calc[delay_index_high_limit];  //set array to size of delay_index_high_limit
byte min_amdf_index = 0;                 //store the value of the AMDF until next processing
unsigned long time1 = 0;                 //timer for displaying information on screen
byte loudness = 15;                      //current loudness level for allowing amdf output
byte loudness_timer = 0;                 //time to update loudness level
char max_loudness = 0;                   //max peak of a sample
byte calculation_time_start = 0;         //hold the time for when the first ADC retrieval
byte calculation_time = 0;               //hold the calculation time after AMDF algorithm for printing
boolean calculate_time = true;           //let calculation start time only update on sample 0
byte last_commit = 0;                    //last amdf value before level dropped below limit
boolean harmonic_detect = false;         //if harmonic is present
byte clip_level = 0;                     //floor any data that isn't above clip level

//Xbee SHits
SoftwareSerial serial1(0, 1); // RX, TX


// Initialize Global Variables
XBee xbee=XBee();
XBeeResponse response = XBeeResponse();
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();
const int beacon_id = 1 ;
uint8_t option = 0;
uint8_t data = 0;
uint8_t rssi = 0;

int old_message_id = 0;
int original_beacon_ID = 0;

//Beacon mode tells us if we're listening for microphone(1), or broadcasting back(2)
int beacon_mode = 0;

void setup() 
{
  Serial.begin(9600);
  serial1.begin(9600);
  xbee.setSerial(Serial);
  pinMode(13,INPUT);
  pinMode(12,OUTPUT);
  //SETUP THE ADC
  cli();//diable interrupts
  //set up continuous sampling of analog pin 0 at 19.2kHz
  //clear ADCSRA, ADCSRB, ADMUX registers
  //do not make calls for analog write or analog read in loop unless you reset these parameters
  ADCSRA = 0;
  ADCSRB = 0;
  ADMUX = 0;
  ADMUX |= (1 << REFS0); // set reference voltage (AVcc with external capacitor at aref pin)
  ADMUX |= (1 << ADLAR); // left align for data collection
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1); // set ADC clock with 32 prescaler- 16mHz/64=250kHz
                                         // 250khz / 13 instruction cycles = 19.2khz
                                         // nyquist frequency is 9615 hz
                                         // remember that all higher frequencies above 9.6khz will aliased
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE);  //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN);  //enable ADC
  ADCSRA |= (1 << ADSC);  //start ADC measurements
  sei();//re-enable interrupts
  
  
  Serial.print("\nInitializing SD card...");
  pinMode(chipSelect, OUTPUT); 
  digitalWrite(chipSelect, HIGH); // Add this line


  tmrpcm.speakerPin = 9;
  
  root = SD.open("/");
  delay(2000);;
  tmrpcm.play("8-16-kg.wav"); //the sound file "music" will play each time the arduino power
}



 
void loop() 
{
  // Initialize Local Variables
  int message_type = 0;
  int hop_number = 0;
  int message_id = 0;
  char beacon_chain[30];
  int lastIndex = 0;


  xbee.readPacket(100);
  if (xbee.getResponse().isAvailable())
  {
    Serial.println("Getting Signal Strength: ");
    if(xbee.getResponse().getApiId() == RX_64_RESPONSE || xbee.getResponse().getApiId() == RX_16_RESPONSE)
    { 
      if (xbee.getResponse().getApiId() == RX_16_RESPONSE) 
      {
        xbee.getResponse().getRx16Response(rx16);
        rssi = rx16.getRssi();
        Serial.println(rssi);
        String packet_sec[4];
        char packet_char[rx16.getDataLength()];
        String packet_input;
        int counter = 0;
        // This loop converts the data in to a string for processing
        for(int packet_counter = 0 ; packet_counter < rx16.getDataLength(); packet_counter++)
        {
          packet_char[packet_counter] = (char)rx16.getData(packet_counter);
        };

        // This method splits up the packet data based off a comma delimiter, not optimised but time constraints....
        packet_input = packet_char;

        for (int i = 0; i < packet_input.length(); i++) {
              // Loop through each character and check if it's a comma
              if (packet_input.substring(i, i+1) == ",") {
                // Grab the piece from the last index up to the current position and store it
                packet_sec[counter] = packet_input.substring(lastIndex, i);
                // Update the last position and add 1, so it starts from the next character
                lastIndex = i + 1;
                // Increase the position in the array that we store into
                counter++;
              }
              // If we're at the end of the string (no more commas to stop us)
              if (i == packet_input.length() - 1) {
                // Grab the last part of the string from the lastIndex to the end
                packet_sec[counter] = packet_input.substring(lastIndex, i);
              }
            }
        message_type = packet_sec[0].toInt();
        hop_number = packet_sec[1].toInt();
        message_id = packet_sec[2].toInt();
        packet_sec[3].toCharArray(beacon_chain, packet_sec[3].length());  
        Serial.println(message_type);
        Serial.println(hop_number);
        Serial.println(message_id);
        Serial.println(beacon_chain[1]);

        // Clear out string and counters to get ready for the next incoming string
        counter = 0;
        lastIndex = 0;

        // Handling various types of messages 
        // Check if you re-received the previous message
        if(old_message_id != message_id && original_beacon_ID != beacon_chain[1]){
          switch(message_type){
            case 1:
              Serial.println('Test');
              beacon_mode = 1;
              if(digitalRead(13) == HIGH && beacon_mode == 1)
                {
                  run_voice();
                  format_message_payload(2,0,message_id, beacon_chain);
                }
                else
                {
                  digitalWrite(12,LOW);
              }
            break;
            case 2:
              //todo
            break;
            case 3:
              //todo
            break;
            default:
              //todo
            break;
          }
        }
      } 
      else 
      {
        Serial.println("64");
        xbee.getResponse().getRx64Response(rx64);
        rssi = rx64.getRssi();
        Serial.println(rssi);
      }
    }
  }
}


void run_voice()
{
  //process the window when samples are available
                    run_amdf();
                  
                    //return amdf index
                    amdf_index();
                    
                    //update display every 100ms
                    unsigned long time2 = millis();
                    if( time2 - time1 > 250){
                      time1 = time2;
//                      lcd.clear();
//                      lcd.print(calculation_time);  //top left
//                      lcd.setCursor(5,0);
//                      lcd.print(int(loudness)); //top middle
//                      lcd.setCursor(10,0);
//                      lcd.print(int(sample_array[0])); //a sample at top right
//                      lcd.setCursor(0,1);
                      float print_this = 19230 / float(last_commit);
//                      lcd.print(print_this); //frequency on bottom left
                      if(harmonic_detect == true){
                        harmonic_detect = false;
//                        lcd.setCursor(8,1);
//                        lcd.print(" H");
                        count++;
                       if(count < maxnum){
                        tmrpcm.play("8-16-hello.wav");
                       }
                       else if(count > maxnum){
                        tmrpcm.play("8-16-iws.wav");
                       }
                      }
                    }
}

//Prepares the outgoing message to be sent
void format_message_payload(int message_type, int hop_number, int message_id,String beacon_chain)
{
  String padded_hop_number;
  hop_number++;
  if(hop_number < 10)
  {
    padded_hop_number = "0"+hop_number;
  } 
  beacon_chain += beacon_id;
  String composed_message = String(message_type) + String(hop_number) + String(message_id) + beacon_chain;
  char payload[32];
  composed_message.toCharArray(payload, composed_message.length()+1);
  Serial.println(payload);
  uint8_t payload_bytes[32] = {};
  memcpy((uint8_t*)payload_bytes,(char*)payload, 32);
  Tx16Request tx = Tx16Request(0x0000, payload_bytes, sizeof(payload_bytes));
  xbee.send(tx);

}


//**************************************************************************************************
////////////////////////////////////   ADC INTERRUPT ROUTINE   /////////////////////////////////////
//**************************************************************************************************
ISR(ADC_vect){

  //retreive adc data and form into signed byte with 7 bit precision
  char incoming_char = ADCH - adc_offset;

  //clipping operation
  if(incoming_char > 0){
    if(incoming_char < clip_level){
      incoming_char = 0;
    }
  }else{
    if(incoming_char > -clip_level){
      incoming_char = 0;  
    }  
  }
  
  //Stop updating window if mcu needs to finish AMDF
  if(sample_number < sample_upper_limit){

    //if first sample, record time for algorithm speed
    if(sample_number == 0){
      calculation_time_start = millis();
    }
    
    //update window
    sample_array[sample_number] = incoming_char;
    sample_number++;

    //determine loudness of signal
    if(incoming_char < 0){
      incoming_char = -incoming_char;
    };
    if (incoming_char > loudness){
      loudness = incoming_char;
      //reset of loudness variable occurs in amdf output completion
    }
  }//STOP Updating window

}//end adc interrupt routine


//**************************************************************************************************
////////////////////////////////////   AMDF WINDOW CALCULATION  ////////////////////////////////////
//**************************************************************************************************
void run_amdf(){
  
  //PROCESS WINDOW WHEN ENOUGH SAMPLES AVAILABLE TO DO SO
  if( ((delay_index << 1) < sample_number) && process_window == true){
    
    // AMDF ALGORITHM
    // AMDF(delay) = 1/period * Sum(n=0 -> period) of (abs(x(n) - x(n+delay)))

      //sum the differences of the sample window of a specific delay
      amdf_calc[delay_index] = 0;
      for(byte i = 0; i < delay_index; i++){
        //calculate difference
          char calc_difference = sample_array[i] - sample_array[delay_index+i];
        //get absolute value
          if(calc_difference < 0){
            calc_difference = -calc_difference;
          }
        //add the value to the sum
          amdf_calc[delay_index] += calc_difference; 
      }
      
    //divide the summation by using multiplication table and bit shift
    unsigned long calculate_division = amdf_calc[delay_index] * pgm_read_word(&division[delay_index]);
    calculate_division >>= 11;
    amdf_calc[delay_index] = word(calculate_division);

    //increment the delay index for next delay calculation
    delay_index++;

    //allow window to refresh and stop AMDF on current window until frequency determined
    if(delay_index > delay_index_high_limit){
      process_window = false;                               //stop AMDF calculation
      delay_index = delay_index_low_limit;                  //reset AMDF minimum delay calc point
      sample_number = 0;                                    //let the ADC start getting new data
      calculation_time = millis() - calculation_time_start; //calculate the time to finish the window
      clip_level = loudness >> 1;                           //establish clip level from previous loudness
      loudness = 0;                                         //reset loudness
    }
  }//end PROCESS WINDOW
  
}//end amdf window calculation


//**************************************************************************************************
/////////////////////////////////////////   AMDF RETURN   //////////////////////////////////////////
//**************************************************************************************************
void amdf_index(){
   
  //determine the best delay
  if(process_window == false){
    
    word min_amdf = 100;
    
    //cycle through minimum to maximum delays (highest frequency to lowest)
    for(byte i = delay_index_low_limit; i <= delay_index_high_limit; i++){
      
      //check for lowest value out of all delays
      if(amdf_calc[i] < min_amdf){
        min_amdf = amdf_calc[i];
        min_amdf_index = i;
      }
    }

    //check harmonics
    check_harmonics(min_amdf_index);

    if(clip_level > loudness_limit){
      last_commit = min_amdf_index;
    }
    
    //reset to reprocess new window
    process_window = true;
  }
  
}//end AMDF RETURN


//**************************************************************************************************
////////////////////////////////   CHECKING FOR HARMONICS IN AMDF  /////////////////////////////////
//**************************************************************************************************
void check_harmonics(byte returned_delay){

  //check 2nd harmonic in case of aliasing
  byte temp_index = returned_delay >> 1;
  if(temp_index >= delay_index_low_limit){
    if(amdf_calc[temp_index] < amdf_calc[returned_delay] + harmonic_offset){
      min_amdf_index = temp_index;
      harmonic_detect = true;
    }    
  }
  
  //check 3rd harmonic in case of aliasing
  byte temp_index1 = returned_delay / 3;
  if(temp_index1 >= delay_index_low_limit){
    if(amdf_calc[temp_index1] < amdf_calc[returned_delay] + harmonic_offset){
      min_amdf_index = temp_index1;
      harmonic_detect = true;
    }    
  }
  
  //check 4th harmonic in case of aliasing
  byte temp_index2 = returned_delay >> 2;
  if(temp_index2 >= delay_index_low_limit){
    if(amdf_calc[temp_index2] < amdf_calc[returned_delay] + harmonic_offset){
      min_amdf_index = temp_index2;
      harmonic_detect = true;
    }    
  }
  
  //check 5th harmonic in case of aliasing
  byte temp_index3 = returned_delay / 5;
  if(temp_index3 >= delay_index_low_limit){
    if(amdf_calc[temp_index3] < amdf_calc[returned_delay] + harmonic_offset){
      min_amdf_index = temp_index3;
      harmonic_detect = true;
    }    
  }

}//end checking harmonics













