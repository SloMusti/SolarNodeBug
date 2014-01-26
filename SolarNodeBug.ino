/*
SolarNodeBug.cpp - Low power wireless communication based on MSP430G2452
NFR24L01 and energia.nu environment. 

This code implemebnts basic packet communication via a serial connection.

Copyright (c) 2014 Luka Mustafa - Musti (musti [at] wlan-si.net). All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//Radio libraries: https://github.com/spirilis/Enrf24 
#include <Enrf24.h> 
#include <nRF24L01.h>
//Energia libraries
#include <SPI.h>
//Custom includes
#include "packet.h"

//Define pins
#define BAT_V P1_0 //A0-there is a bug, must do analog read directly with A?
#define SOLAR_V P1_3//A3
#define STATUS_LED P1_1 //uart tx on the ping as well, user rst also
#define MAIN_LED P1_2
#define IRQ P1_4
#define CE P2_7
#define CSN P2_6

//Create raio objects and addresses
Enrf24 radio(CE, CSN, IRQ);  // CE, CSN, IRQ pins
const uint8_t rxaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01 };//broadcast for now
const uint8_t txaddr[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01 };//broadcast for now

packet_t packet_tx;
packet_t packet_rx;

//optionally add initial values here

void setup()
{
  Serial.begin(9600); // pinMode must be after Serial, because we are overriding RX
  
  //analog inputs via potential dividers, est is to measre and calibrate
  // roughly multiply measured voltage with 2.6
  // 1024bits 1.5V range
  // multiply the measured number of bits with roughly 3.81 for the result in mV
  pinMode(BAT_V, INPUT);
  digitalWrite(BAT_V, LOW); // making sure the pull/up is off
  pinMode(SOLAR_V, INPUT);
  digitalWrite(SOLAR_V, LOW); // making sure the pull/up is off 
  analogReference(INTERNAL1V5); // 1.5V reference set
  analogRead(TEMPSENSOR); // first reading usually wrong
  
  pinMode(MAIN_LED, OUTPUT); 
  //pinMode(STATUS_LED, OUTPUT); // if serial is enabled, status led should not be used
  
  // SPI setup
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  
  // Radio setup
  radio.begin();  // Defaults 1Mbps, channel 0, max TX power
  radio.setRXaddress((void*)rxaddr);
  radio.setTXaddress((void*)txaddr);
  
  radio.enableRX();
  
  // Power on self-check
  if(radio.radioState()==ENRF24_STATE_PRX){
    Serial.println("Radio OK");// This will as well blink the status led 
  }
  
  digitalWrite(MAIN_LED, HIGH);
  delay(1000);
  digitalWrite(MAIN_LED, LOW);

}

void loop()
{
   
  receive_package(); //call to receive a packet - packet_rx structure will be updated
  digitalWrite(MAIN_LED, packet_rx.data.led); //controlling the LED
  
  //for a show, toggling the led and incrementing destination
  packet_tx.data.led=~packet_tx.data.led;
  packet_tx.data.identifier=0x01;
  measure_temeparture();
  transmit_package();
  
  
  delay(1000);

}

void dump_packet(packet_t* packet){

  Serial.println("Packet dump: ");
  Serial.print("identifier: ");
  Serial.println(packet->data.identifier);
  Serial.print("destination: ");
  Serial.println(packet->data.destination);
  Serial.print("control: ");
  Serial.println(packet->data.control);
  Serial.print("battery_voltage: ");
  Serial.println(packet->data.battery_voltage);
  Serial.print("solar_voltage: ");
  Serial.println(packet->data.solar_voltage);
  Serial.print("supply_voltage: ");
  Serial.println(packet->data.supply_voltage);
  Serial.print("temperature: ");
  Serial.println(packet->data.temperature);
  Serial.print("led: ");
  Serial.println(packet->data.led);
}


void receive_package(){
  // RX sections
  if(radio.available(true)){
    if (radio.read(&packet_rx)) {
      /*
      //DEBUG:
      Serial.print("Received packet: ");
      for(char i=0;i<sizeof(struct_packet_t);i++){
        Serial.print(packet_rx.byte_packet[i],HEX);
      }
      Serial.println("");
      //end debug */
      dump_packet(&packet_rx);
    }
  }
}

void transmit_package(){
  /*
  //DEBUG
  Serial.print("TX packet: ");
  for(char i=0;i<sizeof(struct_packet_t);i++){
    Serial.print(packet_tx.byte_packet[i],HEX);
  }
  Serial.println("");
   //dump_radio_status_to_serialport(radio.radioState());  // Should show Receive Mode
  //end DEBUG */
  
  //Serial.print("Sending packet: ");
  //Serial.println(str_on);
  
  //create a pointer to the struct and increment it, alternatively passing chars directly could work as well
  
  byte *ptr = (byte *) &packet_tx;
  for(char i=0;i<sizeof(packet_tx);i++){
    radio.write(*ptr++);
  }
  radio.flush();  // Force transmit (don't wait for any more data)
  //dump_radio_status_to_serialport(radio.radioState());  // Should report receive mode
}

void form_packet(){
  int battery_voltage=analogRead(A0);
  int solar_voltage=analogRead(A3);
  
  Serial.println(battery_voltage);
  Serial.println(solar_voltage);
 
  delay(1000);
  
  Serial.println("");
}

void measure_temeparture(){
  packet_tx.data.temperature=((uint32_t)analogRead(TEMPSENSOR)*27069 - 18169625) *10 >> 16;
}



// DEBUGGING CODE ONLY! TO BE REMOVED IN PRODUCTION
/*
void dump_radio_status_to_serialport(uint8_t status)
{
  Serial.print("Enrf24 radio transceiver status: ");
  switch (status) {
    case ENRF24_STATE_NOTPRESENT:
      Serial.println("NO TRANSCEIVER PRESENT");
      break;

    case ENRF24_STATE_DEEPSLEEP:
      Serial.println("DEEP SLEEP <1uA power consumption");
      break;

    case ENRF24_STATE_IDLE:
      Serial.println("IDLE module powered up w/ oscillators running");
      break;

    case ENRF24_STATE_PTX:
      Serial.println("Actively Transmitting");
      break;

    case ENRF24_STATE_PRX:
      Serial.println("Receive Mode");
      break;

    default:
      Serial.println("UNKNOWN STATUS CODE");
  }
}//*/
