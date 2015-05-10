/**   
 * \file sdcard.cpp
 * \date 2013-4-27
 * \brief SD card
 *
 * \copyright
 * Copyright (c) 2013, Stemlux Systems Ltd. All rights reserved.
 * distributed under the New BSD license.
 * See accompanying file LICENSE.txt
 * 
 */

/************************************************************************/
/* Includes                                                             */
/************************************************************************/
#include <aery32/gpio.h>
#include <aery32/intc.h>
#include <aery32/delay.h>
#include <aery32/spi.h>
#include "sdcard.h"			// Common include file for FatFs and disk I/O layer

/************************************************************************/
/* Macros                                                               */
/************************************************************************/
/* PIN SETTINGS: */
#define MMC_CS			AVR32_PIN_PA10
#define MMC_MISO		AVR32_PIN_PA11
#define MMC_MOSI		AVR32_PIN_PA12
#define MMC_SCLK		AVR32_PIN_PA13
#define MMC_SPI_GPIO_MASK ( (1 << 11) | (1 << 12) | (1 << 13) )
#define	CS_H()			aery::gpio_set_pin_high( MMC_CS )	// Set MMC CS "high"
#define CS_L()			aery::gpio_set_pin_low( MMC_CS )	// Set MMC CS "low"

/* MMC/SD commands (SPI mode): */
#define CMD0	(0)		// GO_IDLE_STATE
#define CMD1	(1)		// SEND_OP_COND
#define	ACMD41	(0x80+41)	// SEND_OP_COND (SDC)
#define CMD8	(8)		// SEND_IF_COND
#define CMD9	(9)		// SEND_CSD
#define CMD10	(10)		// SEND_CID
#define CMD12	(12)		// STOP_TRANSMISSION
#define CMD13	(13)		// SEND_STATUS
#define ACMD13	(0x80+13)	// SD_STATUS (SDC)
#define CMD16	(16)		// SET_BLOCKLEN
#define CMD17	(17)		// READ_SINGLE_BLOCK
#define CMD18	(18)		// READ_MULTIPLE_BLOCK
#define CMD23	(23)		// SET_BLOCK_COUNT
#define	ACMD23	(0x80+23)	// SET_WR_BLK_ERASE_COUNT (SDC)
#define CMD24	(24)		// WRITE_BLOCK
#define CMD25	(25)		// WRITE_MULTIPLE_BLOCK
#define CMD41	(41)		// SEND_OP_COND (ACMD)
#define CMD55	(55)		// APP_CMD
#define CMD58	(58)		// READ_OCR

#define enable_intc_globally() __builtin_mtsr(AVR32_SR, __builtin_mfsr(AVR32_SR) & ~(1 << 16))

static BYTE CardType;
static DSTATUS Stat = STA_NOINIT;	// Disk status
char dummy[514];

/************************************************************************/
/* Functions                                                            */
/************************************************************************/

volatile bool multiBlockTransfer = false;
volatile unsigned int multiBlockTransferCounter = 0;
BYTE * multiBlockTransferBuffer;
void pdca_int_handler(void) // interrupt handler
{
	aery::intc_disable_globally();
	AVR32_PDCA.channel[0].CR.tdis = 1; // disable transfer
	AVR32_PDCA.channel[1].CR.tdis = 1; // disable transfer
	AVR32_PDCA.channel[1].IDR.trc = 1; // disable interrupt
	aery::intc_enable_globally();
	
	if(multiBlockTransfer)
	{
		if (!(--multiBlockTransferCounter))
		{
			mmc_send_command(CMD12, 0); // end of transfer burst
			multiBlockTransfer = false;
			return;
		}
		multiBlockTransferBuffer+=512;
		if(!mmc_read_datablock(multiBlockTransferBuffer, 512))
			multiBlockTransfer = false;
	}
	//asm("nop");
}

void mmc_init_pdca()
{
	// Channel 0 for spi rx
	AVR32_PDCA.channel[0].PSR.pid = AVR32_PDCA_PID_SPI0_RX; //SPI Rx
	AVR32_PDCA.channel[0].MR.size = 0x0; // 8bit
	
	// Channel 1 for spi tx
	AVR32_PDCA.channel[1].PSR.pid = AVR32_PDCA_PID_SPI0_TX; //SPI Rx
	AVR32_PDCA.channel[1].MR.size = 0x0; // 8bit
	
	for(int i=0; i<514; i++) dummy[i]=0xFF;
	
	aery::intc_register_isrhandler(&pdca_int_handler,3,AVR32_INTC_INT0);
	aery::intc_init();
}

/**
 * \brief Initialize SPI for SD card usage
 * Clock rate is MCLK / clk_div
 */
void mmc_init_spi(BYTE clk_div = 0xFF)
{
	aery::gpio_init_pin( MMC_CS, GPIO_OUTPUT );
 	aery::gpio_set_pin_high(MMC_CS);
  	aery::gpio_init_pins(aery::porta, MMC_SPI_GPIO_MASK, GPIO_FUNCTION_A);  
	aery::spi0->CR.spien = 0; // disable SPI
 	aery::spi_init_master(aery::spi0);
  	aery::spi_setup_npcs(aery::spi0, 0, aery::SPI_MODE3, 8);
	aery::spi0->CSR0.scbr = clk_div; // SPI clock
  	aery::spi0->CR.spien = 1; // enable SPI
	aery::spi0->CSR0.dlybct = 0;
	
	mmc_init_pdca();
}

/**
 * \brief Transfer byte over SPI
 */
inline char mmc_spi_transfer( char c )
{
	mmc_wait_pdca_finish(); // wait for spi line to be free
	unsigned int tmr = 0;
	aery::spi0->tdr = c;
	while ( ((AVR32_SPI0.sr & AVR32_SPI_SR_RDRF_MASK) == 0) && (++tmr < 100000) );
	return aery::spi0->RDR.rd;
}

/**
 * \brief Wait until card is ready
 */
int mmc_wait_ready(void)	
{
	BYTE d;
	unsigned int tmr;

	for( tmr = 50000; tmr; tmr-- )
	{	
		d = mmc_spi_transfer(0xFF);
		if (d == 0xFF) break;
		aery::delay_us(10);
	}

	return tmr ? 1 : 0; // 1=OK, 0=Timeout
}

/**
 * \brief deselect card
 */
void mmc_deselect(void)
{
	mmc_wait_pdca_finish();
	aery::gpio_set_pin_high(MMC_CS);
	mmc_spi_transfer(0xFF); // I'm a dummy
}

/**
 * \brief Select card
 * \return 1 for OK & 0 for timeout
 */
int mmc_select(void)	/*  */
{
	mmc_wait_pdca_finish();
	aery::gpio_set_pin_low(MMC_CS);
	if (mmc_wait_ready()) return 1;
	mmc_deselect();
	return 0;
}


/**
 *	\brief Receive a data packet from the card
 */
volatile BYTE garbage[8];
int mmc_read_datablock (	/* 1:OK, 0:Failed */
	BYTE *buffer,			/* Data buffer to store received data */
	UINT bytes			/* Byte count */
)
{
	BYTE d;
	UINT tmr;

	for (tmr = 10000; tmr; tmr--) {	// Wait for data packet
		d = mmc_spi_transfer(0xFF);
		if (d != 0xFF) break;
		asm("nop");
	}
	if (d != 0xFE) return 0;		/* If not valid data token, return with error */
	
	// channel 0 reload:
	AVR32_PDCA.channel[0].tcr = bytes; // block size;
	AVR32_PDCA.channel[0].tcrr = 2; // response size;
	AVR32_PDCA.channel[0].mar = (unsigned int)buffer;
	AVR32_PDCA.channel[0].marr = (unsigned int)garbage;
	AVR32_PDCA.channel[0].CR.eclr = 1; // clear errors transfer
	AVR32_PDCA.channel[0].isr;

	// channel 1 reload:
	AVR32_PDCA.channel[1].tcr = bytes+2; // block size;
	AVR32_PDCA.channel[1].mar = (uint32_t)dummy;
	AVR32_PDCA.channel[1].CR.eclr = 1; // clear errors transfer
	AVR32_PDCA.channel[1].isr;
	AVR32_PDCA.channel[1].IER.trc = 1;	
	
	// enable interrupt controller:
	enable_intc_globally();
		
	// enable transfer:
	AVR32_PDCA.channel[0].CR.ten = 1; // enable transfer
	AVR32_PDCA.channel[1].CR.ten = 1; // enable transfer
	
	return 1;						// Return with success 
}



/**
 *	\brief Send a data packet to the card
 */
int mmc_send_datablock (	/* 1:OK, 0:Failed */
	const BYTE *buffer,	/* 512 byte data block to be transmitted */
	BYTE token			/* Data/Stop token */
)
{
	BYTE d;
	
	if (!mmc_wait_ready()) return 0;

	mmc_spi_transfer(token);	// transmit a token
	if (token != 0xFD)			// Is it data token?
	{
		for( int i = 0; i < 512; i++ )		
		mmc_spi_transfer(buffer[i]);	// transmit the 512 byte data block to MMC
		mmc_spi_transfer(0xFF);			// transmit dummy CRC (0xFF,0xFF)
		mmc_spi_transfer(0xFF);			// transmit dummy CRC (0xFF,0xFF)
		d = mmc_spi_transfer(0xFF);	    // Receive data response
		if ((d & 0x1F) != 0x05)	// If not accepted, return with error
		return 0;
	}

	return 1;
}

/**
 *	\brief Send command to SD/MMC
 *	Returns command response (bit7==1:Send failed)
 */
BYTE mmc_send_command ( BYTE cmd,  DWORD arg )
{
	BYTE n, d;

	if (cmd & 0x80) {	// ACMD<n> is the command sequence of CMD55-CMD<n>
		cmd &= 0x7F;
		n = mmc_send_command(CMD55, 0);
		if (n > 1) return n;
	}
	
	mmc_deselect();
	mmc_select();
	
	// Send a command packet:
	mmc_spi_transfer(0x40 | cmd);			// Start + Command index
	mmc_spi_transfer((BYTE)(arg >> 24));	// Argument[31..24]
	mmc_spi_transfer((BYTE)(arg >> 16));	// Argument[23..16]
	mmc_spi_transfer((BYTE)(arg >> 8));		// Argument[15..8]
	mmc_spi_transfer((BYTE)arg);			// Argument[7..0]
	if (cmd == CMD0)
		mmc_spi_transfer(0x95);		// (valid CRC for CMD0(0))
	else if (cmd == CMD8)
		mmc_spi_transfer(0x87);		// (valid CRC for CMD8(0x1AA))
	else mmc_spi_transfer(0x01);	// Dummy CRC + Stop

	// Receive command response:
	if (cmd == CMD12) d = mmc_spi_transfer(0xFF);	// Skip a stuff byte when stop reading
	
	d = 0;
	n = 10;						
	do
		d = mmc_spi_transfer( 0xFF );
	while ((d & 0x80) && --n); // Wait for a valid response in timeout of 10 attempts
	
	return d;	// Return with the response value
}



/************************************************************************/
/* FATFS RELATED FUNCTIONS                                              */
/************************************************************************/

DSTATUS disk_status (
	BYTE drv			/* Drive number (always 0) */
)
{
	DSTATUS s;
	BYTE d;

	if (drv) return STA_NOINIT;

	/* Check if the card is kept initialized */
	s = Stat;
	if (!(s & STA_NOINIT)) {
		if (mmc_send_command(CMD13, 0))	/* Read card status */
			s = STA_NOINIT;
		d = mmc_spi_transfer(0xFF);		/* Receive following half of R2 */
		mmc_deselect();
	}
	Stat = s;

	return s;
}

DSTATUS disk_initialize (
	BYTE drv		/* Physical drive number (0) */
)
{
	BYTE ty = 0, cmd, response, rbuf[4];
	unsigned int tmr;
	DSTATUS s;

	if (drv) return RES_NOTRDY;

	mmc_init_spi();
	
	// send 80 dummy clocks:
	for (int i = 0; i<10; i++)
	{
		mmc_spi_transfer(0xFF); 
	}

	if( ( response = mmc_send_command(CMD0,0) ) == 0x1 )
	{ // Card is in idle state
		if( (response = mmc_send_command( CMD8, 0x1AA )) == 1 )
		{
			rbuf[0] = mmc_spi_transfer(0xFF); // R7 type of response
			rbuf[1] = mmc_spi_transfer(0xFF);
			rbuf[2] = mmc_spi_transfer(0xFF);
			rbuf[3] = mmc_spi_transfer(0xFF);
			mmc_deselect();
			
			if (rbuf[2] == 0x01 && rbuf[3] == 0xAA) {		/* The card can work at vdd range of 2.7-3.6V */
				for (tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state (ACMD41 with HCS bit) */
					if (mmc_send_command(ACMD41, 1UL << 30) == 0) break;
					aery::delay_us(1000);
				}
				if (tmr && mmc_send_command(CMD58, 0) == 0) {	/* Check CCS bit in the OCR */
					rbuf[0] = mmc_spi_transfer(0xFF);
					rbuf[1] = mmc_spi_transfer(0xFF);
					rbuf[2] = mmc_spi_transfer(0xFF);
					rbuf[3] = mmc_spi_transfer(0xFF);
					mmc_deselect();
					
					ty = (rbuf[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if (mmc_send_command(ACMD41, 0) <= 1) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			for (tmr = 1000; tmr; tmr--) {	/* Wait for leaving idle state */
				if (mmc_send_command(cmd, 0) == 0) break;
				aery::delay_us(1000);
			}
			if (!tmr || mmc_send_command(CMD16, 512) != 0)
				ty = 0;	/* Set R/W block length to 512 */
				
		}
	}
	
	CardType = ty;
	s = ty ? 0 : STA_NOINIT;
	Stat = s;

	// if card was initialized use 22MHz clock for SPI transfers:
	if( ty )
	{
		mmc_init_spi( 0x2 );
	}


	return s;
}

DRESULT disk_read (
	BYTE drv,			/* Physical drive number (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..128) */
)
{
	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
	if (!count) return RES_PARERR;
	
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */

	if (count == 1) {	/* Single block read */
		mmc_send_command(CMD17, sector);	/* READ_SINGLE_BLOCK */
		mmc_read_datablock(buff, 512);
		mmc_wait_pdca_finish(); // wait for spi line to be free
	}
	else {				/* Multiple block read */
		if (mmc_send_command(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			multiBlockTransfer = true;
			multiBlockTransferCounter = count;
			multiBlockTransferBuffer = buff;
			mmc_read_datablock( buff, 512 ); // start by reading first block
		}
	}
	
	return RES_OK;
}

DRESULT disk_write (
	BYTE drv,			/* Physical drive number (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..128) */
)
{
	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
	
	if (!count) return RES_PARERR;
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */

	if (count == 1) {	/* Single block write */
		if ((mmc_send_command(CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& mmc_send_datablock(buff, 0xFE))
			count = 0;
	}
	else {				/* Multiple block write */
		if (CardType & CT_SDC) mmc_send_command(ACMD23, count);
		if (mmc_send_command(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!mmc_send_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!mmc_send_datablock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	mmc_deselect();

	return count ? RES_ERROR : RES_OK;
}

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive number (0)*/
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	DWORD cs;


	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;	/* Check if card is in the socket */

	res = RES_ERROR;
	switch (ctrl) {
		case CTRL_SYNC :		/* Make sure that no pending write process */
			if (mmc_select()) {
				mmc_deselect();
				res = RES_OK;
			}
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			if ((mmc_send_command(CMD9, 0) == 0) && mmc_read_datablock(csd, 16)) {
				if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
					cs = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 8) + 1;
					*(DWORD*)buff = cs << 10;
				} else {					/* SDC ver 1.XX or MMC */
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					cs = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = cs << (n - 9);
				}
				res = RES_OK;
			}
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			*(DWORD*)buff = 128;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	mmc_deselect();

	return res;
}

DWORD get_fattime(void)
{
	return 0; // since no clock is used, 0 is returned
}