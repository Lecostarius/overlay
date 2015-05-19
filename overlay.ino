#include "MAX7456.h"



int led=13;
int green=30;
int yellow=31;

MAX7456 *mx = new MAX7456();

#define MAX7456_DATAOUT 51//MOSI, PB2
#define MAX7456_DATAIN  50//MISO, PB3
#define MAX7456_SCK  52//sck, PB1
#define MAX7456SELECT 9//pin 9 (one of the motor pwm, used for octo only)
  
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
  
// this is a copy of the character memory of the MAX7456 (one character)  
byte charmem[64];

// receiving one 0 or 1 in ASCII takes at 115200 baud 10 bit or 1/11520 sec or 86 us
// receiving one bytes is 9 chars, so it takes 781 us. Receiving one set of
// 54 bytes (for one character) takes 42 ms. Serial.read() buffers up to 64 bytes.
// So we could read the data as it comes, and then, write it all via SPI away
// and flash it. The latter will take 54*9us+12ms or 12.5 ms. During this time,
// the serial line will receive 12.5ms/86us or 145 bytes, which is too much.
// Therefore, we must either reduce the serial speed to 1/4 (38400), or we have
// to write the NVM asynchronously (so let it write while we continue handling the
// serial).

void receiveMCM() {
#define COLNUM 64 // could be 54 or 64 depending on format, we need only 54
#define BUFSIZE 15
  char buffer[BUFSIZE+1];
  uint32_t charN=0;
  uint8_t bufX=0; // how full is the input buffer? points to the next available free position
  uint8_t lineX=0; // line in char (0..63, or 0..53)
  uint16_t charX=0; // character index (0..255)
  
  uint8_t readState=0; // 0 - waiting for the "MAX7456" string at the beginning, 1 - reading lines
  int incomingByte;
  Serial.println("receive- waiting for MCM file");
 
  
  while (charX < 256) {
  if (Serial.available() > 0) {
    incomingByte = Serial.read(); charN++;
    //if (++charN < 32) Serial.print(incomingByte);
    if (incomingByte != 13 && ( (lineX != COLNUM-1) || (charX != 255) || (bufX != 7) )) {
      buffer[bufX++] = incomingByte & 0xFF;
      if (bufX >= BUFSIZE) bufX = BUFSIZE - 1;
    } else {
      // got an entire line... process it!
      buffer[bufX] = 0; // string end
      bufX = 0;
      
      if (readState == 0) {
        //if (charN<32) {Serial.println();Serial.println(buffer);}
        if (strcmp(buffer,"MAX7456") == 0) readState = 1;
      } else {
        // got a data line. Parse into a 1-byte value "c":
        byte c = 0;
        for (int i=0; i <8; i++) if (buffer[i] == '1') c |= (1<<(7-i)); // the order is highest bit first, this is why we have to do 1<<(7-i)
        // now we have c
        charmem[lineX] = (c & 0xFF);
        if (charN < 256) {
          //Serial.print("charmem: "); Serial.print(buffer); Serial.print(", c="); Serial.println(c,BIN);
        }
        //Serial.print(lineX); Serial.print(" "); Serial.println(charX);
        if (++lineX >= COLNUM) { 
          // we have all the 54 bytes for this character.
          
          writeCharMemAsync(charmem, charX); // write to NVM. Takes about 500 us for SPI to shadow RAM, launches the NVM write, but does not wait for completion.
          Serial.print(charX); Serial.print(" ");
          lineX=0; charX++; 
        }
      }
      
    } // if incomingByte != 13, else part
  } // if Serial.available() > 0
  } // while charX > 256
  Serial.println("Done.");
}

// write a set of 54 character bytes into position "charIdx" into the NVM memory
// this is asynchronous, so we do NOT wait until the data is really written, but
// return immediately. This switches the OSD off, but not on again.
// It is recommended to use writeCharMem() which is the synchronous version, unless
// you cannot afford to wait the roughly 13 ms it takes the NVM to complete the write
// operation.
void writeCharMemAsync(byte *characterBytes, int charIdx) {
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // disable OSD
  while (mx->Peek(0xA0) & (1<<5)); // wait until STAT[5] is 0
  mx->Poke(CMAH_WRITE_ADDR,(charIdx & 0xFF));
  for (int t=0; t < 54; t++) {
    mx->Poke(CMAL_WRITE_ADDR, (t & 0xFF));
    mx->Poke(CMDI_WRITE_ADDR, characterBytes[t]);
  }
  mx->Poke(CMM_WRITE_ADDR, 0b10100000); // write from shadow RAM into NVM
}

void writeCharMem(byte *characterBytes, int charIdx) {
  writeCharMemAsync(characterBytes, charIdx);
  while (mx->Peek(0xA0) & (1<<5)); // wait until STAT[5] is 0
  mx->Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE|VIDEO_MODE_PAL|SYNC_MODE_AUTO); // enable OSD again
}

// read data from NVM character number "charX" into characterBytes[0..53]
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

// read all characters from NVM and print them to Serial in the format of Maxim's .mcm files
// note that the max7456 does not care whether the two bits that define one pixel are 11 or 01,
// both is transparent. However most of the character editors I found do care and incorrectly
// do not work on 11 pixels. So, printMCM01 takes care of that and will transform a 11 into a 01,
// while printMCM will print the original content of the NVM memory.
void printMCM() {
  Serial.println("MAX7456");
  for (int t=0; t < 256; t++) {
    getNVMchar(charmem, t);
    for (int i=0; i<COLNUM; i++) {
      for (int bt=7; bt >=0; bt--) {
        if (charmem[i] & (1<<bt)) Serial.print("1"); else Serial.print("0");
      }
      Serial.println(); 
    }
  }
}
void printMCM01() {
  uint8_t c;
  Serial.println("MAX7456");
  for (int t=0; t < 256; t++) {
    getNVMchar(charmem, t); 
    for (int i=0; i < COLNUM; i++) {
      for (int bt=6; bt >= 0; bt -= 2) {
        c = charmem[i] & (3 << bt);
        c = c >> bt;
        if (c==0) Serial.print("00");
        if (c==1) Serial.print("01");
        if (c==2) Serial.print("10");
        if (c==3) Serial.print("01");
      }
      Serial.println();
    }
  }
}

void printInitPattern() {
  mx->writeCharXY(0x16,2,13);
  mx->writeCharXY(0x17,2,12); 
  mx->writeCharXY(0x17,2,14); 
  mx->writeCharXY(0x19,6,13);
  mx->writeCharXY(0x18,6,12);
  mx->writeCharXY(0x18,6,14);
  mx->writeCharXY(0x01,3,11);
  mx->writeCharXY(0x01,4,11);
  mx->writeCharXY(0x01,5,11);
  mx->writeCharXY(0x11,3,15);
  mx->writeCharXY(0x11,4,15);
  mx->writeCharXY(0x11,5,15);
  
  mx->writeCharXY(0x0E,3,13); mx->writeCharXY(0x0E,4,13); mx->writeCharXY(0x0E,5,13); 
  
  
  mx->writeCharXY(0x16,12,13);
  mx->writeCharXY(0x17,12,12); 
  mx->writeCharXY(0x17,12,14); 
  mx->writeCharXY(0x19,16,13);
  mx->writeCharXY(0x18,16,12);
  mx->writeCharXY(0x18,16,14);
  mx->writeCharXY(0x01,13,11);
  mx->writeCharXY(0x01,14,11);
  mx->writeCharXY(0x01,15,11);
  mx->writeCharXY(0x11,13,15);
  mx->writeCharXY(0x11,14,15);
  mx->writeCharXY(0x11,15,15);
  
  mx->writeCharXY(0x86,13,13); mx->writeCharXY(0x87,14,13); mx->writeCharXY(0x88,15,13); 
  
  
  mx->writeCharXY(0x16, 5,5);  
  mx->writeCharXY('A',1,1);
  mx->writeCharXY('0',0,0);
  mx->writeCharXY('1',1,0);
}
  
void setup() {
  Serial.begin(115200);
  Serial.println("overlay code started\n");

  pinMode(SS,OUTPUT); // to avoid that we can be switched back into Slave mode. This pin is not used, but may not be INPUT and then LOW. See datasheet pp 197ff
  
  pinMode(green,OUTPUT); pinMode(yellow,OUTPUT);pinMode(led, OUTPUT);    

  mx->begin();
  mx->offset(31,16); // hori=-32...31, vert=-15..16. void MAX7456::offset(int horizontal, int vertical)
  mx->home();
  for (uint16_t c=0; c<256;c++) {
    mx->writeChar(c&0xff);
  }    
  mx->writeCharWithAttributes(5,0xc0);
  
  
  char charA[128];
  delay(100);
  Serial.println("********** DISPLAY **********");
  for (int y=0; y < 10; y++) {
    for (int x=0; x < 20; x++) {
      Serial.print(mx->ReadDisplay(x,y)); Serial.print(" ");
    }
    Serial.println();
  }
  printInitPattern();
  Serial.print("MAX7456 >");
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
      case 'd': // download font, transform 11 to 01
        printMCM01(); break;
      case 'r': // reset
        mx->reset();
        Serial.println("Soft reset executed"); break;
      case 'R': // Receive MCM file
        receiveMCM();  break;
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
  
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(250);               // wait for a second
  digitalWrite(led,LOW);
  delay(250);               // wait for a second
  
 
}
