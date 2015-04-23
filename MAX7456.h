/*
  Arduino library for MAX7456 video overlay IC

  based on code from Arduino forum members dfraser and zitron
  http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1220054359
  modified/extended by kg4wsv
  gmail: kg4wsv
*/


#ifndef MAX7456_h
#define MAX7456_h

#include <Arduino.h>
//#include <WConstants.h>

// this is for Arduino Uno
#define MAX7456_DATAOUT 11//MOSI
#define MAX7456_DATAIN  12//MISO
#define MAX7456_SCK  13//sck
#define MAX7456SELECT 10//ss (chip select)
#define MAX7456_VSYNC 5// INT0, not used

// this is for 2560
#define MAX7456_DATAOUT 51//MOSI
#define MAX7456_DATAIN  50//MISO
#define MAX7456_SCK  52//sck
#define MAX7456SELECT 9//pin 9 (one of the motor pwm, used for octo only)
//#define SS 53 // SS pin of the 2560

//MAX7456 register addresses
#define DMM_WRITE_ADDR   0x04
#define DMM_READ_ADDR   0x84
#define DMAH_WRITE_ADDR  0x05
#define DMAH_READ_ADDR 0x85
#define DMAL_WRITE_ADDR  0x06
#define DMAL_READ_ADDR 0x86
#define DMDI_WRITE_ADDR  0x07
#define VM0_WRITE_ADDR   0x00
#define VM0_READ_ADDR   0x80
#define VM1_WRITE_ADDR   0x01
#define CMAL_READ_ADDR  0x0a
#define CMAL_WRITE_ADDR  0x8a
#define HOS_READ_ADDR 0x82
#define VOS_READ_ADDR 0x83
#define HOS_WRITE_ADDR 0x02
#define VOS_WRITE_ADDR 0x03
#define CMAH_READ_ADDR 0x89
#define CMAH_WRITE_ADDR 0x09
#define CMM_READ_ADDR 0x88
#define CMM_WRITE_ADDR 0x08
// There is no CMDO write address
#define CMDO_READ_ADDR 0xC1
#define CMDI_READ_ADDR 0x8B
#define CMDI_WRITE_ADDR 0x0B
// There is no DMDO write adress
#define DMDO_READ_ADDR 0xB0

// video mode register 0 bits. Create the pattern by ORing the ones you need.
#define VIDEO_BUFFER_DISABLE 0x01
#define MAX7456_RESET 0x02
#define VERTICAL_SYNC_NEXT_VSYNC 0x04
#define OSD_ENABLE 0x08
#define SYNC_MODE_AUTO 0x00
#define SYNC_MODE_INTERNAL 0x30
#define SYNC_MODE_EXTERNAL 0x20
#define VIDEO_MODE_PAL 0x40
#define VIDEO_MODE_NTSC 0x00

// video mode register 1 bits
// duty cycle is on_off
#define BLINK_DUTY_CYCLE_50_50 0x00
#define BLINK_DUTY_CYCLE_33_66 0x01
#define BLINK_DUTY_CYCLE_25_75 0x02
#define BLINK_DUTY_CYCLE_75_25 0x03

// blinking time
#define BLINK_TIME_0 0x00
#define BLINK_TIME_1 0x04
#define BLINK_TIME_2 0x08
#define BLINK_TIME_3 0x0C

// background mode brightness (percent)
#define BACKGROUND_BRIGHTNESS_0 0x00
#define BACKGROUND_BRIGHTNESS_7 0x10
#define BACKGROUND_BRIGHTNESS_14 0x20
#define BACKGROUND_BRIGHTNESS_21 0x30
#define BACKGROUND_BRIGHTNESS_28 0x40
#define BACKGROUND_BRIGHTNESS_35 0x50
#define BACKGROUND_BRIGHTNESS_42 0x60
#define BACKGROUND_BRIGHTNESS_49 0x70

#define BACKGROUND_MODE_GRAY 0x80

//MAX7456 commands
#define CLEAR_display 0x04
#define CLEAR_display_vert 0x06
#define END_string 0xff


#define WHITE_level_80 0x03
#define WHITE_level_90 0x02
#define WHITE_level_100 0x01
#define WHITE_level_120 0x00

#define PAL_MODE

// with NTSC
#ifdef NTSC_MODE
#define MAX7456_reset 0x02
#define DISABLE_display 0x00
#define ENABLE_display 0x08
#define ENABLE_display_vert 0x0c
#define MAX_screen_size 390
#define MAX_screen_rows 13
#define CURSOR_X_MIN 2
#define CURSOR_X_MAX 29
#define CURSOR_Y_MIN 0
#define CURSOR_Y_MAX 13
#endif

// with PAL
#ifdef PAL_MODE
#define MAX7456_reset 0x42
#define DISABLE_display 0x40
#define ENABLE_display 0x48
#define ENABLE_display_vert 0x4c
#define MAX_screen_size 480
#define MAX_screen_rows 15
#define CURSOR_X_MIN 1
#define CURSOR_X_MAX 29
#define CURSOR_Y_MIN 0
#define CURSOR_Y_MAX 15
#endif

//class MAX7456 : public Print
class MAX7456 {
 public:
  MAX7456();
  byte MAX7456_spi_transfer(char data);
  byte MAX7456_spi_read(volatile char data);
  void Poke(byte adress, byte data);
  byte Peek(byte adress);
  byte ReadDisplay(uint16_t x, uint16_t y);
  void begin();
  void begin(byte slave_select);
  void write_to_screen(char s[], byte x, byte y, byte blink, byte invert);
  void write_to_screen(char s[], byte x, byte y);
  void write_to_screen(char s[], byte line);
  void write_0(uint8_t c);
  size_t write(uint8_t c);
  void writeChar(uint8_t c);
  void writeChar0(uint8_t c, uint8_t a);
  byte convert_ascii(int character);
  void offset(int horizontal, int vertical);
  void clear();
  void home();
  void blink(byte onoff);
  void blink();
  void noBlink();
  void invert(byte onoff);
  void invert();
  void noInvert();
  void read_character(byte addr, char character[]);
  void write_character(byte addr, char character[]);
 private:
  //  byte MAX7456_spi_transfer(unsigned char data);
  byte MAX7456_SPCR, MAX7456_previous_SPCR;
  byte _slave_select;
  byte _char_attributes;
  byte _cursor_x, _cursor_y;
};


#endif


