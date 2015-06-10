# overlay
MAX7456 arduino support library

Flash this sketch to an Arduino board with a MAX7456 connected via SPI.
You need to set the preprocessor constants in overlay.ino to define the
pins for MISO, MOSI, SCK and SS, to enable the connection.

The script will open up a pseudo-terminal that you can access via serial
(it uses Serial.begin(115200) to open Serial).
I recommend using teraterm.exe to connect to the Arduino. It will
print a welcome message and then wait for commands. The following commands
are available:
p - just print a welcome string "Hallo" to the overlay display with fast printing (16 bit mode, autoincrement). Prints the time taken.
P - clears the screen, shows the current font of the MAX7456 (all characters).
o - prints welcome string "Hallo" to the overlay display with slow printing (8 bit mode, no autoincrement). Prints the time taken.
D - prints the current font from character memory to serial. Activate logging into file and start this,
    the result can be saved as a .mcm file.
d - prints the current font from character memory to serial, converting "11" bit pairs to "10". Although the meaning
    of 11 and 10 is the same to the MAX7456, it is not the same to many character editor programs on the web, and many
    of them do not work properly with "11" pixels.
r - reset. Perform a soft reset of the MAX7456.
R - Receive MCM file. Use "send file" in teraterm to send a new character set to the character flash memory.
f - show font. Prints all characters to see what the current font looks like.
? - read status. Prints the status byte (reads 0xA0)
w - write string. Type to the terminal, and what you type will be printed to the MAX7456. <RETURN> ends.
b - experimental only, dont use.

