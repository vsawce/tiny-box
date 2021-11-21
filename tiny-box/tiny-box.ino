#include <EEPROM.h>
#include <DS3231.h>
#include <TM1637.h>

//Keep at 0 unless you want to reset EEPROM
#define EEPROM_BURN 0

//Each DS3231 module is different, batteries disconnected sometimes. Starts 1/1/2000 as per factory
const uint32_t DS3231_OFFSET = 946684800;

//Jules & I
const uint32_t START_DATE_EPOCH = 1631166840;

const uint32_t CURR_DATE_EPOCH = 1637493459;

const uint8_t UPLOAD_PADDING = 30;

//Update to force correction. Make sure to reset EEPROM to 0
const uint32_t FORCED_OFFSET = CURR_DATE_EPOCH + UPLOAD_PADDING - START_DATE_EPOCH;

const uint16_t EEPROM_ADDR = 0;

uint32_t last_elapsed_epoch;
uint32_t start_timestamp = 0;
/*
//If battery ever disconnected, to recalibrate put # days since start date.
//DEFAULT = START_DATE_EPOCH, since time will only get larger
const uint32_t CURRENT_DATE_EPOCH = START_DATE_EPOCH; 

const uint32_t START_DAY_OFFSET = CURRENT_DATE_EPOCH - START_DATE_EPOCH;

//Time between START DATE and 1/1/2000
const uint32_t COMP_EPOCH = START_DATE_EPOCH - DS3231_OFFSET + START_DAY_OFFSET;
*/
int CLK = 3;
int DIO = 1;

TM1637 tm(CLK, DIO);

//SCL 2 and SDA 0
RTClib rtc;

DateTime curr_dt;

void eeprom_write32(uint16_t eeprom_addr, uint32_t eeprom_val) {
  EEPROM.write(eeprom_addr, eeprom_val & 0xFF);
  EEPROM.write(eeprom_addr+1, (eeprom_val >> 8)  & 0xFF);
  EEPROM.write(eeprom_addr+2, (eeprom_val >> 16) & 0xFF);
  EEPROM.write(eeprom_addr+3, (eeprom_val >> 24) & 0xFF);
}

uint32_t eeprom_read32(uint16_t eeprom_addr) {
  return (uint32_t)EEPROM.read(eeprom_addr) |
         ((uint32_t)EEPROM.read(eeprom_addr + 1) << 8) |
         ((uint32_t)EEPROM.read(eeprom_addr + 2) << 16) |
         ((uint32_t)EEPROM.read(eeprom_addr + 3) << 24);
}

void display_number(uint32_t num) {   
    tm.display(3, num % 10);   
    tm.display(2, num / 10 % 10);   
    tm.display(1, num / 100 % 10);   
    tm.display(0, num / 1000 % 10);
}

void setup() {
  
  tm.init();

  tm.set(3);

  Wire.begin();

  //Code to burn 10 (reset) to EEPROM
  #if (EEPROM_BURN)
    eeprom_write32(EEPROM_ADDR, FORCED_OFFSET);
  #endif

  curr_dt = rtc.now();
  start_timestamp = curr_dt.unixtime();
  //Has RTC been reset?
 
  last_elapsed_epoch = eeprom_read32(EEPROM_ADDR);
  //else rely on current RTC time
}

//uint32_t cnt = 0;

void loop() {

  /*
  uint32_t start_date_offset, comp_epoch, curr_dt_epoch;

  //If battery ever disconnected, to recalibrate put # days since start date.
  //DEFAULT = START_DATE_EPOCH, since time will only get larger
  current_date_epoch = eeprom_read32(EEPROM_ADDR); 
  
  start_date_offset = current_date_epoch - START_DATE_EPOCH;
  
  //Time between START DATE and 1/1/2000
  comp_epoch = START_DATE_EPOCH - DS3231_OFFSET + start_date_offset;
  
  curr_dt = rtc.now();

  curr_dt_epoch = (curr_dt.unixtime()+comp_epoch)-START_DATE_EPOCH;
  */

  uint32_t curr_rtc_epoch, total_elapsed, days_since;
  

  //Current time - base time to get # seconds passed
  curr_dt = rtc.now();
  curr_rtc_epoch = curr_dt.unixtime();
  total_elapsed = last_elapsed_epoch + curr_rtc_epoch - DS3231_OFFSET;


  #if (!EEPROM_BURN)
    eeprom_write32(EEPROM_ADDR, total_elapsed);
  #endif
  
  days_since = total_elapsed/86400;

  display_number(days_since);
  //display_number((curr_dt.unixtime()+comp_epoch)-START_DATE_EPOCH);
  delay(1000);
}
