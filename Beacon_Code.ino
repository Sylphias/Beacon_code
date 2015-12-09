#include <XBee.h>
#include <SoftwareSerial.h>
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

TMRpcm tmrpcm; 
char mychar;
File root;
File entry;

const int this_beacon = 2;
const int chipSelect = 10;    
const int oldCard = SPI_HALF_SPEED;
const int newCard = SPI_QUARTER_SPEED;
int cardType = oldCard;
unsigned long timeDiff = 0;
unsigned long timePress = 0;
int  old_sound = 0;
int sound_delta;
int is_triggered = false;
int count = 0;
int maxnum = 3;



//Xbee SHits
SoftwareSerial serial1(0, 1); // RX, TX


// Initialize Global Variables
XBee xbee=XBee();
XBeeResponse response = XBeeResponse();
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();
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
  pinMode(8,INPUT);
  pinMode(chipSelect, OUTPUT); 
  digitalWrite(chipSelect, HIGH); // Add this line

  tmrpcm.speakerPin = 9;
  if (!SD.begin(chipSelect,cardType)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  root = SD.open("/");
//  delay(2000);
  //tmrpcm.play("8-16-kg.wav"); //the sound file "music" will play each time the arduino power
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
        Serial.println(message_type);
        packet_sec[3].toCharArray(beacon_chain, packet_sec[3].length());  
        // Clear out string and counters to get ready for the next incoming string
        counter = 0;
        lastIndex = 0;

        // Handling various types of messages 
        // Check if you re-received the previous message
        if(old_message_id != message_id && original_beacon_ID != beacon_chain[0]){
          switch(message_type){
            case 1:
             beacon_mode = 1;
             Serial.print('playing');
             tmrpcm.stopPlayback();
             tmrpcm.play("properjapend.wav"); 
             format_message_payload(2,hop_number,message_id, beacon_chain);
            break;
            case 2:
             format_message_payload(2,hop_number,message_id, beacon_chain);
            break;
            case 3:
             tmrpcm.stopPlayback();
             tmrpcm.play("8-16-iws.wav");
            break;
            default:
              tmrpcm.stopPlayback();
            break;
          }
        }
      } 
    }
  }
  if(digitalRead(8) == HIGH)
  {
    is_triggered = true;
  }
  if(beacon_mode == 1 && tmrpcm.isPlaying() == false){
    if(is_triggered)
    {
      Serial.print("Test");
      tmrpcm.stopPlayback();
      tmrpcm.play("proper1end.wav"); 
      format_message_payload(2,0,message_id, beacon_chain);
      delay(200);
      beacon_mode=0;
      is_triggered=false;
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
  beacon_chain += this_beacon;
  String composed_message = String(message_type) +','+ String(hop_number) +','+ String(message_id) +','+ beacon_chain;
  char payload[32];
  composed_message.toCharArray(payload, composed_message.length()+1);
  Serial.println(payload);
  uint8_t payload_bytes[32] = {};
  memcpy((uint8_t*)payload_bytes,(char*)payload, 32);
  Tx16Request tx = Tx16Request(0x0000, payload_bytes, sizeof(payload_bytes));
  xbee.send(tx);

}

void microphone_loudness()
{ 
  analogRead(0);
  delay(10);
  int val = analogRead(0);
  sound_delta = val - old_sound;
  Serial.println(sound_delta);
  if(sound_delta > 550){
    is_triggered = true;
  }
  old_sound = val;
}

















