#include "MAX7456.h"



int led=13;
int green=30;
int yellow=31;

MAX7456 *mx = new MAX7456();

#define MAX7456_DATAOUT 51//MOSI, PB2
#define MAX7456_DATAIN  50//MISO, PB3
#define MAX7456_SCK  52//sck, PB1
#define MAX7456SELECT 9//pin 9 (one of the motor pwm, used for octo only)

// this is a copy of the character memory of the MAX7456  
byte charmem[64];

void receiveMCM() {
#define COLNUM 64 // could be 54 or 64 depending on format, we need only 54
#define BUFSIZE 15
  char buffer[BUFSIZE];
  
  uint8_t bufX=0; // how full is the input buffer? points to the next available free position
  uint8_t lineX=0; // line in char (0..63, or 0..53)
  uint16_t charX=0; // character index (0..255)
  
  uint8_t readState=0; // 0 - waiting for the "MAX7456" string at the beginning, 1 - reading lines
  int incomingByte;
  while (charX < 256) {
  if (Serial.available() > 0) {
    incomingByte = Serial.read(); 
    if (incomingByte != 13) {
      buffer[bufX++] = incomingByte & 0xFF;
      if (bufX >= BUFSIZE) bufX = BUFSIZE - 1;
    } else {
      // got an entire line... process it!
      if (readState == 0) {
        if (strcmp("MAX7456",buffer) == 0) readState = 1;
      } else {
        // got a data line. Parse into a 1-byte value "c":
        char c = 0;
        for (int i=7; i >= 0; i--) if (buffer[i] = '1') c |= (1<<i);
        // now we have c
        //charmem[lineX] = c;
        
        Serial.print(lineX); Serial.print(" "); Serial.println(charX);
        if (++lineX >= COLNUM) { lineX=0; charX++; }
      }
      
    } // if incomingByte != 13, else part
  } // if Serial.available() > 0
  } // while charX > 256
}

// write a set of 54 character bytes into position "charIdx" into the NVM memory
void writeCharMem(byte *characterBytes, int charIdx) {
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // disable OSD
  while (mx->Peek(0xA0) & (1<<5)); // wait until STAT[5] is 0
  mx->Poke(CMAH_WRITE_ADDR,(charIdx & 0xFF));
  for (int t=0; t < 54; t++) {
    mx->Poke(CMAL_WRITE_ADDR, (t & 0xFF));
    mx->Poke(CMDI_WRITE_ADDR, characterBytes[t]);
  }
  mx->Poke(CMM_WRITE_ADDR, 0b10100000); // write from shadow RAM into NVM
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // enable OSD again
}

// read data from NVM character number "charX" into characterData[0..53]
void getNVMchar(byte *characterBytes, int charX) {
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // disable OSD
  while (mx->Peek(0xA0) & (1<<5)); // wait until STAT[5] is 0
  mx->Poke(CMAH_WRITE_ADDR,charX);
  mx->Poke(CMM_WRITE_ADDR, 0b01010000); // read from NVM into shadow RAM
  while (mx->Peek(CMM_READ_ADDR) != 0) ; // wait until command is finished
  while (mx->Peek(0xA0) & (1<<5)); // wait until STAT[5] is 0
  for (int i=0; i<54; i++) {
    mx->Poke(CMAL_WRITE_ADDR,(i&0xff));
    characterBytes[i] = mx->Peek(0xCF);
  }
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // enable OSD again
}

void printMCM() {
  byte c;
  char puf[9];
  // STAT[5] must be 0, VM0[3] must be 0:
  Serial.println("MAX7456");
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // disable OSD
  while (mx->Peek(0xA0) & (1<<5)); // wait until STAT[5] is 0
  for (int t=0; t < 256; t++) {
    mx->Poke(CMAH_WRITE_ADDR,t);
    mx->Poke(CMM_WRITE_ADDR, 0b01010000); // read from NVM into shadow RAM
    while (mx->Peek(CMM_READ_ADDR) != 0) ; // wait until command is finished
    while (mx->Peek(0xA0) & (1<<5)); // wait until STAT[5] is 0
    //Serial.println(t);
    for (int i=0; i<54; i++) {
      mx->Poke(CMAL_WRITE_ADDR,(i&0xff));
      c = mx->Peek(0xCF);
      for (int bt=7; bt >=0; bt--) {
        if (c & (1<<bt)) Serial.print("1"); else Serial.print("0");
      }
      Serial.println(); 
    }
  }
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // enable OSD again
  //Serial.print("Done with testOp. Result is "); Serial.println(c);
}

void testOp2() {
  // try writing into shadow RAM
  byte c;
    
  mx->Poke(DMM_WRITE_ADDR, 0x40); // 8 bit mode
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // disable OSD
  mx->Poke(CMAH_WRITE_ADDR,0);
  mx->Poke(CMAL_WRITE_ADDR,0);
  mx->Poke(CMDI_WRITE_ADDR,0x55);
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // enable OSD again

  Serial.print("Done writing to shadow RAM.\n");

  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // disable OSD
  mx->Poke(CMAH_WRITE_ADDR,0);
  mx->Poke(CMAL_WRITE_ADDR,0);
  c=mx->Peek(CMDO_READ_ADDR);
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // enable OSD again
  mx->Poke(DMM_WRITE_ADDR,0); // 16 bit mode
  
  Serial.print("Read data from memory (must be 0x55): 0x"); Serial.print(c,HEX); Serial.println();
    
}
#ifdef UNDEF
void SPI_MasterInit() {
//  pinMode(MAX7456SELECT, OUTPUT); digitalWrite(MAX7456SELECT, HIGH); delay(10);
//  pinMode(MAX7456_DATAOUT, OUTPUT);
//  pinMode(MAX7456_DATAIN, INPUT);
//  pinMode(MAX7456_SCK, OUTPUT);
  pinMode(SS,OUTPUT); // to avoid that we can be switched back into Slave mode. 
  // This pin is not used, but may not be INPUT and then LOW. See datasheet pp 197ff
  
  // SPcontrolRegister = 
  // SPIE = SPI interrupt enable
  // SPE  = SPI enable
  // DORD = Data order (1=LSB first)
  // MSTR = microcontroller is Master(1)/Slave(0)
  // CPOL = Clock polarity (1 means, SCK is high when idle). For the MAX7456, SCK is low when idle, so this must be 0.
  // CPHA = clock phase (complex, see p 197 ff in datasheet). Since we want to sample on the rising edge, this must be 0 if CPOL is 0.
  // SPR1, SPR0 - speed divider:
  // SPR1  SPR0  divider  @16Mhz
  //  0      0      4      4 MHz
  //  0      1     16      1 MHz
  //  1      0     64      250 kHz
  //  1      1    256      62.5 kHz
  
  SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(1<<SPR0);
  // SPIF - SPI interrupt flag
  // WCOL - write collision flag
  // SPI2X - double speed SPI flag
  SPSR = (0<<SPIF)|(0<<WCOL)|(0<<SPI2X);
  
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("overlay code started\n");

  //SPI_MasterInit();
  pinMode(SS,OUTPUT); // to avoid that we can be switched back into Slave mode. This pin is not used, but may not be INPUT and then LOW. See datasheet pp 197ff
  
  pinMode(green,OUTPUT); pinMode(yellow,OUTPUT);pinMode(led, OUTPUT);    

  mx->begin();
  mx->offset(31,16); // hori=-32...31, vert=-15..16. void MAX7456::offset(int horizontal, int vertical)
  mx->home();
  for (uint16_t c=0; c<256;c++) {
    mx->writeChar(c&0xff);
  }    
  mx->writeChar0(5,0xc0);
  
  
  char charA[128];
  delay(100);
  Serial.println("********** DISPLAY **********");
  for (int y=0; y < 10; y++) {
    for (int x=0; x < 20; x++) {
      Serial.print(mx->ReadDisplay(x,y)); Serial.print(" ");
    }
    Serial.println();
  }
  
  
  //testOp();
  //testOp2();
  Serial.print("MAX7456 >");
  
  //mx->read_character(0x42, charA); // first param: which char to read
  //for (int i=0; i < 32; i++) {
  //  Serial.print("char "); Serial.print(i); Serial.print(" is "); Serial.println((int)charA[i]);
  //}
}


void loop() {
  byte c;
  int x;
  int incomingByte;
  
  if (Serial.available() > 0) {
    incomingByte = Serial.read(); Serial.println();
    switch(incomingByte) {  // wait for commands
    
      case 'D': // download font
        printMCM(); break;
      case 'r': // reset
        mx->reset();
        Serial.println("Soft reset executed"); break;
      case 's': // show charset
        Serial.println("Sorry not yet implemented"); break;
      case 'f': // show font
        mx->show_font(); Serial.println("Font printed"); break;
      case '?': // read status
        Serial.print("Status byte (in binary): "); Serial.println(mx->Peek(0xA0),BIN); break;
      case 'w': // write string
        while (incomingByte != 13) { // CR ends the sequence
          if (Serial.available() > 0) {
            incomingByte = Serial.read(); mx->writeChar(incomingByte & 0xFF);
          }
        }
        break;
      case 'b': // burn into NVM
        getNVMchar(charmem, 1);
        for (int i=0; i < 54; i++) {Serial.print(charmem[i]); Serial.print(",");}
        Serial.println();
        writeCharMem(charmem, 0);
        Serial.println("Copied char 1 to char 0");
        getNVMchar(charmem,0);
        for (int i=0; i < 54; i++) {Serial.print(charmem[i]); Serial.print(",");}
        Serial.println();
        break;
      default:
        Serial.println("invalid command");
      break;
    }
    Serial.print("MAX7456>");
  }
  
  //Serial.println("Loop!");
  // put your main code here, to run repeatedly: 
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  //digitalWrite(MAX7456SELECT,HIGH);
  //digitalWrite(MAX7456_SCK,HIGH);
  //digitalWrite(MAX7456_DATAOUT,HIGH);
  delay(250);               // wait for a second
#ifdef UNDEFINED 
  SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(1<<SPR0);
  delay(1);
  digitalWrite(MAX7456SELECT,LOW); 
  MAX7456_spi_transfer(0x80); // read status register
  SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(1<<CPHA)|(0<<SPR1)|(1<<SPR0);
  c = MAX7456_spi_transfer(0x00); // dont care what I send, this is reading
  SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(1<<SPR0);
  digitalWrite(MAX7456SELECT,HIGH);
  if (c==0) { digitalWrite(yellow,HIGH); } else { digitalWrite(yellow,LOW); }
  //Serial.print("0x80=");Serial.println(c);
  
  digitalWrite(MAX7456SELECT,LOW); 
  MAX7456_spi_transfer(0x81); // read status register
  SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(1<<CPHA)|(0<<SPR1)|(1<<SPR0);
  c = MAX7456_spi_transfer(0x00); // dont care what I send, this is reading
  SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(0<<SPR1)|(1<<SPR0);
  digitalWrite(MAX7456SELECT,HIGH);
  //Serial.print("0x81=");Serial.println(c);
  Serial.println();


  
  for ( x=0x80; x < 0xA1; x++) {
    c = mx->Peek(x); Serial.print(x,HEX); Serial.print(":"); Serial.print(c,BIN); Serial.print(",");
  }
  x=0xEC; c = mx->Peek(x); Serial.print(x,HEX); Serial.print(":"); Serial.print(c,BIN); Serial.print(",");
  x=0xB0; c = mx->Peek(x); Serial.print(x,HEX); Serial.print(":"); Serial.print(c,BIN); Serial.print(",");
  x=0xC0; c = mx->Peek(x); Serial.print(x,HEX); Serial.print(":"); Serial.print(c,BIN); Serial.print(",");
 
  c = mx->Peek(0xA0);
  Serial.print("stat=");Serial.println(c);
#endif   
  digitalWrite(led,LOW);
  //digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  //digitalWrite(MAX7456SELECT,LOW);
  //digitalWrite(MAX7456_SCK,LOW);
  //digitalWrite(MAX7456_DATAOUT,LOW);

  delay(250);               // wait for a second
  
  //mx->write_to_screen("Hallo",1,1);

}
