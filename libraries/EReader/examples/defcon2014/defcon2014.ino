// Copyright 2013 WyoLum, LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.  See the License for the specific language
// governing permissions and limitations under the License.

#include <inttypes.h>
#include <avr/interrupt.h>
#include <ctype.h>
#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/power.h> //Needed for powering down perihperals such as the ADC/TWI and Timers

#include <SPI.h>
#include <SD.h>
#include "EPD.h"
#include "EReader.h"

// globals

uint16_t UNICODE_MSG[11] = {25105, 20204, 29233, 30717, 36882, 31185, 25216, 0};
const uint8_t CMD_BUF_LEN = 10;
const uint8_t cmd_pending = 0;

char cmd_buffer[CMD_BUF_LEN];

int current_page = 0;
char nickname[25];
String inputString = "";
char inputCommand = '0';
boolean stringComplete = false;
File txtFile;


// these are involved in determining when we go to sleep
// after a certain number of milliseconds with no button presses go to sleep.
// if a button is pressed, it resets the timer.
long lastWakeTime; //reset with every interaction
#define AWAKETIME 80000 // how long to stay awake
#define FOCUSTIME 12250 // how long keep optimal responsiveness

#define MAX_PAGES 1

/*
  increment page number by one
 */
void next_page(){
  current_page++;
  if (current_page > MAX_PAGES) {
    current_page = 0;
  }
}

/*
  decrement page number by one
 */
void prev_page(){
  current_page--;
  if (current_page < 0){
    // max pages -1 at this point max is 2
    current_page = MAX_PAGES;
  }
}

void setup() {
  //Serial.begin(9600);
  //Serial.println("Serial Setup Complete");
  ereader.setup(EPD_2_7); // starts SD
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(SEL_PIN, INPUT);
  pinMode(MODE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  bool validStr;
  txtFile = SD.open("nickname.txt");
  
  if (txtFile.available())
  {
    int i = 0;
    char c;
    validStr = false;
    while ((i < 25) && (txtFile.available()))
    {
      if (validStr == true)
      {
        break;
      }
      c = txtFile.read();
      switch(c)
      {
        case ';':
          validStr = true;
          break;
        case '\n':
          break;
        case '\r':
          break;
        default:
          nickname[i++] = c;
          break;
      }
    }
  }
  
  txtFile.close();
  
  display();
  
  for(int ii=0; ii < 3; ii++){
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
  // ereader.spi_detach();
}


void display(){
  digitalWrite(LED_PIN, LOW);
  ereader.spi_attach();
  ereader.EPD.begin();
  int offset=35; int height=16;

  //current_page = 0;
  switch (current_page) {
    case 0:
      ereader.display_wif("/IMAGES/SHM_EYES.WIF", 0, 0);
      //ereader.display_wif("/IMAGES/NOVA.WIF", 0, 0);
      ereader.put_bigascii(10, 35, "Richard", true);
      ereader.put_bigascii(10, 55, "Harman", true);
      //ereader.put_ascii(10, height*3+offset, "NoVA HA Member", true);
      ereader.put_bigascii(10, height*4+offset, "LABS 2015", true);
      ereader.put_bigascii(10, 150, nickname, true);

      break;
    case 1:
      ereader.display_wif("/PAGES/PAGE02.WIF",10, 0);
      break;
  }
  ereader.show();
  ereader.EPD.end();

  uint16_t start = millis();
  //ereader.spi_detach(); // this call takes .8 seconds to execute!
  if(true){
    for(int ii=0; ii<4; ii++){
      digitalWrite(LED_PIN, ii % 2 == 0 );
      if(ii < 3){
	delay(25);
      }
    }
  }
}

// main loop
void loop() {
  
  bool update_needed = false;
  int pulse = millis() % (2000 * (2 - ereader.attached));
  if(pulse  < 50){
    digitalWrite(LED_PIN, HIGH);
  }
  else if(150 < pulse && pulse < 250){
    digitalWrite(LED_PIN, HIGH);
  }
  else{
    digitalWrite(LED_PIN, LOW);
  }
  long current = millis();
  if ((current - lastWakeTime) > FOCUSTIME && ereader.attached){
    digitalWrite(LED_PIN, LOW);
    ereader.spi_detach(); // this call takes .8 seconds to execute!
    //Serial.println("ereader detached");
  }
  else if ((current - lastWakeTime) > AWAKETIME){
     //Serial.println("should sleep");
    goToSleep();
  }

  if (analogRead(MODE_PIN) > 512){
    prev_page();
    update_needed = true;
  }
  if(digitalRead(SEL_PIN)){
    next_page();
    update_needed = true;
  }
  if(digitalRead(UP_PIN)){
    current_page = 0;
    update_needed = true;
  }
  if(update_needed){
    lastWakeTime = current;
    display();
  }
  if(stringComplete) {
	switch(inputCommand) {
 		case 'A':
			inputString.toCharArray(nickname, inputString.length()+1);
			for(int ii=0; ii < 5; ii++){
				digitalWrite(LED_PIN, HIGH);
				delay(100);
				digitalWrite(LED_PIN, LOW);
				delay(100);
			}
    			display();
 			break;
 		case 'N':
			next_page();
    			display();
 			break;
 		case 'P':
			prev_page();
    			display();
 			break;
 		//case 'S':
 			//Serial.print("Schedule: ");
 			//Serial.println(inputString);
 			//break;
 		default:
 			//Serial.print("Unknown Command: ");
 			//Serial.println(inputString);
 			break;
       	}

    stringComplete = false;
    inputCommand = '0';
    inputString = "";
  }
 
}

void goToSleep(){
  digitalWrite(LED_PIN, LOW);
//  ereader.spi_detach();
  // ereader.EPD.end();   // make sure EPD panel is off
  delay(500);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(0,wake,RISING); // pin 2 intterupt UP button on V3 
  attachInterrupt(1,wake,RISING);// pin 3 interrupt  SEL button on V3

 #ifndef COMPLICATED
 //Shut off ADC, TWI, SPI, Timer0, Timer1
 // ADCSRA &= ~(1<<ADEN); //Disable ADC
 // ACSR = (1<<ACD); //Disable the analog comparator
 // DIDR0 = 0x3F; //Disable digital input buffers on all ADC0-ADC5 pins
 // DIDR1 = (1<<AIN1D)|(1<<AIN0D); //Disable digital input buffer on AIN1/0
  
  power_twi_disable();
//  power_spi_disable();
  power_usart0_disable();
//  power_timer0_disable(); //Needed for delay_ms and apparently for pin interrupts
  power_timer1_disable();
  power_timer2_disable(); 
 #endif
  sleep_mode(); // this immediately goes to sleep
  // and when we wake up, we will execute this
  detachInterrupt(0);// we don't want to keep getting interrupted when waking
  detachInterrupt(1);

  sleep_disable();
  power_twi_enable();
//  power_spi_enable();
  power_usart0_enable();
//  power_timer0_enable(); //Needed for delay_ms
  power_timer1_enable();
  power_timer2_enable(); 

  //Serial.println("returning from sleep");
  lastWakeTime = millis();
}


void wake()
{
  // apparently the uart takes a while to reconfigure on waking up
  //Serial.begin(9600);
  delay(100); // crude debounce
}

/*
void serialEvent() {
        while(Serial.available()) {
                char inChar = (char)Serial.read();

		//If the inputString is blank the first character is the command
		if( (inputCommand == '0') and (inputString == "") ) {
          		inputCommand = inChar;
                } else if(inChar == '\n') {
                        stringComplete = true;
                } else {
                        inputString += inChar;
                }
        }
}
*/
  
