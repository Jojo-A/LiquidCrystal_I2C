
# LiquidCrystal_I2C

This is a library for LCD HD44780 and its clones (S6A0069, KS0066U, NT3881D, LC7985, ST7066, SPLC780, WH160xB, AIP31066, GDM200xD). It works in 4-bit mode via I²C bus with 8-bit PCF8574/PCF8574A I/O expander.

The 99.9% of all PCF8574 I²C backpacks from eBay connected like this:

| PCF8574 ports | LCD pins |
| ---- | ------- |
| P0 | 4/RS |
| P1 | 5/RW |
| P2 | 6/En |
| P3 | 16/BACKLIGHT LED-, with turn-on level HIGH/POSITIVE |
| P4 | 11/DB4 |
| P5 | 12/DB5 |
| P6 | 13/DB6 |
| P7 | 14/DB7 |

The initialization string:
```C++
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);
```
But what if your I²C backpack is different? The LCD pin **4/RS** connected to PCF8574 port **P6** & LCD pin **13/DB6** connected to PCF8574 port **P0**.

| PCF8574 ports | LCD pins |
| ---- | ------- |
| P0 | 13/DB6 |
| P1 | 5/RW |
| P2 | 6/En |
| P3 | 16/BACKLIGHT LED-, with turn-on level HIGH/POSITIVE |
| P4 | 11/DB4 |
| P5 | 12/DB5 |
| P6 | 4/RS |
| P7 | 14/DB7 |

The initialization string:
```C++
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 13, 5, 6, 16, 11, 12, 4, 14, POSITIVE);
```

Supports:
- Arduino STM32 (HAL)

