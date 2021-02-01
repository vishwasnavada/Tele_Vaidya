
#ifndef _ST7735H_
#define _ST7735H_

#if ARDUINO >= 100
 #include "Arduino.h"
 #include "Print.h"
#else
 #include "WProgram.h"
#endif
#include "ERGFX.h"
#include <avr/pgmspace.h>


#define ST7735_TFTWIDTH  128
#define ST7735_TFTHEIGHT 128

#define ST7735_CASET   0x2A
#define ST7735_PASET   0x2B
#define ST7735_RAMWR   0x2C


// Color definitions
#define	ST7735_BLACK   0x0000
#define	ST7735_BLUE    0x001F
#define	ST7735_RED     0xF800
#define	ST7735_GREEN   0x07E0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW  0xFFE0  
#define ST7735_WHITE   0xFFFF


class ST7735 : public ERGFX {

 public:

  ST7735(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK,
		   int8_t _RST);
  ST7735(int8_t _CS, int8_t _DC, int8_t _RST = -1);

  void     begin(void),
           setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1),
           pushColor(uint16_t color),
           fillScreen(uint16_t color),
           drawPixel(int16_t x, int16_t y, uint16_t color),
           drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
           drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
           fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
             uint16_t color);
        
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

  /* These are not for current use, 8-bit protocol only! */
  uint8_t  readdata(void),
    readcommand8(uint8_t reg, uint8_t index = 0);
  /*
  uint16_t readcommand16(uint8_t);
  uint32_t readcommand32(uint8_t);
  void     dummyclock(void);
  */  

  void     spiwrite(uint8_t),
    writecommand(uint8_t c),
    writedata(uint8_t d),
    commandList(uint8_t *addr);
  uint8_t  spiread(void);

 private:
  uint8_t  tabcolor;



  boolean  hwSPI;
#if defined (__AVR__) || defined(TEENSYDUINO)
  uint8_t mySPCR;
  volatile uint8_t *mosiport, *clkport, *dcport, *rsport, *csport;
  int8_t  _cs, _dc, _rst, _mosi, _sclk;
  uint8_t  mosipinmask, clkpinmask, cspinmask, dcpinmask;
#elif defined (__arm__)
    volatile RwReg *mosiport, *clkport, *dcport, *rsport, *csport;
    uint32_t  _cs, _dc, _rst, _mosi, _sclk;
    uint32_t  mosipinmask, clkpinmask, cspinmask, dcpinmask;
#endif
};

#endif
