// ssd1306.h
#ifndef SSD1306_H
#define SSD1306_H

#include <stdbool.h>

// SSD1306 command definitions
#define SSD1306_DISPLAY_OFF            0xAE
#define SSD1306_DISPLAY_ON             0xAF
#define SSD1306_SET_CONTRAST           0x81
#define SSD1306_ENTIRE_DISPLAY_RESUME  0xA4
#define SSD1306_ENTIRE_DISPLAY_ON      0xA5
#define SSD1306_NORMAL_DISPLAY         0xA6
#define SSD1306_INVERT_DISPLAY         0xA7
#define SSD1306_SET_DISPLAY_OFFSET     0xD3
#define SSD1306_SET_START_LINE         0x40
#define SSD1306_SET_SEGMENT_REMAP_0    0xA0
#define SSD1306_SET_SEGMENT_REMAP_1    0xA1
#define SSD1306_SET_COM_SCAN_INC       0xC0
#define SSD1306_SET_COM_SCAN_DEC       0xC8
#define SSD1306_SET_MULTIPLEX_RATIO    0xA8
#define SSD1306_SET_DISPLAY_CLOCK_DIV  0xD5
#define SSD1306_SET_PRECHARGE          0xD9
#define SSD1306_SET_VCOMH_DESELECT     0xDB
#define SSD1306_SET_CHARGE_PUMP        0x8D
#define SSD1306_MEMORY_MODE            0x20
#define SSD1306_COLUMN_ADDR            0x21
#define SSD1306_PAGE_ADDR              0x22

void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_show(void);

void ssd1306_draw_pixel(int x, int y, bool on);
void ssd1306_draw_line(int x0, int y0, int x1, int y1, bool on);
void ssd1306_draw_char(int x, int y, char c);
void ssd1306_draw_string(int x, int y, const char *str);

void ssd1306_walk_horizontal_pixel(void);

#endif
