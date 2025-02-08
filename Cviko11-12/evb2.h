#ifndef EVB2_H_
#define EVB2_H_

#define LCD_RS_H		P1OUT |= 1
#define LCD_RS_L		P1OUT &= ~1
#define LCD_EN_H		P1OUT |= (1 << 1)
#define LCD_EN_L		P1OUT &= ~(1 << 1)

void evb2_io_init(void);
void lcd_write(unsigned char byte);
void lcd_clear(void);
void lcd_puts(const unsigned char *s);
void lcd_putch(char c);
void lcd_goto(unsigned char pos);
void lcd_init(void);
void lcd_strobe(void);

#endif /*EVB2_H_*/
