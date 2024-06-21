
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <led_pulse_train.h>
#include <Adafruit_NeoPixel.h>
#include <gfx_bitmaps.h>
#include <serialPrint.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES       10 // Number of snowflakes in the animation example
#define NPXL_PIN        2
#define NPXL_CNT        16

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

#define SETUP_DELAY     1000
#define NUM_BYTES       32


#define NXP_ROT_DIR_STOP        0
#define NXP_ROT_DIR_CCW         1
#define NXP_ROT_DIR_CW          2



char inByteBuffer[NUM_BYTES] = {};

int x;
int dir;
uint16_t idx = 0;
uint16_t npxlIdx00 = 0;
uint16_t npxlIdx01 = 0;
uint16_t npxlIdx02 = 0;
uint8_t npxl_rotation_dir = true;

int def_count = 0;
int dir_up_count = 0;
int dir_dn_count = 0;
int azim = 0;
uint8_t _r = 0;
uint8_t b = 0;
uint8_t g = 0;
String inStr = "";

int64_t iCount = 0;

Adafruit_NeoPixel strip(NPXL_CNT, NPXL_PIN, NEO_GRB + NEO_KHZ800);
bool nxplEn = true;


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

//-----------------------------------------------------------------------------
String recvWithEndMarker() {
  static int bCnt = 0;
  char endMarker = '\n';
  char rc;

  while (Serial.available() > 0) {
    rc = Serial.read();
    if (bCnt < NUM_BYTES - 1) {
      if (rc != endMarker) {
        inByteBuffer[bCnt] = rc;
        }
      bCnt++;
      }
    else
      serPrntNL("buffer overflow");
    }

  if (bCnt > 0) {
    serPrntVNL("Rx'ed ", bCnt, " bytes");
    inByteBuffer[bCnt] = '\0';
    inStr = inByteBuffer;
    }
  else
    inStr = "";

  bCnt = 0;

  return inStr;
  }

//=================================================================================================
void setup() {
  int ser_wait_cnt = 0;
  pinMode(LED_BUILTIN, OUTPUT);

  strip.begin();
  // strip.clear();
  strip.setPixelColor(0, 31, 0, 0);
  strip.setPixelColor(8, 0, 31, 0);
  // strip.setPixelColor(15, 0, 0, 31);
  strip.show();

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


  ledPulseTrain(5);
  display.clearDisplay();
  display.display();
  delay(SETUP_DELAY);

  }

//=================================================================================================
void taskSerOut() {
  String _tmpStr = "";
  if (iCount % 100) {
    _tmpStr = "--iC:";
    _tmpStr += iCount;
    // _tmpStr += " azim:";
    // _tmpStr += azim;
    // _tmpStr += " x:";
    // _tmpStr += x;

    _tmpStr += " idx:";
    _tmpStr += idx;

    _tmpStr += " npxlIdx00:";
    _tmpStr += npxlIdx00;
    _tmpStr += " npxlIdx01:";
    _tmpStr += npxlIdx01;
    _tmpStr += " npxlIdx02:";
    _tmpStr += npxlIdx02;

    _tmpStr += " _r:";
    _tmpStr += _r;
    _tmpStr += " b:";
    _tmpStr += b;
    _tmpStr += " g:";
    _tmpStr += g;
    _tmpStr += " d:";
    // _tmpStr += dir;
    // _tmpStr += " def_count:";
    // _tmpStr += def_count;
    // _tmpStr += " dir_dn_count:";
    // _tmpStr += dir_dn_count;
    // _tmpStr += " dir_up_count:";
    // _tmpStr += dir_up_count;
    Serial.println(_tmpStr);
    }
  }

//-----------------------------------------------------------------------------
void taskHandleSerIn() {
  if (recvWithEndMarker() > "") {
    if (inStr == "non") {
      nxplEn = true;
      serPrntNL("on: neopixel ring on");
    }

    else if (inStr == "noff") {
      nxplEn = false;
      serPrntNL("off: neopixel ring off");
    }

    else if (inStr == "n+") {
      npxl_rotation_dir = 2;
      serPrntNL("n-: neopixel rotation CCW");
    }
    else if (inStr == "n-") {
      npxl_rotation_dir = 1;
      serPrntNL("n-: neopixel rotation CCW");
    }

    else if (inStr == "n0") {
      npxl_rotation_dir = 0;
      serPrntNL("n-: neopixel rotation stop");

    }

    inStr = "";
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
void taskNpxl() {
  // static uint16_t _idx = 0;
  static bool npxlEnShadow = false;
  if(_r > 64)
     _r = 0;
  else
    _r++;


  if (npxl_rotation_dir == NXP_ROT_DIR_CW){
    idx++;
    if (idx >= NPXL_CNT) {
      idx = 0;
    }
  }
  else if(npxl_rotation_dir == NXP_ROT_DIR_CCW) {
    idx--;
    if(idx == 0)
      idx = NPXL_CNT - 1;
  }


  switch(idx) {
    default:
      serPrntNL("idx: default");
      npxlIdx00 = idx;
      npxlIdx01 = idx -1;
      npxlIdx02 = idx -2;
      break;

    case 65535:
      serPrntNL("idx: 65535");
      idx = NPXL_CNT - 1;
      npxlIdx00 = idx;
      npxlIdx01 = idx - 2;
      npxlIdx02 = idx - 3;
      break;

    case 0:
      serPrntNL("idx: 0");
      npxlIdx00 = idx;
      npxlIdx01 = 15;
      npxlIdx02 = 14;
      break;

    case 1:
      serPrntNL("idx: 1");
      npxlIdx00 = idx;
      npxlIdx01 = 0;
      npxlIdx02 = 15;
      break;
  }

  if(nxplEn) {
    strip.clear();
    strip.setPixelColor(npxlIdx00, 8, g, b);
    strip.setPixelColor(npxlIdx01, 64, g, b);
    strip.setPixelColor(npxlIdx02, 8, g, b);
    strip.show();
    }
  else if (npxlEnShadow != nxplEn) {
    strip.clear();
    strip.show();

  }
  npxlEnShadow = nxplEn;
}


//=================================================================================================
void loop() {

  taskHandleSerIn();


  if (iCount == 15) {
    // stpr.step(512);

    dir = 1;
  }

  if(iCount % 300 == 0)
    taskNpxl();

  taskOledOut();
  taskNpxl();
  taskSerOut();

  display.display();

  iCount++;
  delay(100);


  }