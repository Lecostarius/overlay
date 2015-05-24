/*
  Arduino library for MAX7456 video overlay IC

  based on code from Arduino forum members dfraser and zitron
  http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1220054359
  modified/extended by kg4wsv
  gmail: kg4wsv
*/

#include <Arduino.h>
#include "MAX7456.h"


// Constructor
MAX7456::MAX7456() {
  _slave_select = MAX7456SELECT;
  _char_attributes = 0x02;
  _cursor_x = CURSOR_X_MIN;
  _cursor_y = CURSOR_Y_MIN;
}


 
 /* -----------------------------------------------------------------------------
  Private functions - not meant to be used by the user:
   - MAX7456_spi_transfer
   - writeCharLinepos
 ------------------------------------------------------------------------------ */
 
// basic SPI transfer: use the Atmega hardware to send and receive one byte
// over SPI. MAX7456_spi_transfer does NOT set chip select, so it is a bit of
// a misnomer: it will do SPI data transfer with whatever SPI device is connected
// and which has its CS set active.
byte MAX7456::MAX7456_spi_transfer(volatile char data) {
  MAX7456_previous_SPCR = SPCR;  // save SPCR, so we play nice with other SPI peripherals
  SPCR = MAX7456_SPCR;  // set SPCR to what we need
  
  SPDR = data;                    // Start the transmission
  while (!(SPSR & (1<<SPIF))) ;   // Wait the end of the transmission
  SPCR = MAX7456_previous_SPCR;
  return SPDR;                    // return the received byte
}
void MAX7456::writeCharLinepos(uint8_t c, uint16_t linepos) {
  Poke(DMM_WRITE_ADDR, _char_attributes | 0x40); // enter 8 bit mode, no increment mode
  Poke(DMAH_WRITE_ADDR, linepos>>8); // As linepos cannot be larger than 480, this will clear bit 1, which means we write character index and not the attributes
  Poke(DMAL_WRITE_ADDR, linepos&0xFF);
  Poke(DMDI_WRITE_ADDR, c);
}

 /* -----------------------------------------------------------------------------
  Functions related to initialization of the MAX7456, basic settings, hardware
  related stuff...
   - Poke
   - Peek
   - begin
   - reset
   - initialize
   - offset
 ------------------------------------------------------------------------------ */
void MAX7456::Poke(byte adress, byte data) {
  digitalWrite(MAX7456SELECT,LOW); 
  MAX7456_spi_transfer(adress);
  MAX7456_spi_transfer(data); 
  digitalWrite(MAX7456SELECT,HIGH);
}

byte MAX7456::Peek(byte adress) {
  byte retval=0;
  digitalWrite(MAX7456SELECT,LOW); 
  MAX7456_spi_transfer(adress);
  retval=MAX7456_spi_transfer(0xff);
  digitalWrite(MAX7456SELECT,HIGH);
  return(retval);
}

void MAX7456::begin(byte slave_select) {
  _slave_select = slave_select;
  begin();
}

// do a soft reset of the MAX7456
void MAX7456::reset() {
  Poke(VM0_WRITE_ADDR, MAX7456_reset); // soft reset
  delay(1); // datasheet: after 100 us, STAT[6] can be polled to verify that the reset process is complete
  while (Peek(0xA0) & (1<<6)) ; // wait for RESET bit to be cleared
  initialize();
}

// initialize the default parameters for the MAX7456
// your personal preferences go here
void MAX7456::initialize() {
  Poke(DMM_WRITE_ADDR, 0x40 | _char_attributes); // 8 bit operation mode, default for attribute bits is all off, dont clear memory, no auto increment mode
  // set basic mode: enable, PAL, Sync mode, ...
  Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE|VIDEO_MODE_PAL|SYNC_MODE_AUTO);
  // set more basic modes: background mode brightness, blinking time, blinking duty cycle:
  Poke(VM1_WRITE_ADDR, BLINK_DUTY_CYCLE_50_50 | BACKGROUND_BRIGHTNESS_21);
  // set all rows to same character white level, 90%
  for (int x = 0; x < MAX_screen_rows; x++) {
    Poke(x+0x10, WHITE_level_90);
  }
}  
  
void MAX7456::begin() {
  uint8_t x;
  // uint8_t spi_junk;
  
  pinMode(_slave_select,OUTPUT);
  digitalWrite(_slave_select,HIGH); //disable device
  pinMode(MAX7456_DATAOUT, OUTPUT);
  pinMode(MAX7456_DATAIN, INPUT);
  pinMode(MAX7456_SCK,OUTPUT);

  MAX7456_SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(0<<SPR0);
  // configure SPI device on the microcontroller
  // SPIF - SPI interrupt flag
  // WCOL - write collision flag
  // SPI2X - double speed SPI flag
  // SPR1,0 - clock divider (we have a 16 MHz clock on our ATmega). If 00, divide by 4. If 01, by 16. If 10, by 64. If 11, by 128.
  MAX7456_previous_SPCR = SPCR;  // save SPCR, so we play nice with other SPI peripherals
  SPCR = MAX7456_SPCR;
  SPSR = (0<<SPIF)|(0<<WCOL)|(0<<SPI2X);
  //spi_junk=SPSR;spi_junk=SPDR;delay(25); // do we really need that? TK TODO
  
  // now configure the MAX7456
  reset();
  
  // we are done, restore SPI interface in case other peripherals are using it too
  SPCR = MAX7456_previous_SPCR;   // restore SPCR
}  



// Adjust the horizontal and vertical offet
// Horizontal offset between -32 and +31
// Vertical offset between -15 and +16
void MAX7456::offset(int horizontal, int vertical) {
  //Constrain horizontal between -32 and +31
  if (horizontal < -32) horizontal = -32;
  if (horizontal > 31)  horizontal = 31;
   
  //Constrain vertical between -15 and +16
  if (vertical < -15) vertical = -15;
  if (vertical > 16)  vertical = 16;

  // Write new offsets to the OSD
  Poke(HOS_WRITE_ADDR,horizontal);
  Poke(VOS_WRITE_ADDR,vertical);
 
}
 /* -----------------------------------------------------------------------------
  Cursor setting functions
   - clear
   - home
   - advanceCursor
   - setCursor
 ------------------------------------------------------------------------------ */

void MAX7456::clear() {
  Poke(DMM_WRITE_ADDR,CLEAR_display);
  home();
  while(Peek(DMM_READ_ADDR) & CLEAR_display) ; // wait until operation is completed and bit is set to zero again
}

// send the cursor to home
void MAX7456::home() {
  setCursor(CURSOR_X_MIN, CURSOR_Y_MIN);
} 
// send the cursor to position (x,y)
void MAX7456::setCursor(uint8_t x, uint8_t y) {
  if (x > CURSOR_X_MAX) x = CURSOR_X_MAX;
  if (y > CURSOR_Y_MAX) y = CURSOR_Y_MAX;
  _cursor_y = y; _cursor_x = x;
}
void MAX7456::advanceCursor() {
  if (++_cursor_x >= CURSOR_X_MAX) {
    if (++_cursor_y >= CURSOR_Y_MAX) _cursor_y = CURSOR_Y_MIN;
    _cursor_x = CURSOR_X_MIN;
  }
}


/* --------------------------------------------------------------------------
   Single character printing primitives
   - writeChar
   - writeCharWithAttributes
   - writeCharXY
   --------------------------------------------------------------------------- */
   

void MAX7456::writeCharXY(uint8_t c, uint8_t x, uint8_t y) {
  setCursor(x,y); 
  writeChar(c); 
}  

void MAX7456::writeChar(uint8_t c) {
  writeCharLinepos(c, _cursor_y * 30 + _cursor_x);
  advanceCursor(); // compute next cursor position
} 
 
void MAX7456::writeCharWithAttributes(uint8_t c, uint8_t attributes) {
  uint16_t linepos = _cursor_y * 30 + _cursor_x; // convert x,y to line position
  writeCharLinepos(c, linepos);
  Poke(DMAH_WRITE_ADDR, 0x02 | linepos>>8);
  Poke(DMAL_WRITE_ADDR, linepos&0xFF);
  Poke(DMDI_WRITE_ADDR, attributes);
  advanceCursor();
} 

// writeString is faster than a sequence of writeChar(). It is slightly slower for
//  strings of 1 char length, but 5 SPI transfers per character faster generally. Note that the string must
//  be null-terminated and must not contain the character 0xFF (this is a restriction of the MAX7456). 
//  This function works nicely if CURSOR_X_MIN is zero. If CURSOR_X_MIN is nonzero, any overwrapping write
//  will start at x=0 and not at x=CURSOR_X_MIN, so make sure that the string you print is not longer
//  than the rest of the line. 
void MAX7456::writeString(const char c[]) {
  uint16_t i=0;
  uint16_t linepos = _cursor_y * 30 + _cursor_x; // convert x,y to line position
  Poke(DMAH_WRITE_ADDR, linepos>>8); // As linepos cannot be larger than 480, this will clear bit 1, which means we write character index and not the attributes
  Poke(DMAL_WRITE_ADDR, linepos&0xFF);
  
  Poke(DMM_WRITE_ADDR, _char_attributes | 0x01); // enter 16 bit mode and auto increment mode
  
  // the i<480 is for safety, if the user gives us a string without zero at the end
  while(c[i] != 0 && i < 12) {
    digitalWrite(MAX7456SELECT,LOW); MAX7456_spi_transfer(c[i]); digitalWrite(MAX7456SELECT,HIGH);
    advanceCursor();
    i++;
  }
  // send 0xFF to end the auto-increment mode
  digitalWrite(MAX7456SELECT,LOW); MAX7456_spi_transfer(0xFF); digitalWrite(MAX7456SELECT,HIGH);
  
  Poke(DMM_WRITE_ADDR, _char_attributes | 0x40);   // back to 8 bit mode
}
 
// basic, slow writeString method. Honors CURSOR_X_MIN. 
void MAX7456::writeStringSlow(const char c[]) {
  uint16_t i=0;
  while(c[i] != 0 && i < 480) {
    writeChar(c[i]);
    i++;
  }
}
  



/* the following functions set the default mode bits for incremental mode printing of the MAX7456. */ 
void MAX7456::blink(byte onoff) {
  if (onoff) {
      _char_attributes |= 0x10;
  } else {
      _char_attributes &= ~0x10;
  }
}

void MAX7456::invert(byte onoff) {
  if (onoff) {
      _char_attributes |= 0x08;
  } else {
      _char_attributes &= ~0x08;
  }
}

// Read one character from character memory (x=0..29, y=0..12 (NTSC) or 0..15 (PAL))
byte MAX7456::ReadDisplay(uint16_t x, uint16_t y) {
    byte c;   
    uint16_t linepos = y * 30 + x; // convert x,y to line position
    
    Poke(DMM_WRITE_ADDR,0x40); // 8 bit mode
    Poke(DMAH_WRITE_ADDR, linepos >> 8); // DMAH bit 1 cleared since linepos is <480
    Poke(DMAL_WRITE_ADDR, linepos & 0xFF); 
    c=Peek(DMDO_READ_ADDR);
    return(c);
}
  
  






