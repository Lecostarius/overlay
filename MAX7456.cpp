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
  _char_attributes = 0x01;
  _cursor_x = CURSOR_X_MIN;
  _cursor_y = CURSOR_Y_MIN;
}

byte MAX7456::ReadDisplay(uint16_t x, uint16_t y) {
    byte c;   
    uint16_t linepos = y * 30 + x; // convert x,y to line position
    
    Poke(DMM_WRITE_ADDR,0x40); // 8 bit mode
    Poke(DMAH_WRITE_ADDR, linepos >> 8); // DMAH bit 1 cleared since linepos is <480
    Poke(DMAL_WRITE_ADDR, linepos & 0xFF); 
    c=Peek(DMDO_READ_ADDR);
    return(c);
}
  
  
void MAX7456::Poke(byte adress, byte data) {
  digitalWrite(MAX7456SELECT,LOW); 
  MAX7456_spi_transfer(adress);
  MAX7456_spi_transfer(data); 
  digitalWrite(MAX7456SELECT,HIGH);
  //delay(1);
}
byte MAX7456::Peek(byte adress) {
  byte retval=0;
  //delay(1);
  digitalWrite(MAX7456SELECT,LOW); 
  //delay(1);
  MAX7456_spi_transfer(adress);
  //delay(1);
  retval=MAX7456_spi_transfer(0xff);
  //delay(1);
  digitalWrite(MAX7456SELECT,HIGH);
  //delay(1);
  return(retval);
}
    

byte MAX7456::MAX7456_spi_transfer(volatile char data) {
  MAX7456_previous_SPCR = SPCR;  // save SPCR, so we play nice with other SPI peripherals
  SPCR = MAX7456_SPCR;  // set SPCR to what we need
  
  SPDR = data;                    // Start the transmission
  while (!(SPSR & (1<<SPIF))) ;   // Wait the end of the transmission
  SPCR = MAX7456_previous_SPCR;
  return SPDR;                    // return the received byte
}


void MAX7456::begin(byte slave_select)
{
  _slave_select = slave_select;
  begin();
}

void MAX7456::begin() {
  uint8_t x, spi_junk;
  
  pinMode(_slave_select,OUTPUT);
  digitalWrite(_slave_select,HIGH); //disable device
  pinMode(MAX7456_DATAOUT, OUTPUT);
  pinMode(MAX7456_DATAIN, INPUT);
  pinMode(MAX7456_SCK,OUTPUT);
  
  // configure SPI device on the microcontroller
  MAX7456_previous_SPCR = SPCR;  // save SPCR, so we play nice with other SPI peripherals
  MAX7456_SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(1<<SPR0);
  // SPIF - SPI interrupt flag
  // WCOL - write collision flag
  // SPI2X - double speed SPI flag
  SPCR = MAX7456_SPCR;
  SPSR = (0<<SPIF)|(0<<WCOL)|(0<<SPI2X);
  spi_junk=SPSR;spi_junk=SPDR;delay(25); // do we really need that? TK TODO
  
  
  // now configure the MAX7456
  Poke(VM0_WRITE_ADDR, MAX7456_reset); // soft reset
  delay(500);
  Poke(DMM_WRITE_ADDR, 0x40); // 8 bit operation mode, default for attribute bits is all off, dont clear memory, no auto increment mode
  // set basic mode: enable, PAL, Sync mode, ...
  Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE|VIDEO_MODE_PAL|SYNC_MODE_AUTO);
  // set more basic modes: background mode brightness, blinking time, blinking duty cycle:
  Poke(VM1_WRITE_ADDR, BLINK_DUTY_CYCLE_50_50);
  
  // set all rows to same character white level, 90%
  for (x = 0; x < MAX_screen_rows; x++) {
    Poke(x+0x10, WHITE_level_90);
  }
  
  //delay(1);
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

void MAX7456::clear() {
  Poke(DMM_WRITE_ADDR,CLEAR_display);
  home();
  while(Peek(DMM_READ_ADDR) & CLEAR_display) ; // wait until operation is completed and bit is set to zero again
}

// send the cursor to home
void MAX7456::home() {
  _cursor_x = CURSOR_X_MIN;
  _cursor_y = CURSOR_Y_MIN;
} 


size_t MAX7456::write(uint8_t c) {
  write_0(c);
  //writeChar(c);
  return(0);
}

void MAX7456::writeChar(uint8_t c) {
  uint16_t linepos = _cursor_y * 30 + _cursor_x; // convert x,y to line position
  // compute next cursor position
  if (++_cursor_x >= CURSOR_X_MAX) {
    if (++_cursor_y >= CURSOR_Y_MAX) _cursor_y = CURSOR_Y_MIN;
    _cursor_x = CURSOR_X_MIN;
  }
  Poke(DMM_WRITE_ADDR, 0x40); // enter 8 bit mode, no increment mode
  Poke(DMAH_WRITE_ADDR, linepos>>8); // As linepos cannot be larger than 480, this will clear bit 1, which means we write character index and not the attributes
  Poke(DMAL_WRITE_ADDR, linepos&0xFF);
  Poke(DMDI_WRITE_ADDR, c);

} 
 
void MAX7456::writeChar0(uint8_t c, uint8_t attributes) {
  uint16_t linepos = _cursor_y * 30 + _cursor_x; // convert x,y to line position
  // compute next cursor position
  if (++_cursor_x >= CURSOR_X_MAX) {
    if (++_cursor_y >= CURSOR_Y_MAX) _cursor_y = CURSOR_Y_MIN;
    _cursor_x = CURSOR_X_MIN;
  }
  Poke(DMM_WRITE_ADDR, 0x40); // enter 8 bit mode, no increment mode
  Poke(DMAH_WRITE_ADDR, linepos>>8); // As linepos cannot be larger than 480, this will clear bit 1, which means we write character index and not the attributes
  Poke(DMAL_WRITE_ADDR, linepos&0xFF);
  Poke(DMDI_WRITE_ADDR, c);

  Poke(DMAH_WRITE_ADDR, 0x02 | linepos>>8);
  Poke(DMAL_WRITE_ADDR, linepos&0xFF);
  Poke(DMDI_WRITE_ADDR, attributes);
} 
  
// this is probably inefficient, as i simply modified a more general function
// that wrote arbitrary length strings. need to check modes of writing
// characters to MAX7456 to see if there's a better way to write one at a time
void MAX7456::write_0(uint8_t c)
{
  unsigned int linepos;
  byte char_address_hi, char_address_lo;

  if (c == '\n')
    {
      _cursor_y++;
      if (_cursor_y > CURSOR_Y_MAX)
	_cursor_y = CURSOR_Y_MIN;
      _cursor_x = CURSOR_X_MIN;
      return;
    }

  if (c == '\r')
    {
      _cursor_x = CURSOR_X_MIN;
      return;
    }

  // To print non-ascii character, this line must either be commented out
  // or convert_ascii() needs to be modified to make sure it doesn't conflict
  // with the special characters being printed.
  c = convert_ascii(c);

  MAX7456_previous_SPCR = SPCR;  // save SPCR, so we play nice with other SPI peripherals
  SPCR = MAX7456_SPCR;  // set SPCR to what we need

  char_address_hi = 0;
  char_address_lo = 0;
    
  // convert x,y to line position
  linepos = _cursor_y * 30 + _cursor_x;
  _cursor_x++;
  if (_cursor_x >= CURSOR_X_MAX)
    {
      _cursor_y++;
      if (_cursor_y > CURSOR_Y_MAX)
	_cursor_y = CURSOR_Y_MIN;
      _cursor_x = CURSOR_X_MIN;
    }

  
  // divide in to hi & lo byte
  char_address_hi = linepos >> 8;
  char_address_lo = linepos;
  
  
  digitalWrite(_slave_select,LOW);

  MAX7456_spi_transfer(DMM_WRITE_ADDR); //dmm
  MAX7456_spi_transfer(_char_attributes);

  MAX7456_spi_transfer(DMAH_WRITE_ADDR); // set start address high
  MAX7456_spi_transfer(char_address_hi);

  MAX7456_spi_transfer(DMAL_WRITE_ADDR); // set start address low
  MAX7456_spi_transfer(char_address_lo);
  
  
  MAX7456_spi_transfer(DMDI_WRITE_ADDR);
  MAX7456_spi_transfer(c);
  
  MAX7456_spi_transfer(DMDI_WRITE_ADDR);
  MAX7456_spi_transfer(END_string);
  
  MAX7456_spi_transfer(DMM_WRITE_ADDR); //dmm
  MAX7456_spi_transfer(B00000000);

  digitalWrite(_slave_select,HIGH);

  SPCR = MAX7456_previous_SPCR;   // restore SPCR
  
}


void MAX7456::write_to_screen(char s[], byte x, byte y) {
  write_to_screen(s, x, y, 0, 0);
}

void MAX7456::write_to_screen(char s[], byte line) {
  write_to_screen(s, 1, line, 0, 0);
}


void MAX7456::write_to_screen(char s[], byte x, byte y, byte blink, byte invert){
  unsigned int linepos;
  byte local_count;
  byte settings, char_address_hi, char_address_lo;
  byte screen_char;


  MAX7456_previous_SPCR = SPCR;  // save SPCR, so we play nice with other SPI peripherals
  SPCR = MAX7456_SPCR;  // set SPCR to what we need

  local_count = 0;

  char_address_hi = 0;
  char_address_lo = 0;
    
  // convert x,y to line position
  linepos = y*30+x;
  
  // divide in to hi & lo byte
  char_address_hi = linepos >> 8;
  char_address_lo = linepos;
  
  
  settings = B00000001;
  
  // set blink bit
  if (blink) {
    settings |= (1 << 4);       // forces nth bit of x to be 1.  all other bits left alone.
    //x &= ~(1 << n);      // forces nth bit of x to be 0.  all other bits left alone.  
  }
  // set invert bit
  if (invert){
    settings |= (1 << 3);       // forces nth bit of x to be 1.  all other bits left alone.
  }

  
  digitalWrite(_slave_select,LOW);

  MAX7456_spi_transfer(DMM_WRITE_ADDR); //dmm
  MAX7456_spi_transfer(settings);

  MAX7456_spi_transfer(DMAH_WRITE_ADDR); // set start address high
  MAX7456_spi_transfer(char_address_hi);

  MAX7456_spi_transfer(DMAL_WRITE_ADDR); // set start address low
  MAX7456_spi_transfer(char_address_lo);
  
  
  while(s[local_count]!='\0') // write out full screen
  {
    screen_char = convert_ascii(s[local_count]);
    MAX7456_spi_transfer(DMDI_WRITE_ADDR);
    MAX7456_spi_transfer(screen_char);
    local_count++;
  }
  
  MAX7456_spi_transfer(DMDI_WRITE_ADDR);
  MAX7456_spi_transfer(END_string);
  
  MAX7456_spi_transfer(DMM_WRITE_ADDR); //dmm
  MAX7456_spi_transfer(B00000000);

  digitalWrite(_slave_select,HIGH);

  SPCR = MAX7456_previous_SPCR;   // restore SPCR
} 


void MAX7456::blink(byte onoff)
{
  if (onoff)
    {
      _char_attributes |= 0x10;
    }
  else
    {
      _char_attributes &= ~0x10;
    }
}

void MAX7456::blink()
{
  blink(1);
}

void MAX7456::noBlink()
{
  blink(0);
}


void MAX7456::invert(byte onoff)
{
  if (onoff)
    {
      _char_attributes |= 0x08;
    }
  else
    {
      _char_attributes &= ~0x08;
    }
}

void MAX7456::invert()
{
  invert(1);
}

void MAX7456::noInvert()
{
  invert(0);
}

// Returns all 0s for some reason
void MAX7456::read_character(byte addr, char character[]) 
{
  // Enable the SPI
  // Write VM0[3] = 0 to disable the OSD image.
  Serial.println("Enable SPI, disable OSD output (section)");

  MAX7456_previous_SPCR = SPCR;  // save SPCR, so we play nice with other SPI peripherals
  SPCR = MAX7456_SPCR;  // set SPCR to what we need

  digitalWrite(_slave_select, LOW);
  Serial.println("Enable SPI");
  MAX7456_spi_transfer(VM0_WRITE_ADDR);
  Serial.println("Sent VM0_WRITE_ADDR");
  MAX7456_spi_transfer(0x00|VERTICAL_SYNC_NEXT_VSYNC); // double check that the other bits can be zero
  delay(100); // Not sure if this delay is needed. It's used in the init code
  
  Serial.println("Select character by address");
  // Write CMAH[7:0]  = xxH  to select the character (0â€“255) to be read
  MAX7456_spi_transfer(CMAH_WRITE_ADDR);
  MAX7456_spi_transfer(addr);

  Serial.println("Read character data from NVRAM to Shadow RAM");
  // Write CMM[7:0] = 0101xxxx to read the character data from the NVM to the shadow RAM
  MAX7456_spi_transfer(CMM_WRITE_ADDR);
  MAX7456_spi_transfer(0b01010000); // Double check that bits 0-3 can be zero
  
  char test[64];

  for(int i = 0; i < 54; i++)
  {
    // Write CMAL[7:0] = xxH to select the 4-pixel byte (0â€“63) in the character to be read
    MAX7456_spi_transfer(CMAL_WRITE_ADDR);
    MAX7456_spi_transfer(i);

    // Read CMDO[7:0] = xxH to read the selected 4-pixel byte of data
    test[i] = MAX7456_spi_transfer(CMDO_READ_ADDR);
    character[i] = MAX7456_spi_transfer(i);
  }

  // Write VM0[3] = 1 to enable the OSD image display.
  MAX7456_spi_transfer(VM0_WRITE_ADDR);
  MAX7456_spi_transfer(VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE);
  
  digitalWrite(_slave_select, HIGH);
  SPCR = MAX7456_previous_SPCR;   // restore SPCR
  Serial.println("Done reading. Data collected.");
  
  Serial.println("Character: ");
  for(int i = 0; i < 54; i++) {
    Serial.print("0x");
    Serial.print(character[i], HEX);
    Serial.print(",");
  }
  Serial.println();
  Serial.println("Test: ");
  for(int i = 0; i < 54; i++) {
    Serial.print("0x");
    Serial.print(test[i], HEX);
    Serial.print(",");
  }
  Serial.println();
  
}


void MAX7456::write_character(byte addr, char character[]) 
{
  // Enable the SPI
  // Write VM0[3] = 0 to disable the OSD image.
  Serial.println("Enable SPI, disable OSD output (section)");

  MAX7456_previous_SPCR = SPCR;  // save SPCR, so we play nice with other SPI peripherals
  SPCR = MAX7456_SPCR;  // set SPCR to what we need

  digitalWrite(_slave_select, LOW);
  MAX7456_spi_transfer(VM0_WRITE_ADDR);
  MAX7456_spi_transfer(0x00|VERTICAL_SYNC_NEXT_VSYNC); // double check that the other bits can be zero
  
  // Write CMAH[7:0]  = xxH  to select the character (0â€“255) to be read
  MAX7456_spi_transfer(CMAH_WRITE_ADDR);
  MAX7456_spi_transfer(addr);

  for(int i = 0; i < 54; i++)
  {
    // Write CMAL[7:0] = xxH to select the 4-pixel byte (0â€“63) in the character to be read
    MAX7456_spi_transfer(CMAL_WRITE_ADDR);
    MAX7456_spi_transfer(i);
    
    // Write CMDI[7:0] = xxH to set the pixel values of the selected part of the character
    MAX7456_spi_transfer(CMDI_WRITE_ADDR);
    MAX7456_spi_transfer(character[i]);
  }

  // Write CMM[7:0] = 1010xxxx to write the character data from the shadow RAM to the NVRAM
  MAX7456_spi_transfer(CMM_WRITE_ADDR);
  MAX7456_spi_transfer(0b10100000); // Double check that bits 0-3 can be zero
  
  // Wait at least 12ms to finish writing
  delay(150);

  // Write VM0[3] = 1 to enable the OSD image display.
  MAX7456_spi_transfer(VM0_WRITE_ADDR);
  MAX7456_spi_transfer(VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE);
  
  digitalWrite(_slave_select, HIGH);
  SPCR = MAX7456_previous_SPCR;   // restore SPCR
}

byte MAX7456::convert_ascii(int character) {
// for some reason the MAX7456 does not follow ascii letter
// placement, so you have to have this odd lookup table

  byte lookup_char;
  return(character & 0xff);
  if (character == 32)
    lookup_char = 0x00; // blank space
  else if (character == 48)
    lookup_char = 0x0a; // 0
  else if ((character > 48) && (character < 58))
    lookup_char = (character - 48); // 1-9
  else if ((character > 64) && (character < 90))
    lookup_char = (character - 54); // A-Z
  else if ((character > 96) && (character < 123))
    lookup_char = (character - 60); // a-z
  else if (character == 34)
    lookup_char = 0x48; // "
  else if (character == 39)
    lookup_char = 0x46; // '
  else if (character == 40)
    lookup_char = 0x3f; // (
  else if (character == 41)
    lookup_char = 0x40; // )
  else if (character == 44)
    lookup_char = 0x45; // ,
  else if (character == 45)
    lookup_char = 0x49; // -
  else if (character == 46)
    lookup_char = 0x41; // .
  else if (character == 47)
    lookup_char = 0x47; // /
  else if (character == 58)
    lookup_char = 0x44; // :
  else if (character == 59)
    lookup_char = 0x43; // ;
  else if (character == 60)
    lookup_char = 0x4a; // <
  else if (character == 62)
    lookup_char = 0x4b; // >
  else if (character == 63)
    lookup_char = 0x42; // ?
//  else
//    lookup_char = 0x00; // out of range, blank space

 return (lookup_char);
}

