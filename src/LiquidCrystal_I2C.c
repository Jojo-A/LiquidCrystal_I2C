/***************************************************************************************************/
/*
   This is a library for HD44780, S6A0069, KS0066U, NT3881D, LC7985, ST7066, SPLC780,
   WH160xB, AIP31066, GDM200xD, ADM0802A LCD displays.

   Screens are operated in 4 bit mode over i2c bus with 8-bit I/O expander PCF8574x.
   Typical displays sizes: 8x2, 16x1, 16x2, 16x4, 20x2, 20x4 & etc.

   written by : enjoyneering79, edited by Jojo-A
   sourse code: https://github.com/enjoyneering/


   This chip uses I2C bus to communicate, specials pins are required to interface
   Board:                                    SDA                    SCL                    Level
   Blue Pill, STM32F103xxxx boards.......... PB7                    PB6                    3.3v/5v

   GNU GPL license, all text above must be included in any redistribution,
   see link for details  - https://www.gnu.org/licenses/licenses.html
*/
/***************************************************************************************************/

#include "LiquidCrystal_I2C.h"


/**************************************************************************/
/*
    LiquidCrystal_I2C()

    Constructor. Initializes the class variables, defines I2C address,
    LCD & PCF8574 pins.
*/
/**************************************************************************/  
LiquidCrystal_I2C(PCF8574_address addr, uint8_t P0, uint8_t P1, uint8_t P2, uint8_t P3, uint8_t P4, uint8_t P5, uint8_t P6, uint8_t P7, backlightPolarity polarity)
{
  uint8_t PCF8574_TO_LCD[8] = {P0, P1, P2, P3, P4, P5, P6, P7}; //PCF8574 ports to LCD pins mapping array

  _PCF8574_address        = addr;
  _PCF8574_initialisation = true;
  _backlightPolarity      = polarity;

  /* maping LCD pins to PCF8574 ports */
  for (uint8_t i = 0; i < 8; i++)
  {
    switch(PCF8574_TO_LCD[i])
    {
      case 4:                   //RS pin
        _LCD_TO_PCF8574[7] = i;
        break;

      case 5:                   //RW pin
        _LCD_TO_PCF8574[6] = i;
        break;

      case 6:                   //EN pin
        _LCD_TO_PCF8574[5] = i;
        break;

      case 14:                  //D7 pin
        _LCD_TO_PCF8574[4] = i;
        break;

      case 13:                  //D6 pin
        _LCD_TO_PCF8574[3] = i;
        break;

      case 12:                  //D5 pin
        _LCD_TO_PCF8574[2] = i;
        break;

      case 11:                  //D4 pin
        _LCD_TO_PCF8574[1] = i;
        break;

      case 16:                  //BL pin
        _LCD_TO_PCF8574[0] = i;
        break;

      default:
        _PCF8574_initialisation = false; //safety check, make sure the declaration of lcd pins is right
        break;
    }
  }

  /* backlight control via PCF8574 */
  switch (_backlightPolarity)
  {
    case POSITIVE:
      _backlightValue = LCD_BACKLIGHT_ON;
      break;

    case NEGATIVE:
      _backlightValue = ~LCD_BACKLIGHT_ON;
      break;
  }

  _backlightValue <<= _LCD_TO_PCF8574[0];
}

/**************************************************************************/
/*
    LCDbegin()

    Initializes, resets & configures I2C bus & LCD
*/
/**************************************************************************/
bool LCDbegin(uint8_t lcd_colums, uint8_t lcd_rows, lcd_font_size f_size)
{	
  if (_PCF8574_initialisation == false) return false; //safety check, make sure the declaration of lcd pins is right

  //Wire.beginTransmission(_PCF8574_address);
  //if (Wire.endTransmission() != 0) return false;      //safety check, make sure the PCF8574 is connected

  if(writePCF8574(PCF8574_ALL_LOW) == false) return false;                      //safety check, set all PCF8574 pins low
	
  _lcd_colums 	 = lcd_colums;
  _lcd_rows      = lcd_rows;
  _lcd_font_size = f_size;

  LCDinitialization();                                   //soft reset & 4-bit mode initialization

  return true;
}

/**************************************************************************/
/*
    LCDclear()

    Clears display & move cursor to home position

    NOTE:
    - clear by fill it with space
    - cursor home position (0, 0)
    - command duration > 1.53 - 1.64ms
*/
/**************************************************************************/
void LCDclear(void)
{
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_CLEAR_DISPLAY, LCD_CMD_LENGTH_8BIT);

  HAL_Delay(LCD_HOME_CLEAR_DELAY);
}

/**************************************************************************/
/*
    LCDhome()

    Moves cursor position to home position

    NOTE:
    - sets DDRAM address to 0 in address counter, returns display to
      home position, but DDRAM contents remain unchanged
    - command duration > 1.53 - 1.64ms
*/
/**************************************************************************/
void LCDhome(void)
{
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_RETURN_HOME, LCD_CMD_LENGTH_8BIT);

  HAL_Delay(LCD_HOME_CLEAR_DELAY);
}

/**************************************************************************/
/*
    LCDsetCursor()

    Sets cursor position

    NOTE:
    - cursor start position (0, 0)
    - cursor end   position (lcd_colums - 1, lcd_rows - 1)
    - DDRAM data/text is sent & received after this setting
*/
/**************************************************************************/
void LCDsetCursor(uint8_t colum, uint8_t row)
{
  uint8_t row_address_offset[] = {0x00, 0x40, uint8_t(0x00 + _lcd_colums), uint8_t(0x40 + _lcd_colums)};

  /* safety check, cursor position & array are zero indexed */
  if (row   >= _lcd_rows)   row   = (_lcd_rows   - 1);
  if (colum >= _lcd_colums) colum = (_lcd_colums - 1);

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_DDRAM_ADDR_SET | (row_address_offset[row] + colum), LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDnoDisplay()

    Clears text from the screen

    NOTE:
    - text remains in DDRAM
*/
/**************************************************************************/
void LCDnoDisplay(void)
{
  _displayControl &= ~LCD_DISPLAY_ON;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDdisplay()

    Retrives text from DDRAM

    NOTE:
    - text remains in DDRAM
*/
/**************************************************************************/
void LCDdisplay(void)
{
  _displayControl |= LCD_DISPLAY_ON;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    noCursor()

    Turns OFF the underline cursor
*/
/**************************************************************************/
void LCDnoCursor(void)
{
  _displayControl &= ~LCD_UNDERLINE_CURSOR_ON;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDcursor()

    Turns ON the underline cursor
*/
/**************************************************************************/
void LCDcursor(void)
{
  _displayControl |= LCD_UNDERLINE_CURSOR_ON;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDnoBlink()

    Turns OFF the blinking cursor
*/
/**************************************************************************/
void LCDnoBlink(void)
{
  _displayControl &= ~LCD_BLINK_CURSOR_ON;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    blink()

    Turns ON the blinking cursor
*/
/**************************************************************************/
void LCDblink(void)
{
  _displayControl |= LCD_BLINK_CURSOR_ON;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_DISPLAY_CONTROL | _displayControl, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDscrollDisplayLeft()

    Scrolls once current row with text on the display to the left

    NOTE:
    - call this function just before write() or print()
    - text grows from cursor to the left
*/
/**************************************************************************/
void LCDscrollDisplayLeft(void)
{
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_CURSOR_DISPLAY_SHIFT | LCD_DISPLAY_SHIFT | LCD_SHIFT_LEFT, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDscrollDisplayRight()

    Scrolls once current row with text on the display to the right

    NOTE:
    - call this function just before write() or print()
    - text & cursor grows together to the left from cursor position
*/
/**************************************************************************/
void LCDscrollDisplayRight(void)
{
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_CURSOR_DISPLAY_SHIFT | LCD_DISPLAY_SHIFT | LCD_SHIFT_RIGHT, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDleftToRight()

    Sets text direction from left to right
*/
/**************************************************************************/
void LCDleftToRight(void)
{
  _displayMode |= LCD_ENTRY_LEFT;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDrightToLeft()

    Sets text direction from right to left
*/
/**************************************************************************/
void LCDrightToLeft(void)
{
  _displayMode &= ~LCD_ENTRY_LEFT;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDautoscroll()

    Autoscrolls the text rightToLeft() or rightToRight() on the display

    NOTE:
    - whole text on the display shift when byte written, but cursor stays
    - same as scrollDisplayRight() or scrollDisplayLeft() but no need to
      call it the loop, just call it once it setup()
*/
/**************************************************************************/
void LCDautoscroll(void) 
{
  _displayMode |= LCD_ENTRY_SHIFT_ON;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);
}


/**************************************************************************/
/*
    LCDnoAutoscroll()

    Stops text autoscrolling on the display

    NOTE:
    - whole text on the display stays, cursor shifts when byte written
*/
/**************************************************************************/
void LCDnoAutoscroll(void)
{
  _displayMode &= ~LCD_ENTRY_SHIFT_ON;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    LCDcreateChar()

    Fills 64-bytes CGRAM, with custom characters from dynamic memory

    NOTE:
    - 8 patterns for 5x8DOTS display, write address 0..7
      & read address 0..7/8..15
    - 4 patterns for 5x10DOTS display, wrire address 0..3
      & read address 0..3/0..7
*/
/**************************************************************************/
void LCDcreateChar(uint8_t CGRAM_address, uint8_t *char_pattern)
{
  uint8_t CGRAM_capacity = 0;
  int8_t  font_size      = 0;

  /* set CGRAM capacity */
  switch (_lcd_font_size)
  {
    case LCD_5x8DOTS:
      CGRAM_capacity = 7;                                                                      //8 patterns, 0..7
      font_size      = 8;
      break;

    case LCD_5x10DOTS:
      CGRAM_capacity = 3;                                                                      //4 patterns, 0..3
      font_size      = 10;
      break;
  }

  /* safety check, make sure "CGRAM_address" never exceeds the "CGRAM_capacity" */
  if (CGRAM_address > CGRAM_capacity) CGRAM_address = CGRAM_capacity;

  LCDsend(LCD_INSTRUCTION_WRITE, LCD_CGRAM_ADDR_SET | (CGRAM_address << 3), LCD_CMD_LENGTH_8BIT); //set CGRAM address

  for (uint8_t i = 0; i < font_size; i++)
  {
    LCDsend(LCD_DATA_WRITE, char_pattern[i], LCD_CMD_LENGTH_8BIT);                                //write data from dynamic memory to CGRAM address
  }
}


/**************************************************************************/
/*
    LCDnoBacklight()

    Turns off the backlight via PCF8574. 

    NOTE:
    - doesn't affect lcd controller, because we are working with
      transistor conncted to PCF8574 port
*/
/**************************************************************************/
void LCDnoBacklight(void)
{
  switch (_backlightPolarity)
  {
    case POSITIVE:
      _backlightValue = LCD_BACKLIGHT_OFF;
      break;

    case NEGATIVE:
      _backlightValue = ~LCD_BACKLIGHT_OFF;
      break;
  }

  _backlightValue <<= _LCD_TO_PCF8574[0];

  writePCF8574(PCF8574_ALL_LOW);
}

/**************************************************************************/
/*
    backlight()

    Turns on backlight via PCF8574.

    NOTE:
    - doesn't affect lcd controller, because we are working with
      transistor conncted to PCF8574 port
*/
/**************************************************************************/
void LCDbacklight(void)
{
  switch (_backlightPolarity)
  {
    case POSITIVE:
      _backlightValue = LCD_BACKLIGHT_ON;
      break;

    case NEGATIVE:
      _backlightValue = ~LCD_BACKLIGHT_ON;
      break;
  }

  _backlightValue <<= _LCD_TO_PCF8574[0];

  writePCF8574(PCF8574_ALL_LOW);
}

/**************************************************************************/
/*
    LCDwrite()

    Replaces function "write()" in Arduino class "Print" & sends character
    to the LCD
*/
/**************************************************************************/
void LCDwrite(uint8_t value)
{
  LCDsend(LCD_DATA_WRITE, value, LCD_CMD_LENGTH_8BIT);
}

/**************************************************************************/
/*
    initialization()

    Soft reset lcd & activate 4-bit interface

    NOTE:
    - for correct LCD operation it is necessary to do the internal circuit
      reset & initialization procedure. See 4-bit initializations
      procedure fig.24 on p.46 of HD44780 datasheet and p.17 of 
      WH1602B/WH1604B datasheet for details.
*/
/**************************************************************************/
void LCDinitialization(void)
{
  uint8_t displayFunction = 0; //don't change!!! default bits value DB7, DB6, DB5, DB4=(DL), DB3=(N), DB2=(F), DB1, DB0

  /*
     HD44780 & clones needs ~40ms after voltage rises above 2.7v
  */
  HAL_Delay(45);

  /*
     FIRST ATTEMPT: set 8-bit mode
     - wait > 4.1ms, some LCD even slower than 4.5ms
     - for Hitachi & Winstar displays
  */
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_8BIT_MODE, LCD_CMD_LENGTH_4BIT);
  HAL_Delay(5);

  /*
     SECOND ATTEMPT: set 8-bit mode
     - wait > 100us.
     - for Hitachi, not needed for Winstar displays
  */
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_8BIT_MODE, LCD_CMD_LENGTH_4BIT);
  HAL_Delay(1);
	
  /*
     THIRD ATTEMPT: set 8 bit mode
     - used for Hitachi, not needed for Winstar displays
  */
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_8BIT_MODE, LCD_CMD_LENGTH_4BIT);
  HAL_Delay(1);
	
  /*
     FINAL ATTEMPT: set 4-bit interface
     - Busy Flag (BF) can be checked after this instruction
  */
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_4BIT_MODE, LCD_CMD_LENGTH_4BIT);

  /* sets qnt. of lines */
  if (_lcd_rows > 1) displayFunction |= LCD_2_LINE;    //line bit located at BD3 & zero/1 line by default

  /* sets font size, 5x8 by default */
  if (_lcd_font_size == LCD_5x10DOTS)
  {
    displayFunction |= LCD_5x10DOTS;                   //font bit located at BD2
    if(_lcd_rows != 1) displayFunction &= ~LCD_2_LINE; //safety check, two rows displays can't display 10 pixel font
  }

  /* initializes lcd functions: qnt. of lines, font size, etc., this settings can't be changed after this point */
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_FUNCTION_SET | LCD_4BIT_MODE | displayFunction, LCD_CMD_LENGTH_8BIT);
	
  /* initializes lcd controls: turn display off, underline cursor off & blinking cursor off */
  _displayControl = LCD_UNDERLINE_CURSOR_OFF | LCD_BLINK_CURSOR_OFF;
  LCDnoDisplay();

  /* clear display */
  LCDclear();

  /* initializes lcd basics: sets text direction "left to right" & cursor movement to the right */
  _displayMode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_OFF;
  LCDsend(LCD_INSTRUCTION_WRITE, LCD_ENTRY_MODE_SET | _displayMode, LCD_CMD_LENGTH_8BIT);

  LCDdisplay();
}

/**************************************************************************/
/*
    send()

    The most anvanced & fastes way to write COMMAND or DATA/TEXT to LCD
    
    NOTE:
    - all inputs formated as follow: 
      - mode : RS,RW,E=1,DB7,DB6,DB5,DB4,BCK_LED=0
      - value: DB7,DB6,DB5,DB4,DB3,DB2,DB1,DB0

    - duration of command > 43usec for GDM2004D
    - duration of the En pulse > 450nsec
*/
/**************************************************************************/
void LCDsend(uint8_t mode, uint8_t value, uint8_t length)
{
  uint8_t  halfByte = 0; //lsb or msb
  uint8_t  data     = 0, dataBuffer[2] = {0};

  /* 4-bit or 1-st part of 8-bit command */
  halfByte  = value >> 3;                     //0,0,0,DB7,DB6,DB5,DB4,DB3
  halfByte &= 0x1E;                           //0,0,0,DB7,DB6,DB5,DB4,BCK_LED=0
  data      = portMapping(mode | halfByte);   //RS,RW,E=1,DB7,DB6,DB5,DB4,BCK_LED=0

  writePCF8574(data);                         //send command
                                              //En pulse duration > 450nsec
  bitClear(data, _LCD_TO_PCF8574[5]);         //RS,RW,E=0,DB7,DB6,DB5,DB4,BCK_LED=0
  writePCF8574(data);                         //execute command
  //delayMicroseconds(LCD_COMMAND_DELAY);       //command duration
  HAL_Delay(1);

  /* second part of 8-bit command */
  if (length == LCD_CMD_LENGTH_8BIT)
  {
    halfByte  = value << 1;                   //DB6,DB5,DB4,DB3,DB2,DB1,DB0,0
    halfByte &= 0x1E;                         //0,0,0,DB3,DB2,DB1,DB0,BCK_LED=0
    data      = portMapping(mode | halfByte); //RS,RW,E=1,DB3,DB2,DB1,DB0,BCK_LED=0

    writePCF8574(data);                       //send command
                                              //En pulse duration > 450nsec
    bitClear(data, _LCD_TO_PCF8574[5]);       //RS,RW,E=0,DB3,DB2,DB1,DB0,BCK_LED=0
    writePCF8574(data);                       //execute command
    //delayMicroseconds(LCD_COMMAND_DELAY);     //command duration
    HAL_Delay(1);
  }
}

/**************************************************************************/
/*
    LCDportMapping()

    All magic of all lcd pins to ports mapping is happening here!!!

    NOTE:
    - input value formated as:
        7  6  5  4  3   2   1   0-bit
      - RS,RW,E,DB7,DB6,DB5,DB4,BCK_LED
      - RS,RW,E,DB3,DB2,DB1,DB0,BCK_LED

    - lcd pin to PCF8574 ports table/array formated
        0       1   2  3   4   5  6  7
      {BCK_LED,DB4,DB5,DB6,DB7,E,RW,RS} 

    - shifts value bits to the right PCF8574 ports
      {BCK_LED,DB4,DB5,DB6,DB7,E,RW,RS} shift-> to ports position P7..P0
      {BCK_LED,DB4,DB5,DB6,DB7,E,RW,RS} shift-> to ports position P7..P0

    - "switch case" is 32us faster than
      bitWrite(data, _LCD_TO_PCF8574[i], bitRead(value, i));
*/
/**************************************************************************/
uint8_t LCDportMapping(uint8_t value)
{
  uint8_t data = 0;

  /* mapping value = RS,RW,E,DB7,DB6,DB5,DB4,BCK_LED */
  for (int8_t i = 7; i >= 0; i--)
  {
    switch (bitRead(value, i))              //"switch case" has smaller footprint than "if else"
    {
      case 1:
        data |= 0x01 << _LCD_TO_PCF8574[i];
        break;
    }
  }

  return data; 
}

/**************************************************************************/
/*
    writePCF8574()

    Masks backlight with data & writes it to PCF8574 over I2C

    NOTE:
    - Wire.endTransmission() returned code:
      0 - success
      1 - data too long to fit in transmit data16
      2 - received NACK on transmit of address
      3 - received NACK on transmit of data
      4 - other error
*/
/**************************************************************************/
bool writePCF8574(uint8_t value)
{
  /*Wire.beginTransmission(_PCF8574_address);
	
  Wire.send(value | _backlightValue);

  if (Wire.endTransmission(true) == 0) return true;
                                       return false;*/
  value |= _backlightValue;
  if( HAL_I2C_Master_Transmit(&hi2c1, _PCF8574_address,(uint8_t *) &value, 1, 100) != HAL_OK) return false;
	
  return true;
}

/**************************************************************************/
/*
    readPCF8574()

    Reads byte* from PCF8574 over I2C

    *logic values on the PCF8574 pins P0...P7

    NOTE:
    - if PCF8574 output is written low before read, the low is always
      returned, regardless of the device state connected
      to the I/O, see Quasi-Bidirectional I/O for more details.
    - if PCF8574 output is written high before read, devices has fully
      control of PCF8574 I/O.
*/
/**************************************************************************/
uint8_t readPCF8574()
{
  uint8_t l_Data = 0;
	
  if(HAL_I2C_Master_Receive(&hi2c1, _PCF8574_address,(uint8_t *) &l_Data, 1, 100) != HAL_OK) return false;
	
  return l_Data;
}

/**************************************************************************/
/*
    readBusyFlag()

    Reads busy flag (BF)

    NOTE:
    - set RS=0 & RW=1 to retrive busy flag
    - set PCF8574 input pins to HIGH, see Quasi-Bidirectional I/O
    - DB7 = 1, lcd busy
      DB7 = 0, lcd ready
    - input value formated as:
        7  6  5  4  3   2   1   0-bit
      - RS,RW,E,DB7,DB6,DB5,DB4,BCK_LED
      - RS,RW,E,DB3,DB2,DB1,DB0,BCK_LED
*/
/**************************************************************************/
bool LCDreadBusyFlag()
{
  LCDsend(LCD_BUSY_FLAG_READ, PCF8574_DATA_HIGH, LCD_CMD_LENGTH_4BIT); //set RS=0, RW=1 & input pins to HIGH, see Quasi-Bidirectional I/O

  return bitRead(readPCF8574(), _LCD_TO_PCF8574[4]);
}

/**************************************************************************/
/*
    LCDgetCursorPosition()

    Returns contents of address counter

    NOTE:
    - set RS=0 & RW=1 to retrive address counter
    - set PCF8574 input pins to HIGH, see Quasi-Bidirectional I/O
    - address counter content DB6,DB5,DB4,DB3,DB2,DB1,DB0 
    - input value formated as:
        7  6  5  4  3   2   1   0-bit
      - RS,RW,E,DB7,DB6,DB5,DB4,BCK_LED
      - RS,RW,E,DB3,DB2,DB1,DB0,BCK_LED
*/
/**************************************************************************/
uint8_t LCDgetCursorPosition()
{
  uint8_t data     = 0;
  uint8_t position = 0;

  LCDsend(LCD_BUSY_FLAG_READ, PCF8574_DATA_HIGH, LCD_CMD_LENGTH_4BIT); //set RS=0, RW=1 & input pins to HIGH, see Quasi-Bidirectional I/O

  data = readPCF8574();                                             //read RS,RW,E,DB7,DB6,DB5,DB4,BCK_LED

  /* saving DB6,DB5,DB4 bits*/
  for (int8_t i = 3; i >= 1; i--)
  {
    bitWrite(position, (3 + i), bitRead(data, _LCD_TO_PCF8574[i])); //xx,DB6,DB5,DB4,DB3,DB2,DB1,DB0
  }

  LCDsend(LCD_BUSY_FLAG_READ, PCF8574_DATA_HIGH, LCD_CMD_LENGTH_4BIT); //set RS=0, RW=1 & input pins to HIGH, see Quasi-Bidirectional I/O

  data = readPCF8574();                                             //read RS,RW,E,DB3,DB2,DB1,DB0,BCK_LED

  /* saving DB3,DB2,DB1,DB0 bits */
  for (int8_t i = 4; i >= 1; i--)
  {
    bitWrite(position, (i - 1), bitRead(data, _LCD_TO_PCF8574[i])); //xx,DB6,DB5,DB4,DB3,DB2,DB1,DB0
  }

  return position;
}

/*************** !!! arduino not standard API functions !!! ***************/
/**************************************************************************/
/*
    LCDprintHorizontalGraph(name, row, value, maxValue)

    Prints horizontal graph
*/
/**************************************************************************/
void LCDprintHorizontalGraph(char name, uint8_t row, uint16_t currentValue, uint16_t maxValue)
{
  uint8_t currentGraph = 0;
  uint8_t colum        = 0;

  if (currentValue > maxValue) currentValue = maxValue;          //safety check, to prevent ESP8266 crash

  currentGraph = map(currentValue, 0, maxValue, 0, _lcd_colums);

  setCursor(colum, row);
  send(LCD_DATA_WRITE, name, LCD_CMD_LENGTH_8BIT);

  /* draw the horizontal bar without clearing the display, to eliminate flickering */
  for (colum = 1; colum < currentGraph; colum++)
  {
    setCursor(colum, row);
    send(LCD_DATA_WRITE, 0xFF, LCD_CMD_LENGTH_8BIT);             //print 0xFF - built in "solid square" symbol, see p.17 & p.30 of HD44780 datasheet
  }

  /* fill the rest with spaces */
  while (colum++ < _lcd_colums)
  {
    send(LCD_DATA_WRITE, 0x20, LCD_CMD_LENGTH_8BIT);             //print 0x20 - built in "space" symbol, see p.17 & p.30 of HD44780 datasheet
  }
}

/**************************************************************************/
/*
    LCDdisplayOff()

    Turns off backlight via PCF8574 & clears text from the screen

    NOTE:
    - text remains in DDRAM
*/
/**************************************************************************/
void LCDdisplayOff(void)
{
  LCDnoBacklight();
  LCDnoDisplay();
}

/**************************************************************************/
/*
    LCDdisplayOn()

    Turns on backlight via PCF8574 & shows text from DDRAM
*/
/**************************************************************************/
void LCDdisplayOn(void)
{
  LCDdisplay();
  LCDbacklight();
}

/**************************************************************************/
/*
    setBrightness()

    NOTE:
    - to use this function, the "LED" jumper on the back of backpack has
      to be removed & the top pin has to be connected to one of Arduino
      PWM pin in series with 470 Ohm resistor
    - recomended min. value = 25, max. value = 255 (0.5v .. 4.5v)
                 min. value = 0,  max. value = 255 (0.0v .. 4.5v)  
*/
/**************************************************************************/
/*void setBrightness(uint8_t pin, uint8_t value, backlightPolarity polarity)
{
  pinMode(pin, OUTPUT);

  if (polarity == NEGATIVE) value = 255 - value;

  analogWrite(pin, value);
}*/
