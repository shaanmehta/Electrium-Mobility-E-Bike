
//Include libraries copied from VESC
#include "VescUart.h"
#include "datatypes.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <PNGdec.h>
#include "electrium_logo.h"
#include "battery.h"
#include <math.h>

PNG png;
TFT_eSPI tft = TFT_eSPI();
#define LCD_LED 27
#define MAX_IMAGE_WIDTH 238
#define WIDTH 238
#define HEIGHT 30
int16_t xpos = 42;
int16_t ypos = 70;
int16_t textxpos = 140;
int16_t textypos = 130;
int16_t batteryxpos = textxpos - 25;
int16_t batteryypos = textypos + 78;

HardwareSerial VescSerial(0);
VescUart UART;

void drawImageFromProgmem(int x, int y, int width, int height, const uint16_t* image) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            // Calculate index in the array
            int index = row * width + col;
            // Read 16-bit pixel from PROGMEM
            uint16_t color = pgm_read_word(&image[index]);
            // Draw pixel
            tft.drawPixel(x + col, y + row, color);
        }
    }
}

void setup() {
	//Setup debug serial
	delay(500);
	//Initialize VESC UART (16 rx, 16tx)
	VescSerial.begin(115200, SERIAL_8N1, 22, 21);
	delay(500);

	pinMode(LCD_LED, OUTPUT);
	digitalWrite(LCD_LED, HIGH);

	tft.init();
	tft.setRotation(2);
	tft.fillScreen(TFT_BLACK);
	tft.setCursor(0,0);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(6);
  tft.setTextFont(1);
	//VERT
	tft.drawLine(0,0,0,480,TFT_GREEN);
	tft.drawLine(319,0,319,480,TFT_GREEN);
	//HORIZ
	tft.drawLine(0,0,319,0,TFT_GREEN);
	tft.drawLine(0,479,319,479,TFT_GREEN);

	drawImageFromProgmem(xpos, ypos, WIDTH, HEIGHT, electrium_logo);

	tft.setTextDatum(MC_DATUM);
  tft.drawString("WELCOME", 160, 170);
	tft.setTextSize(2);
  tft.drawString("You are using", 160, 230);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
	tft.drawString("Electrium Mobility's", 160, 250);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.drawString("F25 Electric Bike :)", 160, 270);
	tft.setTextSize(1);
	tft.drawString("Building Sustainable, Affordable Transportation", 160, 360);
	delay(3500);
	tft.fillScreen(TFT_BLACK);
	tft.setTextFont(1);
	tft.setTextSize(2);
	delay(300);
	UART.setSerialPort(&VescSerial);
	tft.drawLine(0,0,0,480,TFT_GREEN);
	tft.drawLine(319,0,319,480,TFT_GREEN);
	//HORIZ
	tft.drawLine(0,0,319,0,TFT_GREEN);
	tft.drawLine(0,479,319,479,TFT_GREEN);
	drawImageFromProgmem(42, 440, WIDTH, HEIGHT, electrium_logo);
	//batteryval x - 25 , batteryval y - 2.
	drawImageFromProgmem(batteryxpos, batteryypos, 24, 24, battery);
	tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
	tft.drawString("Input Voltage", 160, textypos - 55);
	tft.drawString("Battery Percentage", 160, textypos + 60);
	tft.drawString("Speed of Motor", 160, textypos + 185);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setTextSize(3);
	delay(100);
}

float avgMotorCurrent = 0;
float avgInputCurrent = 0;
float dutyCycleNow = 0;
float rpm = 0;
float inpVoltage = 0;
float ampHours = 0; // total drawn over time
float ampHoursCharged = 0; // charge battery has recieved
float wattHours = 0;
float wattHoursCharged = 0;
long tachometer = 0;
long tachometerAbs = 0;
float tempMosfet = 0;
float tempMotor = 0;
float pidPos = 0;
int batteryval = 0;
int speed = 0;
uint8_t id;

float last_avgMotorCurrent = -1;
float last_avgInputCurrent = -1;
float last_dutyCycleNow = -1;
float last_rpm = -1;
float last_inpVoltage = -1;
float last_ampHours = -1;
float last_ampHoursCharged = -1;
float last_wattHours = -1;
float last_wattHoursCharged = -1;
long last_tachometer = -1;
long last_tachometerAbs = -1;
float last_tempMosfet = -1;
float last_tempMotor = -1;
float last_pidPos = -1;
int last_batteryval = -1;
int last_speed = -1;
int last_fill = -1;

void loop() {

	if (UART.getVescValues()) {
  	inpVoltage = UART.data.inpVoltage;
		batteryval = (int)floor(((inpVoltage - 30.0f) / (36.0f - 30.0f)) * 100.0f);
		speed = (int)floor(0.00446 * UART.data.rpm);
	}

	if(last_inpVoltage != inpVoltage) {
		tft.setCursor(textxpos - 10, textypos - 40);
		tft.print("        ");
		tft.setCursor(textxpos - 10, textypos - 40);
		tft.print(inpVoltage, 2);
		tft.setTextSize(1);
		tft.setCursor(tft.getCursorX(), tft.getCursorY() + 14);
    tft.print("V");
		tft.setTextSize(3);
	}

	if(last_batteryval != batteryval) {
		tft.setCursor(textxpos, textypos + 80);
		tft.print("        ");
		tft.setCursor(textxpos, textypos + 80);
		tft.print(batteryval);
  	tft.print("%");
		if(batteryval > 80 && batteryval <= 100 && last_fill != 5) {
			tft.fillRect(batteryxpos + 8, batteryypos + 5, 8, 18, TFT_GREEN);
			last_fill = 5;
		} else if(batteryval > 60 && batteryval <= 80 && last_fill != 4) {
			tft.fillRect(batteryxpos + 8, batteryypos + 5, 8, 18, TFT_BLACK);
			tft.fillRect(batteryxpos + 8, batteryypos + 8, 8, 15, TFT_GREEN);
			last_fill = 4;
		} else if(batteryval > 40 && batteryval <= 60 && last_fill != 3) {
			tft.fillRect(batteryxpos + 8, batteryypos + 5, 8, 18, TFT_BLACK);
			tft.fillRect(batteryxpos + 8, batteryypos + 11, 8, 12, TFT_GREEN);
			last_fill = 3;
		} else if(batteryval > 20 && batteryval <= 40 && last_fill != 2) {
			tft.fillRect(batteryxpos + 8, batteryypos + 5, 8, 18, TFT_BLACK);
			tft.fillRect(batteryxpos + 8, batteryypos + 14, 8, 9, TFT_GREEN);
			last_fill = 2;
		} else if(batteryval > 5 && batteryval <= 20 && last_fill != 1) {
			tft.fillRect(batteryxpos + 8, batteryypos + 5, 8, 18, TFT_BLACK);
			tft.fillRect(batteryxpos + 8, batteryypos + 17, 8, 6, TFT_YELLOW);
			last_fill = 1;
		} else if(batteryval > 0 && batteryval <= 5 && last_fill != 0){
			tft.fillRect(batteryxpos + 8, batteryypos + 5, 8, 18, TFT_BLACK);
			tft.fillRect(batteryxpos + 8, batteryypos + 21, 8, 2, TFT_RED);
			last_fill = 0;
		} else if(batteryval <= 0 && last_fill != -1) {
			tft.fillRect(batteryxpos + 8, batteryypos + 5, 8, 18, TFT_BLACK);
			last_fill = -1;
		}
	}

	if(last_speed != speed) {
		tft.setCursor(textxpos + 5, textypos + 200);
		tft.print("        ");
		tft.setCursor(textxpos, textypos + 200);
		tft.print(speed);
		tft.setTextSize(1);
		tft.setCursor(tft.getCursorX(), tft.getCursorY() + 14);
  	tft.print("km/h");
		tft.setTextSize(3);
	}

	last_inpVoltage = inpVoltage;
	last_batteryval = batteryval;
	last_speed = speed;

	/*
	batteryval -= 2;
	if(batteryval < 0){
		batteryval = 100;
	}
	*/
	/*
	inpVoltage += 1.0;
	if(inpVoltage > 10.0) {
		inpVoltage = 0.0;
	}
	ampHoursCharged += 2.0;
	if(ampHoursCharged > 20.0) {
		ampHoursCharged = 0.0;
	}
	avgInputCurrent += 3.0;
	if(avgInputCurrent > 30.0) {
		avgInputCurrent = 0.0;
	}
	*/
	delayMicroseconds(10); // x0.01 of a millisecond
}

// Callback function to draw pixels to the display
int pngDraw(PNGDRAW *pDraw) {
  uint16_t lineBuffer[MAX_IMAGE_WIDTH];
  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  tft.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
  return 1;
}





