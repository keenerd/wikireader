#ifndef CONFIG_H
#define CONFIG_H

//#define BOARD_S1C33E07 1
//#define BOARD_PRT33L17LCD 1
#define BOARD_PROTO1 1

#define LCD_MONOCHROME 1
//#define POWER_MANAGEMENT 1

#if BOARD_PROTO1
	#define EEPROM_SST25VF040 1
#else
	#define EEPROM_MP45PE80 1
#endif

#endif /* CONFIG_H */
