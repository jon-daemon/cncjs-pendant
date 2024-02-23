#include <AlignedJoy.h>
#include <Coordinates.h>
#include <PinChangeInterrupt.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>
#include <Bounce2.h>
#include <WS2812.h>

#define JOYSTICK_MIN 0
#define JOYSTICK_MAX 1023
#define JOYSTICK_MID JOYSTICK_MAX / 2
#define JOYSTICK_MAX_R 512
#define JOYSTICK_TOLERANCE 50
#define JOYSTICK_REPORTING_MS 100
#define DISPLAY_MESSAGE 800
#define INACTIVITY_INTERVAL 60000

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

unsigned long now;
unsigned long lastJoystickPrintTime;
unsigned long lastMessage;
unsigned long last_buzz;
unsigned long lastActivity;

//PIN ASSIGNMENTS 
static int PIN_BUTTON_AXIS = 11;
static int PIN_BUTTON_DISTANCE = 10;
static int PIN_BUTTON_ZERO = 5;
static int PIN_BUTTON_JOYSTICK = 4;
static int PIN_BUTTON_FUNCTION = 17;
static int PIN_BUTTON1 = 3;
static int PIN_BUTTON2 = 2;
static int PIN_JOYSTICK_X = A0;
static int PIN_JOYSTICK_Y = A1;
static int PIN_JOYSTICK_Z = A2;
static int PIN_MPG_A = 8;
static int PIN_MPG_B = 9;
static int STRIP_LEDS = 6;
static int BUZZER = 7;

static int MODE_JOYSTICK = 1;
static int MODE_MPG = 2;
int mode = MODE_MPG;
int fn = 0;

// mpg
static int AXIS_X = 0;
static int AXIS_Y = 1;
static int AXIS_Z = 2;

static float DISTANCE_VALUES[] = {1.0, 0.5, 0.25, 0.1, 0.05, 0.01};
static int DISTANCE_SIZE = sizeof(DISTANCE_VALUES)/sizeof(float);
int aVal = 0;
int bVal = 0;
volatile int encoderPos = 0;
volatile int oldEncPos = 0;
int axis = 0;
int distance = 1;

// joystick
Coordinates point = Coordinates();
AlignedJoy joystick_1(PIN_JOYSTICK_X, PIN_JOYSTICK_Y);
int rVal = 0;
int xVal = 0;
int yVal = 0;
int zVal = 0;
const float RAD_PER_SEG = M_PI / 8;
int idleXY = 1;
int idleZ = 1;
int locked = 1;

// screen stuff
int distanceX = 0;
int distanceY = 0;
int distanceW = 0;
int distanceH = 0;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
String message;
int showMessage = 0;

WS2812 LED(2);

// buttons
Bounce2::Button axisButton = Bounce2::Button();
Bounce2::Button distanceButton = Bounce2::Button();
Bounce2::Button zeroButton = Bounce2::Button();
Bounce2::Button functionButton = Bounce2::Button();
Bounce2::Button button1 = Bounce2::Button();
Bounce2::Button button2 = Bounce2::Button();
Bounce2::Button joystickButton = Bounce2::Button();

void setup() {
	Serial.begin(115200);
	while(!Serial){};
	axisButton.attach(PIN_BUTTON_AXIS, INPUT_PULLUP);
	axisButton.setPressedState(LOW);
	distanceButton.attach(PIN_BUTTON_DISTANCE, INPUT_PULLUP);
	distanceButton.setPressedState(LOW);
	zeroButton.attach(PIN_BUTTON_ZERO, INPUT_PULLUP);
	zeroButton.setPressedState(LOW);
	functionButton.attach(PIN_BUTTON_FUNCTION, INPUT_PULLUP);
	functionButton.setPressedState(LOW);
	button1.attach(PIN_BUTTON1, INPUT_PULLUP);
	button1.setPressedState(LOW);
	button2.attach(PIN_BUTTON2, INPUT_PULLUP);
	button2.setPressedState(LOW);
	joystickButton.attach(PIN_BUTTON_JOYSTICK, INPUT_PULLUP);
	joystickButton.setPressedState(LOW);
	pinMode(PIN_MPG_A, INPUT);
	pinMode(PIN_MPG_B, INPUT);
	attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_MPG_A), MPG_PinA_Interrupt, RISING);
	attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PIN_MPG_B), MPG_PinB_Interrupt, RISING);
	LED.setOutput(STRIP_LEDS);
	pinMode(BUZZER, OUTPUT);
//	SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
		Serial.println(F("SSD1306 allocation failed"));
		for(;;); // Don't proceed, loop forever
	}
	display.setRotation(2);
	clearDisplay();
	//while(!Serial.available()){}
	Serial.println("on");
	updateDisplay();
}

void buzz(int times, unsigned int onTime, unsigned int offTime){
	int i =0;
	last_buzz = millis();
	while (i < times){
		if (millis() - last_buzz < onTime){
			digitalWrite(BUZZER, HIGH);
		}
		else{
			digitalWrite(BUZZER, LOW);
			if (millis() - last_buzz > onTime + offTime){
				last_buzz = millis();
				i++;
			}
		}
	}
}

void MPG_PinA_Interrupt(){
	bVal = digitalRead(PIN_MPG_B);
	if(!bVal) {
		encoderPos++;
	}
}

void MPG_PinB_Interrupt(){
	aVal = digitalRead(PIN_MPG_A);
	if(!aVal) {
	  encoderPos--;
	}
}

void MPG_Reset() {
	encoderPos = 0;
	oldEncPos = 0;
}

float getDistance() {
	for (int i = 0; i < DISTANCE_SIZE; i++){
		if (i == distance) return DISTANCE_VALUES[i];
	}
}

String getAxisLabel() {
	return (axis == AXIS_X) ? "X" : (axis == AXIS_Y) ? "Y" : "Z";
}

void clearDisplay() {
	display.clearDisplay();
	display.display();
}

void updateDisplay() {
	display.clearDisplay();
	display.setTextColor(SSD1306_WHITE);

	if (!locked){
		if (showMessage > 0){
			showMessage = -1;
			display.setTextSize(2);
			display.getTextBounds(message, 0, 0, &distanceX, &distanceY, &distanceW, &distanceH);
			display.setCursor((SCREEN_WIDTH - distanceW)/2, (SCREEN_HEIGHT - distanceH)/2);
			display.print(message);
		}
		else if (!showMessage){
			if (fn) display.drawRect(0, 0, 128, 32, WHITE);

			if (mode == MODE_MPG){
				if (axis == AXIS_X) LED.set_crgb_at(1, {255, 0, 0});
				else if (axis == AXIS_Y) LED.set_crgb_at(1, {155, 0, 255});
				else LED.set_crgb_at(1, {135, 255, 0});

				LED.set_crgb_at(0, {0, 255, 0});

				String axisLabel = getAxisLabel();
				String distanceLabel = String(getDistance(), 2) + " mm";
				display.setCursor(0, 5);
				display.setTextSize(4);
				display.print(axisLabel);
				display.setTextSize(2);
				display.getTextBounds(distanceLabel, 0, 0, &distanceX, &distanceY, &distanceW, &distanceH);
				display.setCursor(SCREEN_WIDTH - distanceW, (SCREEN_HEIGHT - distanceH)/2);
				display.print(distanceLabel);
			}
			else if (mode == MODE_JOYSTICK){
				display.drawCircle(16, 16, 6, WHITE);
				display.drawTriangle(16,2,11,8,21,8, WHITE);
				display.drawTriangle(2,16,8,11,8,21, WHITE);
				display.drawTriangle(24,11,24,21,30,16, WHITE);
				display.drawTriangle(11,24,21,24,16,30, WHITE);
				String infoLabel;
				if ((idleXY) && (idleZ)){
					infoLabel = "Idle";
					LED.set_crgb_at(1, {0, 255, 0});
					LED.set_crgb_at(0, {255, 0, 0});
				}
				else{
					if (!idleXY) infoLabel = "Move XY";
					else infoLabel = "Move Z";
					LED.set_crgb_at(1, {0, 0, 0});
					LED.set_crgb_at(0, {255, 110, 0});
				}
				display.setTextSize(2);
				display.getTextBounds(infoLabel, 0, 0, &distanceX, &distanceY, &distanceW, &distanceH);
				display.setCursor((SCREEN_WIDTH + 32 - distanceW)/2, (SCREEN_HEIGHT - distanceH)/2);
				display.print(infoLabel);
			}
		}
	}
	else{
		String lockStr = "LOCKED";
		display.setCursor(0, 5);
		display.setTextSize(3);
		display.getTextBounds(lockStr, 0, 0, &distanceX, &distanceY, &distanceW, &distanceH);
		display.setCursor(SCREEN_WIDTH - distanceW, (SCREEN_HEIGHT - distanceH)/2);
		display.print(lockStr);
		LED.set_crgb_at(1, {0, 255, 0});
		LED.set_crgb_at(0, {0, 255, 0});
	}

	LED.sync();

	display.display();
}

void printJoystickData(float angle, float r, int z) {

	if ((idleXY) && (idleZ)){
		if (r > JOYSTICK_TOLERANCE) {
			idleXY = 0;
			showMessage = 0;
			updateDisplay();
		}
		else if (abs(z) > JOYSTICK_TOLERANCE){
			idleZ = 0;
			showMessage = 0;
			updateDisplay();
		}
	}
	else{
		Serial.print("JD|");
		if (abs(z) <= JOYSTICK_TOLERANCE){
			if (!idleZ){
				idleZ = 1;
				updateDisplay();
			}
		}
		if (r > JOYSTICK_TOLERANCE){
			if (!idleXY) z = 0;
			if(angle < RAD_PER_SEG) Serial.print("EAST|");
			else if(angle < 3*RAD_PER_SEG) Serial.print("NORTHEAST|");
			else if(angle < 5*RAD_PER_SEG) Serial.print("NORTH|");
			else if(angle < 7*RAD_PER_SEG) Serial.print("NORTHWEST|");
			else if(angle < 9*RAD_PER_SEG) Serial.print("WEST|");
			else if(angle < 11*RAD_PER_SEG) Serial.print("SOUTHWEST|");
			else if(angle < 13*RAD_PER_SEG) Serial.print("SOUTH|");
			else if(angle < 15*RAD_PER_SEG) Serial.print("SOUTHEAST|");
			else if(angle <= 2*M_PI) Serial.print("EAST|");
			//else {Serial.print("DUNNO-|");Serial.print(angle);}
		}
		else {
			Serial.print("CENTER|");
			if (!idleXY){
				idleXY = 1;
				updateDisplay();
			}
		}
		Serial.println(String(angle) + "|" + String(min(r / JOYSTICK_MAX_R, 1.0)) + "|" + String(z/float(JOYSTICK_MID)));
		lastActivity = now;
	}
}

void handleJoystick() { 
	xVal = joystick_1.read(X);
	yVal = joystick_1.read(Y);
	zVal = analogRead(PIN_JOYSTICK_Z);

	xVal = JOYSTICK_MID - xVal;
	
	yVal = yVal - JOYSTICK_MID;

//	if(yVal >= JOYSTICK_MID) yVal = 1 - (yVal - JOYSTICK_MID);
//	else yVal = JOYSTICK_MID - yVal;

	zVal = zVal - JOYSTICK_MID;

	point.fromCartesian(xVal, yVal);
	rVal = point.getR();

	if(now - lastJoystickPrintTime > JOYSTICK_REPORTING_MS) {
		printJoystickData(point.getAngle(), rVal, zVal);
		lastJoystickPrintTime = now;
	}
}

void handleMPG() {
	if(oldEncPos != encoderPos) {
		Serial.println("MC|" + String(encoderPos - oldEncPos) + "|" + getAxisLabel() + "|" + String(getDistance(), 3));
		oldEncPos = encoderPos;
		lastActivity = now;
	}
}

void checkButtons(){
	axisButton.update();
	distanceButton.update();
	zeroButton.update();
	functionButton.update();
	button1.update();
	button2.update();
	joystickButton.update();

	if (functionButton.pressed()) {
		fn = 1;
		showMessage = 0;
		buzz(1,10,0);
		updateDisplay();
	}
	if (functionButton.released()){
		fn = 0;
		showMessage = 1;
		updateDisplay();
	}

	if (joystickButton.pressed()) {
		if (!fn){
			if (!locked){
				if (mode != MODE_JOYSTICK){
					mode = MODE_JOYSTICK;
					showMessage = 0;
					//setColor (0,0,255,2);
					buzz(1,10,0);
					updateDisplay();
				}
				else if ((idleXY) && (idleZ)){
					Serial.println("BT|$X");
					message = "UNLOCK";
					showMessage = 1;
					buzz(1,10,0);
				}
				lastActivity = now;
			}
		}
		else{
			locked = (locked) ? 0 : 1;
			buzz(1,10,0);
			lastActivity = now;
		}
	}

	if (!locked){
		if (axisButton.pressed()) {
			if (mode != MODE_MPG) {
				if ((idleXY) && (idleZ)){
					if (fn) axis = AXIS_Z;
					else if (axis > AXIS_Y) axis = AXIS_X;
					mode = MODE_MPG;
					MPG_Reset();
					showMessage = 0;
					buzz(1,10,0);
					updateDisplay();
				}
			}
			else{
				if (fn) axis = AXIS_Z;
				else if (++axis > AXIS_Y) axis = AXIS_X;
				showMessage = 0;
				buzz(1,10,0);
				updateDisplay();
			}
			lastActivity = now;
		}

		if (distanceButton.pressed()) {
			if (mode != MODE_MPG) {
				if ((idleXY) && (idleZ)){
					mode = MODE_MPG;
					MPG_Reset();
					showMessage = 0;
					buzz(1,10,0);
					updateDisplay();
				}
			}
			else{
				if (!fn){
					if (++distance > DISTANCE_SIZE - 1){
						distance = 0;
						buzz(2,20,100);
					}
					else buzz(1,10,0);
				}
				else{
					if (--distance < 0){
						distance = DISTANCE_SIZE - 1;
						buzz(2,20,100);
					}
					else buzz(1,10,0);
				}
				showMessage = 0;
				updateDisplay();
			}
			lastActivity = now;
		}

		if (zeroButton.pressed()) {
			if (mode != MODE_MPG) {
				if ((idleXY) && (idleZ)){
					Serial.println("BT|G10L20P1X0Y0"); // zero XY
					message = "SET X0 Y0";
					buzz(1,10,0);
					showMessage = 1;
				}
			}
			else{
				Serial.print("BT|G10L20P1");
				message = "SET ";
				if (!fn){
					Serial.println(getAxisLabel() + '0'); // zero current axis
					message += getAxisLabel() + '0';
				}
				else{
					if (axis < AXIS_Z){
						Serial.println("X0Y0"); // zero XY
						message += "X0 Y0";
					}
					else{
						Serial.println("Z0.1"); // zero XY
						message += "Z +0.1";
					}				
				}
				buzz(1,10,0);
				showMessage = 1;
			}
			lastActivity = now;
		}

		if (button1.pressed()){
			if (((mode != MODE_MPG) && (idleXY) && (idleZ) ) || (mode != MODE_JOYSTICK)){
				Serial.print("BT|");
				if (!fn){
					Serial.println("$H");
					message = "HOME";
				}
				else{
					Serial.println("G53G21X-50Y-50Z-10");
					message = "XY-50 Z-10";
				}
				buzz(1,10,0);
				showMessage = 1;
				lastActivity = now;
			}
		}

		if (button2.pressed()){
			if (((mode != MODE_MPG) && (idleXY) && (idleZ)) || (mode != MODE_JOYSTICK)){
				Serial.print("BT|");
				if (!fn){
					Serial.println("G28");
					message = "G28";
				}
				else{
					Serial.println("G28.1");
					message = "G28.1";
				}
				buzz(1,10,0);
				showMessage = 1;
				lastActivity = now;
			}
		}
	}
}

void loop(){
	checkButtons();

	now = millis();

	if (!locked){
		if (now - lastActivity > INACTIVITY_INTERVAL){
			locked = 1;
			buzz(1,10,0);
			showMessage = 1;
		}
		if(mode == MODE_JOYSTICK) {
			handleJoystick();
		}
		else if(mode == MODE_MPG) {
			handleMPG();
		}
	}

	if (showMessage > 0){
		if(now - lastMessage > DISPLAY_MESSAGE) {
			updateDisplay();
			lastMessage = now;
		}
	}
	else if (showMessage < 0){
		if(now - lastMessage > DISPLAY_MESSAGE) {
			showMessage = 0;
			updateDisplay();
		}
	}
}
