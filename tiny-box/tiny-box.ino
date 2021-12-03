#include <DS3231.h>
#include <TM1637.h>

const uint16_t LOOP_TIME = 10000;
const int8_t TIMEZONE_CONVERT_PST = -8; //PST = GMT-8

const uint8_t NUM_7SEG = 4;
const uint8_t DAY_STARTHOUR  = 7;  //7AM
const uint8_t EVENING_STARTHOUR     = 17; //5PM
const uint8_t NIGHT_STARTHOUR       = 22; //10PM
const uint8_t DAY_BRIGHTNESS        = 6; //Bright
const uint8_t EVENING_BRIGHTNESS    = 3; //Normal
const uint8_t NIGHT_BRIGHTNESS      = 2; //Dim
const int8_t F_SEG = 0x71;  //0b01110001
const int8_t o_SEG = 0x5C;  //0b01011100
const int8_t r_SEG = 0x50;  //0b01010000
const int8_t J_SEG = 0x0E;  //0b00001110
const int8_t A_SEG = 0x77;  //0b01110111
const int8_t n_SEG = 0x54;  //0b01010100
const int8_t d_SEG = 0x5E;  //0b01011110
const int8_t V_SEG = 0x3E;  //0b00111110

//Each DS3231 module is different, batteries disconnected sometimes. Starts 1/1/2000 as per factory
const uint32_t DS3231_OFFSET = 946684800;

//Jules & I (or target date to count away from)
const uint32_t START_DATE_EPOCH = 1631166840;
//Current epoch date. Replace 3v battery (reset RTC) every time this is updated and flashed
const uint32_t LAST_DATE_EPOCH = 1638522583;

//Testing
//const uint32_t START_DATE_EPOCH = 1637749313;
//const uint32_t LAST_DATE_EPOCH = 1637749313;

const uint8_t UPLOAD_PADDING = 30;

//Update to force correction. Make sure to reset EEPROM to 0
const uint32_t FORCED_OFFSET = LAST_DATE_EPOCH + UPLOAD_PADDING - START_DATE_EPOCH;

uint32_t last_elapsed_epoch;
uint32_t start_timestamp = 0;

//TM1637 Seven-Segment SPI pins
int CLK = 3;
int DIO = 1;

TM1637 tm(CLK, DIO);

//DS3231 I2C is SCL pin 2 and SDA pin 0
RTClib rtc;

DateTime curr_dt;

void displayNoCoding(uint8_t BitAddr,int8_t DispData)
{
  tm.start();          //start signal sent to TM1637 from MCU
  tm.writeByte(ADDR_FIXED);//
  tm.stop();           //
  tm.start();          //
  tm.writeByte(BitAddr|0xc0);//
  tm.writeByte(DispData);//
  tm.stop();            //
  tm.start();          //
  tm.writeByte(tm.Cmd_DispCtrl);//
  tm.stop();           //
}

void display_number(uint32_t num) {

    uint16_t ten_scale = 1000;

    for (uint8_t i = 0; i < NUM_7SEG; i++) {
      if(num/ten_scale) {
        tm.display(i, num / ten_scale % 10);
      }
      else {
        tm.display(i, ' ');
      }
      ten_scale /= 10;
    }
}

void timeBrightnessAdjust(uint32_t currentEpoch) {
  int currentHour = ((currentEpoch + (TIMEZONE_CONVERT_PST*3600)) % 86400) / 3600;

  //Currently night
  if (currentHour >= NIGHT_STARTHOUR || currentHour < DAY_STARTHOUR){
    tm.set(NIGHT_BRIGHTNESS);
    //tm.display(3, 2);   //Debug
  }
  //Currently evening
  else if (currentHour >= EVENING_STARTHOUR && currentHour < NIGHT_STARTHOUR) {
    tm.set(EVENING_BRIGHTNESS);
    //tm.display(3, 1);   //Debug
  }
  //Currently day
  else {
    tm.set(DAY_BRIGHTNESS);
    //tm.display(3, 0);   //Debug
  }
}

void setup() {
  
  tm.init();

  tm.set(3);

  Wire.begin();

  curr_dt = rtc.now();
  start_timestamp = curr_dt.unixtime();

  /*
  //Test isualize brightness
  displayNoCoding(1, 0b0111111);  
  displayNoCoding(2, 0b0111111);  
  displayNoCoding(3, 0b0111111);
  while(1) {
    for (uint8_t i = 1; i < 8; i++) {
      tm.set(i);
      tm.display(0,i);
      delay(1000);
    }
  }
  */
  
  /*
  //Test time adjustment
  while(1) {
    //Day test
    timeBrightnessAdjust(1638374400); // 12/1 8AM PST
    delay(1000);
    //Evening test
    timeBrightnessAdjust(1638417600); // 12/1 8PM PST
    delay(1000);
    //Night test
    timeBrightnessAdjust(1638428400); // 12/1 11PM PST
    delay(1000);
  }
  */
  

  //Wake sequence
  displayNoCoding(0, F_SEG);
  displayNoCoding(1, o_SEG);
  displayNoCoding(2, r_SEG);
  displayNoCoding(3, J_SEG);
  
  delay(2000);
  tm.clearDisplay();

  displayNoCoding(0, A_SEG);
  displayNoCoding(1, n_SEG);
  displayNoCoding(2, d_SEG);
  displayNoCoding(3, V_SEG);

  delay(2000);
  tm.clearDisplay();
}

//uint32_t cnt = 0;

void loop() {

  uint32_t curr_rtc_epoch, curr_total_epoch, total_elapsed, days_since;
  

  //Current time - base time to get # seconds passed
  curr_dt = rtc.now();
  curr_rtc_epoch = curr_dt.unixtime();
  curr_total_epoch = curr_rtc_epoch - DS3231_OFFSET + LAST_DATE_EPOCH; //Seconds elapsed by RTC + last compilation epoch
  total_elapsed = curr_rtc_epoch + FORCED_OFFSET - DS3231_OFFSET;

  days_since = total_elapsed/86400;

  timeBrightnessAdjust(curr_total_epoch);
  display_number(days_since);
  
  delay(LOOP_TIME);
}
