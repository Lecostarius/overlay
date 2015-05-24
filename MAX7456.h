/*
  Arduino library for MAX7456 video overlay IC

  based on code from Arduino forum members dfraser and zitron
  http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1220054359
  modified/extended by kg4wsv and Lecostarius
  gmail: kg4wsv
*/


#ifndef MAX7456_h
#define MAX7456_h

#include <Arduino.h>

// this is for Arduino Uno
//#define MAX7456_DATAOUT 11//MOSI
//#define MAX7456_DATAIN  12//MISO
//#define MAX7456_SCK  13//sck
//#define MAX7456SELECT 10//ss (chip select)
//#define MAX7456_VSYNC 5// INT0, not used

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
#define CMAL_READ_ADDR  0x8a
#define CMAL_WRITE_ADDR  0x0a
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
  MAX7456(); // Constructor
  
  void Poke(byte adress, byte data);    // write "data" into MAX7456's register with adress "adress". Always use Poke and Peek, dont use MAX7456_spi_transfer.
  byte Peek(byte adress);               // read from the MAX7456's register "adress"
  void reset();                         // make a soft reset of the MAX7456, wait until completed, return
  void initialize();                    // initialize default values of the MAX7456 like PAL mode, 16 bit mode, autoincrement, backgnd brightness...
  void begin();                         // initializer: call this once before using the MAX7456. Does the pinModes of the SPI, calls reset()...
  void begin(byte slave_select);        // initializer: set the slave_select pin not to MAX7456SELECT constant, but to the variable slave_select
  void offset(int horizontal, int vertical);  // set the horizontal (-32..31)/vertical (-16..15) offset in pixel. This is where the upper left corner is.

  void clear();                         // clears screen and sets cursor home
  void home();                          // sets cursor home (left upper corner)
  void setCursor(uint8_t x, uint8_t y); // sets cursor to (x,y)
  void advanceCursor();                 // advance cursor one position
  
  void writeCharXY(uint8_t c, uint8_t x, uint8_t y);   // writes character byte "c" to screen position x,y. Shortcut for "setCursor(); writeChar();".
  void writeChar(uint8_t c);            // write a char to current cursor position and move cursor 
  void writeCharWithAttributes(uint8_t c, uint8_t attributes);   // write one char and its attributes to current cursor position and move cursor

  void writeString(const char c[]);       // write a sequence of characters to current cursor position (and move cursor). For attributes, see 
                                          // the functions blink() and invert(). Use this whenever printing >1 character at a time, unless you are
                                          // printing wrapped lines AND have CURSOR_X_MIN set to nonzero. Note that this uses char instead of unsigned
                                          // char to allow constant strings ("hallo!")
  void writeStringSlow(const char c[]) ;  // like writeString(), but slower. Only advantage: honors CURSOR_X_MIN in line wrappings.
  
  byte ReadDisplay(uint16_t x, uint16_t y); // Read one character from character memory (x=0..29, y=0..12 (NTSC) or 0..15 (PAL))


  /* the following functions set the default mode bits for incremental mode printing of the MAX7456. */
  void blink(byte onoff);
  void invert(byte onoff);

 private:
  byte MAX7456_spi_transfer(char data); // shift 8 bit "data" via SPI to the MAX7456 and return its 8 bit response during the same 8 bit shift. Does not set chip select.
  void writeCharLinepos(uint8_t c, uint16_t linepos);  // writes character byte "c" to position "linepos" (0=left upper edge)
  
  byte MAX7456_SPCR, MAX7456_previous_SPCR; // store the desired and previous SPCR register of the Atmega 
  byte _slave_select; // Atmega pin that is connected to MAX7456 chip select
  byte _char_attributes; // standard character attributes if we do not set any
  byte _cursor_x, _cursor_y; // cursor position in character memory (x=0..29, y=0..15 or 0..12)
};


#endif


