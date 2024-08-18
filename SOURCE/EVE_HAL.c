﻿/**
 @file EVE_HAL.c
 */
 /*
  * ============================================================================
  * History
  * =======
  * Nov 2019		Initial beta for FT81x and FT80x
  * Mar 2020		Updated beta - added BT815/6 commands
  * Mar 2021		Beta with BT817/8 support added
  *
  */

  /* Only compile for non-linux platforms or when MPSSE is being used. */
#if !defined(USE_LINUX_SPI_DEV) || defined(USE_MPSSE)

#include <string.h>
#include <stdint.h> 
#include <stdio.h>
#include <errno.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>
//#include "../WiringPI/wiringPi.h"
//#include "../WiringPI/wiringPiSPI.h"

#include "../INCLUDE/EVE.h"
#include "../INCLUDE/HAL.h"
#include "../INCLUDE/MCU.h"
#include "../INCLUDE/FT8xx.h"


#define SPI_CHANNEL 1  // SPI kanal koji koristite (0 ili 1)
#define	CS_PORT	2
#define SPI_SPEED 1000000  // Brzina SPI-a (10MHz)

#define PD_PIN	14

// Used to navigate command ring buffer
static uint16_t writeCmdPointer = 0x0000;


//INICIJALIZACIJA SPI PORTA
#define CONFIG_FILE "/boot/config.txt"
#define DTO_OVERLAY "dtoverlay=spi1-3cs"

void jebiga(void)
{
	uint8_t addr[3];
	uint8_t data[1] = { 0 };
	uint8_t retval;

	addr[0] = 0x01;
	addr[1] = 0x55;
	addr[2] = 0x03;

	usleep(100);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, &addr[0], 3);

}

void addSpiOverlay() {

	FILE* file = fopen(CONFIG_FILE, "r+");
	if (file == NULL) {
		perror("Otvaranje config.txt nije uspjelo");
		printf("Greška kod: %d\n", errno);
		//exit(EXIT_FAILURE);
	}

	// Provjerite da li već postoji dtoverlay=spi1-3cs u datoteci
	char line[256];
	int overlayFound = 0;
	while (fgets(line, sizeof(line), file)) {
		if (strstr(line, DTO_OVERLAY) != NULL) {
			overlayFound = 1;
			break;
		}
	}

	if (!overlayFound) {
		// Dodajte dtoverlay=spi1-3cs na kraj datoteke
		fseek(file, 0, SEEK_END);
		fprintf(file, "\n%s\n", DTO_OVERLAY);
		printf("Dodano %s u config.txt\n", DTO_OVERLAY);
		system("sudo reboot");	//restart sustava
	}
	else {
		printf("%s već postoji u config.txt\n", DTO_OVERLAY);
	}

	fclose(file);
}



int SPI1_enable()
{
	// Dodavanje SPI1 overlay-a u config.txt
	//addSpiOverlay();

	return 0;
}

void MCU_Setup(void)
{

}


void MCU_Init(void)
{
	if (wiringPiSetup() == -1) {
		printf("WiringPi setup failed!\n");
		return 1;
	}
	wiringPiSetupGpio();
	pinMode(PD_PIN, OUTPUT);	//PD pin


	// Uređaj 1 na SPI1, mode 0, 1 MHz
	int fd1 = wiringPiSPIxSetupMode(SPI_CHANNEL, CS_PORT, SPI_SPEED, 0);
	if (fd1 == -1) {
		printf("Neuspjeh u inicijalizaciji SPI uređaja 1!\n");
		return -1;
	}


}

void HAL_EVE_Init(void)
{
	uint8_t val;

	MCU_Init();

	// Set Chip Select OFF
	HAL_ChipSelect(0);

	// Reset the display
	usleep(20000);
	HAL_PowerDown(1);
	usleep(20000);
	HAL_PowerDown(0);
	usleep(20000);

#if (defined EVE1_ENABLE)
	// FT80x_selection - FT80x modules from BRT generally use external crystal 
	// You can also send the host command to set the PLL here if you want to change it from the default of 48MHz (FT80x) or 60MHz (FT81x)
	// Clock selection and clock rate selection will put EVE to sleep and so must be before the Active command
	// for example:
	HAL_HostCmdWrite(0x44, 0x00); // 0x44 = HostCMD_CLKEXT
	HAL_HostCmdWrite(0x62, 0x00); // 0x64 = HostCMD_CLK48M
#endif

#if defined (EVE3_ENABLE) || defined (EVE4_ENABLE)
	// can optionally set to 72MHz system clock here
	// In this case also adjust REG_FREQUENCY a few lines down from here in this file
	HAL_HostCmdWrite(0x44, 0x00); // 0x44 = HostCMD_CLKEXT
	HAL_HostCmdWrite(0x61, 0x46);
#endif

#if defined (EVE2_ENABLE) || defined (EVE3_ENABLE)|| defined (EVE4_ENABLE)
	HAL_HostCmdWrite(0x68, 0x00); // Reset
#endif

	// Set active
	HAL_HostCmdWrite(0, 0x00);

	usleep(500000);		// Optional delay can be commented so long as we check the REG_ID and REG_CPURESET

		// Read REG_ID register (0x302000) until reads 0x7C
	while ((val = HAL_MemRead8(EVE_REG_ID)) != 0x7C)
	{
	}

	// Ensure CPUreset register reads 0 and so FT8xx is ready
	while (HAL_MemRead8(EVE_REG_CPURESET) != 0x00)
	{
	}

#if defined (EVE3_ENABLE) || defined (EVE4_ENABLE)
	HAL_MemWrite32(EVE_REG_FREQUENCY, 72000000);
#endif

	// This function will not return unless an EVE device is present.
	MCU_Setup();
}


// -------------------------- Power Down line --------------------------------------
void HAL_PowerDown(int8_t enable)
{
	if (enable)
		digitalWrite(PD_PIN, LOW);
	else
		digitalWrite(PD_PIN, HIGH);
}


// ################# COMBINED ADDRESSING AND DATA FUNCTIONS ####################
void HAL_ChipSelect(int8_t state)
{

}
void HAL_SetWriteAddress(uint32_t address) {
	// Postavljanje najvišeg bita za pisanje (0x80)
	address |= 0x800000;

	// Pretvorba adrese u bajtove
	uint8_t data[4];
	data[0] = (address >> 16) & 0xFF;  // Najviši bajt
	data[1] = (address >> 8) & 0xFF;   // Srednji bajt
	data[2] = address & 0xFF;          // Najniži bajt
	data[3] = 0x00;  // Naredba za pisanje (može biti specifično za vaš uređaj)

	// Slanje adrese putem SPI
	if (wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 4) == -1) {
		printf("Neuspješno slanje SPI podataka\n");
	}
}


/*
Imam mogućnost ds s eupravlja sa CS linijom  a tada se koriste
	digitalWrite(CS_PIN, LOW);
	// Slanje i primanje podataka preko SPI-a
	spiWriteRead(data, 4);
	// Ručno podizanje CS signala
	digitalWrite(CS_PIN, HIGH);

	Treba i podesiti RPI da koristi SPI1
*/
// -------------- Write a 32-bit value  --------------------
void HAL_Write32(uint32_t val32)
{
	uint8_t data[4];

	data[0] = (uint8_t)(val32 >> 24);
	data[1] = (uint8_t)(val32 >> 16);
	data[2] = (uint8_t)(val32 >> 8);
	data[3] = (uint8_t)(val32);

	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 4);

}
// -------------- Write a 16-bit value  --------------------
void HAL_Write16(uint16_t val16)
{

	uint8_t data[2];


	data[0] = (uint8_t)(val16 >> 8);
	data[1] = (uint8_t)(val16);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 2);

}

// -------------- Write an 8-bit value  --------------------
void HAL_Write8(uint8_t val8)
{
	uint8_t data[1];

	data[0] = (uint8_t)(val8);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 1);

}

// -------------- Write a 32-bit value to specified address --------------------
void HAL_MemWrite32(uint32_t address, uint32_t val32)
{

	uint8_t data[8];

	data[0] = (uint8_t)(address >> 16);
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);

	data[3] = (uint8_t)(val32 >> 24);
	data[4] = (uint8_t)(val32 >> 16);
	data[5] = (uint8_t)(val32 >> 8);
	data[6] = (uint8_t)(val32);


	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 7);

}

// -------------- Write a 16-bit value to specified address --------------------
void HAL_MemWrite16(uint32_t address, uint16_t val16)
{

	uint8_t data[6];

	data[0] = (uint8_t)(address >> 16);
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);

	data[3] = (uint8_t)(val16 >> 8);
	data[4] = (uint8_t)(val16);

	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 5);

}

// -------------- Write an 8-bit value to specified address --------------------
void HAL_MemWrite8(uint32_t address, uint8_t val8)
{

	uint8_t data[5];

	data[0] = (uint8_t)(address >> 16);
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);

	data[3] = (uint8_t)(val8);

	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 4);
}

// -------------- Read a 32-bit value from specified address --------------------
uint32_t HAL_MemRead32(uint32_t address)
{

	uint8_t data[8] = { 0,0,0,0,0,0,0,0 };
	uint32_t retval;

	data[0] = (uint8_t)(address >> 16);
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	data[3] = 0x00;		//dummy
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 8);


	retval = ((uint32_t)data[4] << 24) |
		((uint32_t)data[5] << 16) |
		((uint32_t)data[6] << 8) |
		((uint32_t)data[7]);

	return retval;

}
// -------------- Read a 16-bit value from specified address --------------------
uint16_t HAL_MemRead16(uint32_t address)
{

	uint8_t data[6] = { 0,0 ,0,0,0,0 };
	uint16_t retval;

	data[0] = (uint8_t)(address >> 16);
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	data[3] = 0x00;	//dummy
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 6);

	retval = ((uint16_t)data[4] << 8) | ((uint16_t)data[5]);

	return retval;
}


// -------------- Read an 8-bit value from specified address --------------------
uint8_t HAL_MemRead8(uint32_t address)
{
	uint8_t data[8];
	uint8_t retval;

	data[0] = (uint8_t)(address >> 16);
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	data[3] = 0;	//DUMMY byte
	data[4] = 0;	

	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 5);
	retval = ((uint8_t)data[4]);
	return retval;
}
// ############################# HOST COMMANDS #################################
// -------------------------- Write a host command -----------------------------
void HAL_HostCmdWrite(uint8_t cmd, uint8_t param)
{

	uint8_t data[3];

	data[0] = cmd;
	data[1] = param;
	data[2] = 0x00;
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 3);


}
// ######################## SUPPORTING FUNCTIONS ###############################

// --------- Increment co-processor address offset counter --------------------
void HAL_IncCmdPointer(uint16_t commandSize)
{
	// Calculate new offset
	writeCmdPointer = (writeCmdPointer + commandSize) & (EVE_RAM_CMD_SIZE - 1);
}

// --------- Increment co-processor address offset counter --------------------
uint16_t HAL_GetCmdPointer(void)
{
	// Return new offset
	return writeCmdPointer;
}

void HAL_WriteCmdPointer(void)
{
	// and move write pointer to here
	HAL_MemWrite32(EVE_REG_CMD_WRITE, writeCmdPointer);
}

// ------ Wait for co-processor read and write pointers to be equal ------------
uint8_t HAL_WaitCmdFifoEmpty(void)
{
	uint32_t readCmdPointer;

	// Wait until the two registers match
	do
	{
		// Read the graphics processor read pointer
		readCmdPointer = HAL_MemRead32(EVE_REG_CMD_READ);

	} while ((writeCmdPointer != readCmdPointer) && (readCmdPointer != 0xFFF));


	if (readCmdPointer == 0xFFF)
	{
		// Return 0xFF if an error occurred
		return 0xFF;
	}
	else
	{
		// Return 0 if pointers became equal successfully
		return 0;
	}
}
// ------------ Check how much free space is available in CMD FIFO -------------
uint16_t HAL_CheckCmdFreeSpace(void)
{
	uint32_t readCmdPointer = 0;
	uint16_t Fullness, Freespace;

	// Check the graphics processor read pointer
	readCmdPointer = HAL_MemRead32(EVE_REG_CMD_READ);

	// Fullness is difference between MCUs current write pointer value and the FT81x's REG_CMD_READ
	Fullness = ((writeCmdPointer - (uint16_t)readCmdPointer) & (EVE_RAM_CMD_SIZE - 1));
	// Free Space is 4K - 4 - Fullness (-4 avoids buffer wrapping round)
	Freespace = (EVE_RAM_CMD_SIZE - 4) - Fullness;

	return Freespace;
}

#endif // __linux__
