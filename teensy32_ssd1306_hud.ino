
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <stdio.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <led_pulse_train.h>
#include <Adafruit_NeoPixel.h>
#include <gfx_bitmaps.h>
#include <serialPrint.h>
#include <neopixel_effects.h>

#define EEPROM_SIZE     255



#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET      -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES       10 // Number of snowflakes in the animation example
#define NPXL_PIN        2
#define NUM_NEOPIXELS        16

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


#define NPX_ROT_DIR_STOP        0
#define NPX_ROT_DIR_CCW         1
#define NPX_ROT_DIR_CW          2

#define DEF_NPX_INC     1
#define DEF_NPX_AMP_MIN 0
#define DEF_NPX_AMP_MAX 31

#define QUARTER_PI float(PI / 4.0)
// #define RAD_TO_DEG float(180/PI)
// #define DEG_TO_RAD float(PI/80)


enum eeprom_registers {
  EE_REG_NEOPIXEL_MODE,
  EE_REG_R_MAX,
  EE_REG_G_MAX,
  EE_REG_B_MAX,
  EE_REG_R_MIN,
  EE_REG_G_MIN,
  EE_REG_B_MIN,
  EE_REG_R_INT,
  EE_REG_G_INT,
  EE_REG_B_INT,
  NUM_EEPROM_REG,
}

uint8_t eeprom[EEPROM_SIZE] = {
uint8_t eeprom_live[EEPROM_SIZE] = {
  0,//---EE_REG_NEOPIXEL_MODE
  DEF_NPX_AMP_MAX, //---EE_REG_R_MAX
  DEF_NPX_AMP_MAX, //---EE_REG_G_MAX
  DEF_NPX_AMP_MAX, //---EE_REG_B_MAX
  DEF_NPX_AMP_MIN, //---EE_REG_R_MIN
  DEF_NPX_AMP_MIN, //---EE_REG_G_MIN
  DEF_NPX_AMP_MIN, //---EE_REG_B_MIN
  1, //---EE_REG_R_INT
  2, //---EE_REG_G_INT
  3, //---EE_REG_B_INT
};
char inByteBuffer[NUM_BYTES] = {};

int x;
int dir;
int npxlMode = 0;
uint16_t idx = 0;
uint16_t npxlIdx00 = 0;
uint16_t npxlIdx01 = 0;
uint16_t npxlIdx02 = 0;
uint8_t npxl_rotation_dir = true;

int def_count = 0;
int dir_up_count = 0;
int dir_dn_count = 0;
int azim = 0;
int rangeR = 32;
int rangeG = 32;
int rangeB = 32;

float incR = 2;
float incG = 4;
float incB = 8;
uint8_t r = 0;
uint8_t b = 0;
uint8_t g = 0;
float rRads = 0;

String inStr = "";

int64_t iCount = 0;

Adafruit_NeoPixel strip(NUM_NEOPIXELS, NPXL_PIN, NEO_GRB + NEO_KHZ800);
bool nxplEn = true;

neopixel_color npxR;
neopixel_color npxG;
neopixel_color npxB;


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
void readEEPROM() {
  serPrntNL("readEEPROM()");
  String tmpStr;
  int byte_read_cnt = 0;
  for (int i = 0; i < EEPROM_SIZE; i++) {
    eeprom[i] = EEPROM.read(i);
    memcpy(eeprom_live, eeprom, sizeof(eeprom_live));
    tmpStr = "readEEPROM()[";
    tmpStr += i;
    tmpStr += "]:";
    tmpStr += eeprom[i];
    byte_read_cnt++;
    serPrntNL(tmpStr);
  }

  serPrntNL();
  tmpStr = "Read ";
  tmpStr += byte_read_cnt;
  tmpStr += " of ";
  tmpStr += EEPROM_SIZE;
  tmpStr += " bytes from EEPROM";
  serPrntNL(tmpStr);
  }

//=================================================================================================
void writeEEPROM() {
  serPrntNL("writeEEPROM()");
  String tmpStr;
  int byte_write_cnt = 0;


  for (int i = 0; i < EEPROM_SIZE; i++) {

    byte_write_cnt += writeEEPROM(uint8_t nIdx);
    delay(5);
  }

  if(byte_write_cnt > 0) {
    EEPROM.commit();

    tmpStr = "Wrote ";
    tmpStr += byte_write_cnt;
    tmpStr += " of ";
    tmpStr += EEPROM_SIZE;
    tmpStr += " bytes from EEPROM";
  }
  else {
    tmpStr = "No changes to EEPROM";
  }
  serPrntNL(tmpStr);
  }

//=================================================================================================
int writeEEPROM(uint8_t nIdx) {
  serPrntNL("writeEEPROM(nIdx)");
  String tmpStr;
  int byte_write_cnt = 0;
  if (eeprom_live[nIdx] != eeprom[nIdx]) {
    EEPROM.write(nIdx, eeprom[nIdx);
    tmpStr = "Register changed: writeEEPROM[";
    tmpStr += nIdx;
    tmpStr += "]:";
    tmpStr += eeprom[nIdx];
    serPrntNL(tmpStr);
    delay(5);
    EEPROM.commit();
    return 1;
  }
  else {
    serPrntNL("No change to register");
    return 0;
  }
}


//=================================================================================================
void setup() {
  int ser_wait_cnt = 0;
  pinMode(LED_BUILTIN, OUTPUT);

  strip.begin();
  // strip.clear();
  strip.setPixelColor(0, eeprom[EE_REG_R_MAX], 0, 0);
  strip.setPixelColor(8, 0, eeprom[EE_REG_R_MAX], 0);
  strip.setPixelColor(15, 0, 0, eeprom[EE_REG_R_MAX]);
  strip.show();

  npxR = neopixel_color(eeprom[EE_REG_R_INT], eeprom[EE_REG_R_MAX]);
  npxG = neopixel_color(eeprom[EE_REG_R_INT], eeprom[EE_REG_G_MAX]);
  npxB = neopixel_color(eeprom[EE_REG_R_INT], eeprom[EE_REG_B_MAX]);

  Serial.begin(9600);

  while (!Serial && ser_wait_cnt < 10) {
    ser_wait_cnt++;
    ledToggle();
    delay(250);
  }
  x = -LOGO_WIDTH / 2;
  serPrntNL("Serial OK");

  serPrntNL("Starting....");
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    serPrntNL("SSD1306 allocation failed");
    for (;;);
    }
  else
    serPrntNL("SSD1306 allocation OK!");

  appendBootLogS("Init EEPROM");
  EEPROM.begin(512);
  appendBootLogS("EEPROM started");

  appendBootLogS("Read EEPROM contents");
  readEEPROM();


  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  ledPulseTrain(1);
  serPrntNL("render bmp00");
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
  uint8_t tmpInt = 0;


  if (iCount % 100) {

    tmpInt = (r + g + b)/3;
    _tmpStr = "--iC:";
    _tmpStr += iCount;
    // _tmpStr += " azim:";
    // _tmpStr += azim;
    // _tmpStr += " x:";
    // _tmpStr += x;
    _tmpStr += " npxlMode:";
    _tmpStr += npxlMode;

    _tmpStr += " incR:";
    _tmpStr += incR;

    // _tmpStr += " rangeR:";
    // _tmpStr += rangeR;

    _tmpStr += "  incG:";
    _tmpStr += incG;

    // _tmpStr += " rangeG:";
    // _tmpStr += rangeG;

    _tmpStr += " incB:";
    _tmpStr += incB;

    // _tmpStr += " rangeB:";
    // _tmpStr += rangeB;


    // _tmpStr += " idx:";
    // _tmpStr += idx;


    // _tmpStr += " npxlIdx00:";
    // _tmpStr += npxlIdx00;
    // _tmpStr += " npxlIdx01:";
    // _tmpStr += npxlIdx01;
    // _tmpStr += " npxlIdx02:";
    // _tmpStr += npxlIdx02;


    _tmpStr += " r:";
    _tmpStr += r;
    _tmpStr += " b:";
    _tmpStr += b;
    _tmpStr += " g:";
    _tmpStr += g;
    _tmpStr += " tmpInt:";
    _tmpStr += tmpInt;

    serPrntNL(_tmpStr);
    }
  }

//-----------------------------------------------------------------------------
void paramSetHandler(String nCmd, String nParamName, int& nParam, int nVal, int nUpLim, int nLoLim) {
  if(inStr == nCmd) {
    serPrntNL(nCmd + ": set " + nParamName);
    nParam = nVal;

    if (nParam > nUpLim)
      nParam = nUpLim;
    else if (nParam < nLoLim)
      nParam = nLoLim;
  }
}
//-----------------------------------------------------------------------------
void paramIncHandler(String nCmd, String nParamName, int& nParam, int nInc, int nUpLim, int nLoLim) {
  if(inStr == nCmd) {
    serPrntNL(nCmd + ": decrement " + nParamName);
    nParam += nInc;

    if (nParam > nUpLim)
      nParam = nUpLim;
    else if (nParam < nLoLim)
      nParam = nLoLim;
  }
}

//-----------------------------------------------------------------------------
void paramIncHandler(String nCmd, String nParamName, float& nParam, float nInc, float nUpLim, float nLoLim) {
  if(inStr == nCmd) {
    serPrntNL(nCmd + ": increment " + nParamName);
    nParam += nInc;

    if (nParam > nUpLim)
      nParam = nUpLim;
    else if (nParam < nLoLim)
      nParam = nLoLim;
  }
}

//-----------------------------------------------------------------------------
void taskHandleSerIn() {
  if (recvWithEndMarker() > "") {

    paramIncHandler("nm+", "next neopixel mode", npxlMode, 1, 5, 0);
    paramIncHandler("nm-", "next neopixel mode", npxlMode, -1, 5, 0);

    paramIncHandler("rR+"," range", rangeR, 1, 255, 0);
    paramIncHandler("rR-","  range", rangeR, -1, 255, 0);
    paramIncHandler("rR++"," range", rangeR, 5, 255, 0);
    paramIncHandler("rR--","  range", rangeR, -5, 255, 0);

    paramIncHandler("rG+"," range", rangeG, 1, 255, 0);
    paramIncHandler("rG-","  range", rangeG, -1, 255, 0);
    paramIncHandler("rG++"," range", rangeG, 5, 255, 0);
    paramIncHandler("rG--","  range", rangeG, -5, 255, 0);

    paramIncHandler("rB+"," range", rangeB, 1, 255, 0);
    paramIncHandler("rB-","  range", rangeB, -1, 255, 0);
    paramIncHandler("rB++"," range", rangeB, 5, 255, 0);
    paramIncHandler("rB--","  range", rangeB, -5, 255, 0);

    paramIncHandler("ir+"," increment", incR, .1, 64, 0);
    paramIncHandler("ir-","  increment", incR, -.1, 64, 0);

    paramIncHandler("ir++"," increment", incR, 1, 64, 0);
    paramIncHandler("ir--","  increment", incR, -1, 64, 0);

    paramIncHandler("ig+"," increment", incG, .1, 64, 0);
    paramIncHandler("ig-","  increment", incG, -.1, 64, 0);

    paramIncHandler("ig++"," increment", incG, 1, 64, 0);
    paramIncHandler("ig--","  increment", incG, -1, 64, 0);

    paramIncHandler("ib+"," increment", incB, .1, 64, 0);
    paramIncHandler("ib-","  increment", incB, -.1, 64, 0);

    paramIncHandler("ib++"," increment", incB, 1, 64, 0);
    paramIncHandler("ib--","  increment", incB, -1, 64, 0);


    paramSetHandler("rrmx", "max range", rangeR, 255, 255, 1);
    paramSetHandler("rrmd", "mid range", rangeR, 127, 255, 1);
    paramSetHandler("rrmn", "min range", rangeR, 1, 255, 1);

    paramSetHandler("rgmx", "max range", rangeG, 255, 255, 1);
    paramSetHandler("rgmd", "mid range", rangeG, 127, 255, 1);
    paramSetHandler("rgmn", "min range", rangeG, 1, 255, 1);

    paramSetHandler("rbmx", "max range", rangeB, 255, 255, 1);
    paramSetHandler("rbmd", "mid range", rangeB, 127, 255, 1);
    paramSetHandler("rbmn", "min range", rangeB, 1, 255, 1);

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
      serPrntNL("n+: neopixel rotation CCW");
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
void taskNpxl_red_breath() {
  uint8_t rTmp;
  uint8_t gTmp;
  uint8_t bTmp;
  float rTmpFloat;
  float gTmpFloat;
  float bTmpFloat;

  static bool npxlEnShadow = false;

  rTmp = npxR.npcLedSine(incR, rangeR);
  gTmp = npxG.npcLedSine(incG, rangeG);
  bTmp = npxB.npcLedSine(incB, rangeB);

  // r = rTmp;
  // g = gTmp;
  // b = bTmp;


  if(nxplEn) {
    strip.clear();
    for(int i = 0; i < NUM_NEOPIXELS; i++) {
      // rTmpFloat = 10;
      // rTmpFloat *= i/16.0;
      // rTmpFloat *= 2 * PI;
      // rTmpFloat = sin(rTmpFloat);

      // rTmpFloat *= 64;
      strip.setPixelColor(i, rTmp, gTmp, bTmp);

    }
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
    dir = 1;
  }

  if(iCount % 1 == 0)
    taskNpxl_red_breath();

  taskOledOut();
  taskSerOut();

  display.display();

  iCount++;
  delay(10);


  }