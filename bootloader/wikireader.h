#ifndef WIKIREADER_H
#define WIKIREADER_H

static inline void init_pins(void)
{					
	/* P13 & P14: debug LEDs */	
	REG_P1_IOC1 = 0x18;		
					
	/* P50 & P52: CS lines */	
	REG_P5_IOC5 = 0x06;		
	REG_PA_IOC = 0x08;		
					
	/* P65-67: SPI */		
	REG_P6_47_CFP = 0x54;		
					
	/* Serial interface */		
	REG_P0_03_CFP = 0x05;		
}


static inline void init_rs232(void)
{										
	/* serial line 0: 8-bit async, no parity, internal clock, 1 stop bit */	
	REG_EFSIF0_CTL = 0xc3;							
										
	/* DIVMD = 1/8, General I/F mode */					
	REG_EFSIF0_IRDA = 0x10;							
										
	/* by default MCLKDIV = 0 which means that the internal MCLK is OSC/1,	
	 * where OSC = OSC3 as OSCSEL[1:0] = 00b				
	 * Hence, MCLK is 48MHz */						
										
	/* set up baud rate timer reload data */				
	/* 									
	 * BRTRD = ((F[brclk] * DIVMD) / (2 * bps)) - 1;			
	 * where								
	 * 	F[brclk] = 48MHz						
	 * 	DIVMD = 1/8							
	 *	bps = 57600							
	 *									
	 *   = 51								
	 */									
										
	REG_EFSIF0_BRTRDL = 51 & 0xff;						
	REG_EFSIF0_BRTRDM = 51 >> 8;						
										
	/* baud rate timer: run! */						
	REG_EFSIF0_BRTRUN = 0x01;						
}

/* MRS command address for burst length=1, CAS latency = 2 */
#define MRSREG          (*(volatile unsigned char *) 0x10000442)
#define RAMDUMMY        (*(volatile unsigned char *) 0x10000000)

static inline void init_ram(void)
{
	int i;

        /* RAM init */

        /* disable write protection of clock registers */
        REG_CMU_PROTECT = 0x96;

        /* switch on SDRAM clk */
        REG_CMU_GATEDCLK0 = 0x78;

        /* P20-P27 functions */
        REG_P2_03_CFP = 0x55;
        REG_P2_47_CFP = 0x55;
        REG_P5_03_CFP = 0x80;

        /* enable write protection of clock registers */
        REG_CMU_PROTECT = 0x00;

        /* re-enable SDRAMC application core */
        REG_SDRAMC_APP = 0x8000000b;

        /* set up SDRAM controller */
        /* 4M x 16 bits x 1, 15 Trs/Trfc/Txsr cycles, 3 Tras, 2 Trp */
        REG_SDRAMC_CTL = 0x37e1;

        /* enable RAM self-refresh */
        //REG_SDRAMC_REF |= (1 << 25);

        REG_SDRAMC_INI = 0x10;  /* exit setup mode */

        /* SDRAM command sequence: PALL - REF - REF - MRS (for MT48LC16M16A2) */
        REG_SDRAMC_INI = 0x12;  /* INIPRE */
        RAMDUMMY = 0x0;         /* dummy write */

        REG_SDRAMC_INI = 0x12;  /* INIPRE */
        RAMDUMMY = 0x0;         /* dummy write */

        for (i = 0; i < 2; i++) {
                REG_SDRAMC_INI = 0x11;  /* INIREF */
                RAMDUMMY = 0x0;         /* dummy write */
        }

        REG_SDRAMC_INI = 0x14;  /* INIMRS */
        MRSREG = 0x0;           /* dummy write */

        REG_SDRAMC_INI = 0x10;  /* exit setup mode */
}

#define DEBUGLED1_ON()	do { REG_P1_P1D &= ~(1 << 4); } while (0)
#define DEBUGLED1_OFF()	do { REG_P1_P1D |=  (1 << 4); } while (0)

#define DEBUGLED2_ON()	do { REG_P1_P1D &= ~(1 << 3); } while (0)
#define DEBUGLED2_OFF()	do { REG_P1_P1D |=  (1 << 3); } while (0)

//#define SDCARD_CS_LO()	do { REG_P5_P5D &= ~(1 << 0); } while (0)
//#define SDCARD_CS_HI()	do { REG_P5_P5D |=  (1 << 0); } while (0)
#define SDCARD_CS_LO()	do { REG_PA_DATA &= ~(1 << 3); } while (0)
#define SDCARD_CS_HI()	do { REG_PA_DATA |=  (1 << 3); } while (0)

#define EEPROM_CS_LO()	do { REG_P5_P5D &= ~(1 << 2); } while (0)
#define EEPROM_CS_HI()	do { REG_P5_P5D |=  (1 << 2); } while (0)

#endif /* WIKIREADER_H */

