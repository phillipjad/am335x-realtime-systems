#ifndef LCD_SCREEN_H
#define LCD_SCREEN_H

#include "project_types.h"

void lcd_clear(int fd);
void lcd_set_cursor(int fd, uint8_t row, uint8_t col);
void lcd_print(int fd, const char *str);
void lcd_print_float(int fd, float64_t val);
int lcd_init(const char *i2c_path, uint8_t lcd_addr);
void lcd_set_cursor(int fd, uint8_t row, uint8_t col);
void lcd_print_float(int fd, float64_t val);
void *lcd_screen_thread_entry(void *arg);

#endif /* LCD_SCREEN_H */
