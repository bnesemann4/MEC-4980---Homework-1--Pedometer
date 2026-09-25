#include <Arduino.h>
#include <math.h>
#include <Adafruit_BNO08x.h>
#include <Adafruit_ST7789.h>

Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(240,135);

void setReports();

#define BNO08X_RESET -1

int pinD0 = 0;
int pinD1 = 1;
int pinD2 = 2;

float totalSteps = 0;
float totalDistance = 0;
float curStride = 3.;

enum AxisMode {
  MODE_BOTH,  // 0
  MODE_X,     // 1
  MODE_Y,     // 2
  MODE_COUNT  // 3
};

enum menuMode {
  STEPS_SCREEN,     // 0
  DISTANCE_SCREEN,  // 1
  STRIDE_SCREEN,    // 2
  RAW_DATA,         // 3
  MENU_COUNT        // 4
};

menuMode curMenu = STEPS_SCREEN;

volatile long prevChangeTime = 0;
volatile long prevChangeTimeTwo = 0;
long debounceTime = 50;
volatile bool changeButtonFlagUp = false;
volatile bool changeButtonFlagDown = false;
volatile bool menuButtonFlag = false;

Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

void IRAM_ATTR buttonToChangeThingsUp() {
  long now = millis();
  if (now > prevChangeTime + debounceTime) {
    changeButtonFlagUp = true;
    prevChangeTime = now;
  }
}

void IRAM_ATTR buttonToChangeThingsDown() {
  long now = millis();
  if (now > prevChangeTime + debounceTime) {
    changeButtonFlagDown = true;
    prevChangeTime = now;
  }
}

void IRAM_ATTR buttonToChangeMenu() {
  long now = millis();
  if (now > prevChangeTimeTwo + debounceTime) {
    menuButtonFlag = true;
    prevChangeTimeTwo = now;
  }
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  pinMode(0,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(0), buttonToChangeThingsDown, RISING);

  pinMode(1,INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(1), buttonToChangeThingsUp, RISING);

  pinMode(2,INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(2), buttonToChangeMenu, RISING);

  Serial.println("Adafruit BNO08x test!");

  // Try to initialize!
  if (!bno08x.begin_I2C()) {
    // if (!bno08x.begin_UART(&Serial1)) {  // Requires a device with > 300 byte
    // UART buffer! if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
    Serial.println("Failed to find BNO08x chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("BNO08x Found!");

  setReports();

  display.init(135, 240);
  display.setRotation(1);
  canvas.setTextColor(ST77XX_BLUE);
  pinMode(TFT_BACKLITE,OUTPUT);
  digitalWrite(TFT_BACKLITE,1);

}


void loop() {
  delay(10);

  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports();
  }

  if (!bno08x.getSensorEvent(&sensorValue)) {
    return;
  }

  float x = sensorValue.un.accelerometer.x;
  float y = sensorValue.un.accelerometer.y;
  float z = sensorValue.un.accelerometer.z;

  if (menuButtonFlag) {
    menuButtonFlag = false;
    curMenu = (menuMode)(((int)curMenu + 1) % (int)menuMode::MENU_COUNT);
  }

  if (changeButtonFlagUp) {
    if (curMenu == STRIDE_SCREEN) {
      curStride += 0.5;
      if (curStride > 10) {
        curStride = 3;
      }
    } 
    changeButtonFlagUp = false;
  }

  if (changeButtonFlagDown) {
    if (curMenu == STRIDE_SCREEN) {
      curStride -= 0.5;
      if (curStride < 3) {
        curStride = 10;
      }
    } 
    changeButtonFlagDown = false;
  }

  canvas.fillScreen(ST77XX_WHITE);
  canvas.setCursor(0,20);
  if (curMenu == menuMode::STEPS_SCREEN) {
    canvas.print("Total steps: ");
    canvas.println(sensorValue.un.stepCounter.steps);
    canvas.print("Press D2 to change to distance menu.");
  }

  if (curMenu == menuMode::DISTANCE_SCREEN) {
    canvas.print("Total distance travelled: ");
    totalDistance = (sensorValue.un.stepCounter.steps)*(curStride);
    canvas.println(totalDistance);
    canvas.print("Press D2 to change to stride menu.");
  }

  if (curMenu == menuMode::STRIDE_SCREEN) {
    canvas.print("Current stride length: ");
    canvas.print(curStride);
    canvas.println(" feet");
    canvas.println("Press D0 to decrease stride length.");
    canvas.println("Press D1 to increase stride length.");
    canvas.print("Press D2 to change to acceleration data menu.");
  }

  if (curMenu == menuMode::RAW_DATA) {
    canvas.print("X Acceleration: ");
    canvas.println(x);
    canvas.print("Y Acceleration: ");
    canvas.println(y);
    canvas.print("Z Acceleration: ");
    canvas.println(z);
    canvas.print("Press D2 to change to steps menu.");
  }

  display.drawRGBBitmap(0,0, canvas.getBuffer(), 240, 135);

}

void setReports(void) {
  Serial.println("Setting desired reports");
  
  if (!bno08x.enableReport(SH2_STEP_COUNTER)) {
    Serial.println("Could not enable step counter");
  } else {
    Serial.println("Set accelerometer report...success!");
  }
  
  if (!bno08x.enableReport(SH2_ACCELEROMETER)) {
    Serial.println("Could not enable accelerometer");
  } else {
    Serial.println("Set accelerometer report...success!");
  }
}