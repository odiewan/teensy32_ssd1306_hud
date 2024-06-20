
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Stepper.h>
#include <led_pulse_train.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES       10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT     16
#define LOGO_WIDTH      16
#define LOGO_X_MIN      -8
#define LOGO_X_MAX      120

#define LED_ON_TIME     50
#define LED_OFF_TIME    750

#define DIR_DOWN        -1
#define DIR_NONE        0
#define DIR_UP          1


#define SER_WAIT_TICKS  10
#define COMPASS_Y       0

#define STEPS 100

#define SETUP_DELAY     1000


static const unsigned char PROGMEM logo_bmp[] = {
  0b00000011, 0b11000000,
  0b00000110, 0b01100000,
  0b00001100, 0b00110000,
  0b00011000, 0b00011000,

  0b00110000, 0b00001100,
  0b01100000, 0b00000110,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,

  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,

  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  };

static const unsigned char PROGMEM bmp00[] = {
  0b00000001, 0b10000000,
  0b00000011, 0b11000000,
  0b00000111, 0b11100000,
  0b00001111, 0b11110000,

  0b00011111, 0b11111000,
  0b00111111, 0b11111100,
  0b01111111, 0b11111110,
  0b11111111, 0b11111111,

  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,

  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  };

static const unsigned char PROGMEM bmp0l[] = {
  0b10000000, 0b00000000,
  0b11100000, 0b00000000,
  0b11111000, 0b00000000,
  0b11111110, 0b00000000,

  0b11111111, 0b10000000,
  0b11111111, 0b11000000,
  0b11111111, 0b11111100,
  0b11111111, 0b11111111,

  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,

  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  };

static const unsigned char PROGMEM bmp02[] = {
  0b00000000, 0b10000001,
  0b00000000, 0b10000111,
  0b00000000, 0b00011111,
  0b00000000, 0b01111111,

  0b00000001, 0b11111111,
  0b00000111, 0b11111111,
  0b00011111, 0b11111111,
  0b01111111, 0b11111111,

  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,

  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  0b00000111, 0b11100000,
  };

static const unsigned char PROGMEM bmp03[] = {
    0b00000001, 0b10000000,
    0b00000001, 0b10000000,
    0b00000011, 0b11000000,
    0b00000011, 0b11000000,

    0b00000011, 0b11000000,
    0b00000111, 0b11100000,
    0b00011111, 0b11111000,
    0b11111111, 0b11111111,

    0b11111111, 0b11111111,
    0b00011111, 0b11111000,
    0b00000111, 0b11100000,
    0b00000011, 0b11000000,

    0b00000011, 0b11000000,
    0b00000011, 0b11000000,
    0b00000001, 0b10000000,
    0b00000001, 0b10000000
  };

static const unsigned char PROGMEM diamond[] =
  {

    0b00000001, 0b10000000,
    0b00000011, 0b11000000,
    0b00000111, 0b11100000,
    0b00001111, 0b11110000,

    0b00011111, 0b11111000,
    0b00111111, 0b11111100,
    0b01111111, 0b11111110,
    0b11111111, 0b11111111,

    0b11111111, 0b11111111,
    0b01111111, 0b11111110,
    0b00111111, 0b11111100,
    0b00011111, 0b11111000,

    0b00001111, 0b11110000,
    0b00000111, 0b11100000,
    0b00000011, 0b11000000,
    0b00000001, 0b10000000

  };

static const unsigned char PROGMEM cross[] = {

    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,

    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,

    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000000, 0b10000000, 0b00000000,
    0b00000000, 0b00000000, 0b10000000, 0b00000000,
    0b00000000, 0b00000000, 0b10000000, 0b00000000,

    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b11111111, 0b11111111, 0b11111111, 0b11111111,

    0b11111111, 0b11111111, 0b11111111, 0b11111111,
    0b00000000, 0b00000000, 0b10000000, 0b00000000,
    0b00000000, 0b00000000, 0b10000000, 0b00000000,
    0b00000000, 0b00000000, 0b10000000, 0b00000000,

    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,

    0b00000000, 0b00000000, 0b10000000, 0b00000000,
    0b00000000, 0b00000000, 0b10000000, 0b00000000,
    0b00000000, 0b00000000, 0b10000000, 0b00000000,
    0b00000000, 0b00000000, 0b10000000, 0b00000000,

    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,
    0b00000000, 0b00000001, 0b10000000, 0b00000000,



  };


int x;
int dir;
int def_count = 0;
int dir_up_count = 0;
int dir_dn_count = 0;
int azim = 0;
int64_t iCount = 0;

// Stepper stpr(STEPS, 8, 9, 10, 11);

//=================================================================================================
void ledToggle() {
  static bool bit = false;
  bit = !bit;
  digitalWrite(LED_BUILTIN, bit);
  }

//=================================================================================================
int azim_to_x() {
  return map(azim, 0, 359, LOGO_X_MIN, LOGO_X_MAX);
}

//=================================================================================================
void setup() {
  int ser_wait_cnt = 0;
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);

  while (!Serial && ser_wait_cnt < 10) {
    ser_wait_cnt++;
    ledToggle();
    delay(250);
    }
  x = -LOGO_WIDTH / 2;
  Serial.println(F("Serial OK"));

  Serial.println("Starting....");
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
    }
  else
    Serial.println(F("SSD1306 allocation OK!"));


  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  ledPulseTrain(1);
  Serial.println(F("render bmp00"));
  display.clearDisplay();
  display.drawBitmap(64, 16, logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
  display.display();
  delay(SETUP_DELAY);

  // ledPulseTrain(2);
  // Serial.println(F("render bmp0l"));
  // display.clearDisplay();
  // display.drawBitmap(64, 16, bmp0l, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
  // display.display();
  // delay(SETUP_DELAY);

  // ledPulseTrain(3);
  // Serial.println(F("render bmp02"));
  // display.clearDisplay();
  // display.drawBitmap(64, 16, bmp02, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
  // display.display();
  // delay(SETUP_DELAY);

  // ledPulseTrain(4);
  // Serial.println(F("render bmp03"));
  // display.clearDisplay();
  // display.drawBitmap(64, 16, bmp03, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
  // display.display();
  // delay(SETUP_DELAY);

  // ledPulseTrain(5);
  // Serial.println(F("render cross"));
  // display.clearDisplay();
  // display.drawBitmap(64, 16, cross, LOGO_WIDTH * 2, LOGO_HEIGHT * 2, SSD1306_WHITE);
  // display.display();
  // delay(SETUP_DELAY);

  ledPulseTrain(5);
  display.clearDisplay();
  display.display();
  delay(SETUP_DELAY);

  // stpr.setSpeed(70);

  }

//=================================================================================================
void taskSerOut() {
  String _tmpStr = "";
  if (iCount % 100) {
    _tmpStr = "--iC:";
    _tmpStr += iCount;
    _tmpStr += " azim:";
    _tmpStr += azim;
    _tmpStr += " x:";
    _tmpStr += x;
    _tmpStr += " d:";
    _tmpStr += dir;
    _tmpStr += " def_count:";
    _tmpStr += def_count;
    // _tmpStr += " dir_dn_count:";
    // _tmpStr += dir_dn_count;
    // _tmpStr += " dir_up_count:";
    // _tmpStr += dir_up_count;
    Serial.println(_tmpStr);
    }
  }


//=================================================================================================
void renderOledE_compass() {
  // static int dir = 0;
  // static int x0 = 0;

  // static int lLim = -LOGO_WIDTH/2;
  // static int rLim = display.width() - (LOGO_WIDTH/2);


  switch (dir) {

      case 1:
        display.fillRect(x, COMPASS_Y, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_BLACK);
        azim++;
        x = azim_to_x();
        display.drawBitmap(x, COMPASS_Y, diamond, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
        dir_up_count++;
        break;

      case -1:
        display.fillRect(x, COMPASS_Y, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_BLACK);
        azim--;
        x = azim_to_x();
        display.drawBitmap(x, COMPASS_Y, diamond, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
        dir_dn_count++;
        break;

      default:
      case 0:
        // display.drawBitmap(x, COMPASS_Y, diamond, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
        def_count++;
        break;
    }



  if (azim > 359) {
    azim = 359;
    dir = -1;
    }

  if (azim < 0) {
    azim = 0;
    x = azim_to_x();;
    dir = 1;
    }


  }

//=================================================================================================
void taskOledOut() {
  renderOledE_compass();
  }

//=================================================================================================
void loop() {

  if (iCount == 30) {
    // stpr.step(512);

    dir = 1;
  }

  taskOledOut();
  taskSerOut();

  display.display();

  iCount++;
  delay(100);


  }