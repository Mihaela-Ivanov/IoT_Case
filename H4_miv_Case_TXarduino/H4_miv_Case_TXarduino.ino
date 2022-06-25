/*
 Name:		H4_miv_Case_TXarduino.ino
 Created:	6/23/2022 12:57:28 PM
 Author:	CW40DH / miv
*/
//--- INCLUDES ---//
// XBee //
#include "MivTimer3.h"
#include <XBee.h>
#include <Printers.h>

// Debug //
#include <avr_debugger.h>
#include <avr8-stub.h>
#include <app_api.h>

// Timer Interrupt //
#include <MsTimer2.h>
#include <MivTimer3.h>


//--- GLOBAL VARIABLES & MACROS ---//
// Serial //
#define USE_SERIAL Serial1 //Define serial one place, so it's easy to change.

// XBee //
XBee xbee = XBee(); //Create the XBee instance of object.
uint8_t payload[] = { 0, 0 };
String sendStatus = "";
XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, 0x4155B983); //Address of receiving XBee: 0013A200+4155B983 divided.
ZBTxRequest zbTx;
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

// Light //
#define lightPin 13 //LED simulation room lighting.
int LightState = 0; //State for light on (1) and light off (0).

// Ultrasonic //
#define echoPinIn A2 // attach pin D2 Arduino to pin Echo of HC-SR04 
#define trigPinIn 2 //attach pin D3 Arduino to pin Trig of HC-SR04 
#define echoPinOut A3 // attach pin D2 Arduino to pin Echo of HC-SR04 
#define trigPinOut 3 //attach pin D3 Arduino to pin Trig of HC-SR04 
int distance = 1; //Variable for the distance measurement.
long durationIn; //Variable for the duration of sound wave travel for incoming people sensor.
int distanceIn; //Variable for the distance measurement for incoming people sensor.
const byte interruptPinIn = 13; //For interrupt from ultrasonic sensor.

long durationOut; //Variable for the duration of sound wave travel for outgoing people sensor.
int distanceOut; //Variable for the distance measurement for outgoing people sensor.
const byte interruptPinOut = 13; //For interrupt from ultrasonic sensor.

// Interrupt //
volatile byte interruptCounter = 0; //Counting how many interrupts has occurred. 
int IFin = 0; //InterruptFlag for incoming stream of people.
int IFout = 0; //InterruptFlag for outgoing stream of people.

// Person Handling //
int personCount = 0; //Counting number of interrupts from ultrasonic, Counter increased and decreaes depending on data from ultrasonic.


//--- INTERRUPTS ---//
//void ICACHE_RAM_ATTR incomingStream_ISR();
//void ICACHE_RAM_ATTR outgoingStream_ISR();


//--- FUNCTIONS ---//
// XBee TX //
String XBee_TX() {
    // break down 10-bit reading into two bytes and place in payload because msg is more than 8 bit.
    payload[0] = personCount >> 8 & 0xff; //high 8 bit i msg.
    payload[1] = personCount & 0xff; //low 8 bit in msg. &'ing removed high 8 bits, leaving 8 low bits.

    zbTx = ZBTxRequest(addr64, payload, sizeof(payload));

    xbee.send(zbTx);

    delay(1000);
}

void incomingStream_callBack_pointerFunction() {
    IFin = 1;
}
void outgoingStream_callBack_pointerFunction() {
    IFout = 1;
}

void incomingStream() {
    digitalWrite(trigPinIn, LOW); //Clears the trigPin condition.
    delayMicroseconds(2); //For 2 microseconds.
    digitalWrite(trigPinIn, HIGH); //Sets the trigPin HIGH (ACTIVE).
    delayMicroseconds(10); //For 10 microseconds.
    digitalWrite(trigPinIn, LOW); //Clears the trigPin condition.

    durationIn = pulseIn(echoPinIn, HIGH); //Get analog value, the sound wave travel time in microseconds.
    distanceIn = durationIn * 0.034 / 2; //Make sense of the value to a distance in cm. (Speed of sound wave divided by 2).

    if (distanceIn <= 30) { //If person is 30 cm or less closer to the sensor, 
        interruptCounter--;
        personCount++;
        LightState = 1;

        Serial.print("An incoming person has interrupted. Number of persons in the room: ");
        Serial.println(personCount);

        distanceOut = 1000; //Setting distance unrealistically high to 'clear' for next comming data input.
    }
}

void outgoingStream() {
    digitalWrite(trigPinOut, LOW); //Clears the trigPin condition.
    delayMicroseconds(2); //For 2 microseconds.
    digitalWrite(trigPinOut, HIGH); //Sets the trigPin HIGH (ACTIVE).
    delayMicroseconds(10); //For 10 microseconds.
    digitalWrite(trigPinOut, LOW); //Clears the trigPin condition.
    durationOut = pulseIn(echoPinOut, HIGH); //Get analog value, the sound wave travel time in microseconds.
    distanceOut = durationOut * 0.034 / 2; //Make sense of the value to a distance in cm. (Speed of sound wave divided by 2).

    if (distanceIn <= 30) { //If person is 30 cm or less closer to the sensor, 
        interruptCounter--;
        if (personCount >= 0) {
            personCount--;
            LightState = 1;
        }
        else if (personCount <= 0) {
            LightState = 0;
            personCount = 0; //Ensure the number of persons does not go below 0.
        }

        Serial.print("An outgoing person has interrupted. Number of persons in the room: ");
        Serial.println(personCount);

        distanceOut = 1000; //Setting distance unrealistically high to 'clear' for next comming data input.
    }
}

void checkPersonStream() {
    if (IFin == 1) {
        incomingStream();
    }
    else if (IFout == 1) {
        outgoingStream();
    }
}

void checkLightState() {
    if (1 == LightState) {
        digitalWrite(lightPin, HIGH);
    }
    else if (0 == LightState) {
        digitalWrite(lightPin, LOW);
    }
}


//--- INITIALIZE ---//
void setup() {
	// Serial //
	USE_SERIAL.begin(9600); //Baud rate must be the same as is on xBee module.

	// XBee //
	xbee.setSerial(Serial); //XBee uses Serial0.

    // Ultrasound //
    pinMode(trigPinIn, OUTPUT); //Sets the trigPin as an OUTPUT for incoming people sensor.
    pinMode(echoPinIn, INPUT); //Sets the echoPin as an INPUT for incoming people sensor.
    //pinMode(trigPinIn, INPUT_PULLUP);//Sets ultrasonic trigger pin as input and pull up.
    //attachInterrupt(xdigitalPinToInterrupt(trigPinIn), incomingStream, FALLING); //Interrupt on (pin, function to execute, flank).

    pinMode(trigPinOut, OUTPUT); //Sets the trigPin as an OUTPUT for outgoing people sensor.
    pinMode(echoPinOut, INPUT);//Sets the echoPin as an INPUT for outgoing people sensor.
    //pinMode(trigPinOut, INPUT_PULLUP);//Sets ultrasonic trigger pin as input and pull up.
    //attachInterrupt(digitalPinToInterrupt(trigPinOut), outgoingStream, FALLING); //Interrupt on (pin, function to execute, flank).

    // Timer Interrupt //
    MsTimer2::set(1000, incomingStream_callBack_pointerFunction); //For every second execute function.
    MsTimer2::start(); //Start timer.
    MivTimer3::set(1000, outgoingStream_callBack_pointerFunction); //For every second execute function.
    MivTimer3::start(); //Start timer.

    // Light LED //
    pinMode(lightPin, OUTPUT);
}//END Setup


//--- MAIN PROGRAM ---//
void loop() {
    XBee_TX();
    checkLightState();
    checkPersonStream();
}//END main
