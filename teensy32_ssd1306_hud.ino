
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

#define DIR_DOWN        -1
#define DIR_NONE        0
#define DIR_UP          1


#define SER_WAIT_TICKS  10
#define COMPASS_Y       0




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

static const unsigned char PROGMEM up_arrow[] = {
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

static const unsigned char PROGMEM up_arrow_l[] = {
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

static const unsigned char PROGMEM up_arrow_r[] = {
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

static const unsigned char PROGMEM star[] =
{
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


int x;
int dir;
int def_count = 0;
int dir_up_count = 0;
int dir_dn_count = 0;
int64_t iCount = 0;


//=================================================================================================
void ledToggle() {
  static bool bit = false;
  bit = !bit;
  digitalWrite(LED_BUILTIN, bit);
}

//=================================================================================================
void setup() {
  int ser_wait_cnt = 0;
  pinMode(LED_BUILTIN, OUTPUT);
  ledToggle();

  Serial.begin(9600);

  while(!Serial && ser_wait_cnt < 10){
    ser_wait_cnt++;
    ledToggle();
    delay(250);
  }
  x = -LOGO_WIDTH / 2;
  Serial.println(F("Serial OK"));

  Serial.println("Starting...");
  ledToggle();
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
    }
  else
    Serial.println(F("SSD1306 allocation OK!"));

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  ledToggle();
  delay(1000);

  ledToggle();
  display.clearDisplay();

  display.display();
  delay(1000);
  ledToggle();

}

//=================================================================================================
void taskSerOut() {
  String _tmpStr = "";
  if(iCount % 100) {
    _tmpStr = "iC:";
    _tmpStr += iCount;
    _tmpStr += " x:";
    _tmpStr += x;
    _tmpStr += " d:";
    _tmpStr += dir;
    _tmpStr += " def_count:";
    _tmpStr += def_count;
    _tmpStr += " dir_dn_count:";
    _tmpStr += dir_dn_count;
    _tmpStr += " dir_up_count:";
    _tmpStr += dir_up_count;
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
      x++;
      display.drawBitmap(x, COMPASS_Y, diamond, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
      dir_up_count++;
      break;

    case -1:
      display.fillRect(x, COMPASS_Y, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_BLACK);
      x--;
      display.drawBitmap(x, COMPASS_Y, diamond, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
      dir_dn_count++;
      break;

    default:
    case 0:
      display.drawBitmap(x, COMPASS_Y, diamond, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
      def_count++;
      break;
  }



  if (x > LOGO_X_MAX) {
    x = LOGO_X_MAX;
    dir = -1;
  }

  if(x < LOGO_X_MIN) {
    x = LOGO_X_MIN;
    dir = 1;
  }


}

//=================================================================================================
void taskOledOut() {
  renderOledE_compass();
}

//=================================================================================================
void loop() {

  if(iCount == 30)
    dir = 1;

  taskOledOut();
  taskSerOut();

  display.display();
  iCount++;
  delay(100);


}