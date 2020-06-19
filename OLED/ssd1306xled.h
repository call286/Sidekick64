/**
 * SSD1306xLED - Library for the SSD1306 based OLED/PLED 128x64 displays
 *
 * @author Neven Boyanov
 *
 * This is part of the Tinusaur/SSD1306xLED project.
 *
 * Copyright (c) 2018 Neven Boyanov, The Tinusaur Team. All Rights Reserved.
 * Distributed as open source software under MIT License, see LICENSE.txt file.
 * Retain in your source code the link http://tinusaur.org to the Tinusaur project.
 *
 * Source code available at: https://bitbucket.org/tinusaur/ssd1306xled
 *
 */

#ifndef SSD1306XLED_H
#define SSD1306XLED_H

// ============================================================================

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32; 

typedef u8		uint8_t;
typedef u16		uint16_t;

// -----(+)-------------------->-----> [Vcc]	Pin 1 on the SSD1306 display board
// -----(-)-------------------->-----> [GND]	Pin 2 on the SSD1306 display board
//#define SSD1306_SCL		PB2   // ----> [SCL]	Pin 3 on the SSD1306 display board
//#define SSD1306_SDA		PB0   // ----> [SDA]	Pin 4 on the SSD1306 display board
#ifndef SSD1306_SCL
//#define SSD1306_SCL		PB2	// SCL,	Pin 3 on SSD1306 Board
#define SSD1306_SCL		1	// SCL,	Pin 3 on SSD1306 Board
#endif
#ifndef SSD1306_SDA
//#define SSD1306_SDA		PB1	// SDA,	Pin 4 on SSD1306 Board
#define SSD1306_SDA		0	// SDA,	Pin 4 on SSD1306 Board
#endif
#ifndef SSD1306_SA
#define SSD1306_SA		0x78	// Slave address
#endif
#define SSD1306_SADDR	0x78  // Display IC2 slave address, default 0x78

// ----------------------------------------------------------------------------

#define ssd1306_clear() ssd1306_fill4(0, 0, 0, 0)
#define ssd1306_fill(p) ssd1306_fill4(p, p, p, p)
#define ssd1306_fill2(p1, p2) ssd1306_fill4(p1, p2, p1, p2)

// ----------------------------------------------------------------------------

void ssd1306_init(void);
void ssd1306_setpos(uint8_t x, uint8_t y);
void ssd1306_fill4(uint8_t, uint8_t, uint8_t, uint8_t);

// ============================================================================

#endif
