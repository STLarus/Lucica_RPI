/**
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


#include "../INCLUDE/EVE.h"
#include "../INCLUDE/HAL.h"
#include "../INCLUDE/MCU.h"
#include "../INCLUDE/FT8xx.h"


#define SPI_CHANNEL 1  // SPI kanal koji koristite (0 ili 1)
#define SPI_SPEED 10000000  // Brzina SPI-a (10MHz)

// Used to navigate command ring buffer
static uint16_t writeCmdPointer = 0x0000;


//INICIJALIZACIJA SPI PORTA
#define CONFIG_FILE "/boot/config.txt"
#define DTO_OVERLAY "dtoverlay=spi1-3cs"

void addSpiOverlay() {
	FILE* file = fopen(CONFIG_FILE, "r+");
	if (file == NULL) {
		perror("Otvaranje config.txt nije uspjelo");
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



int SPI1_enable() {
	// Dodavanje SPI1 overlay-a u config.txt
	addSpiOverlay();

	return 0;
}


void HAL_EVE_Init(void)
{
	uint8_t val;

	MCU_Init();

	// Set Chip Select OFF
	HAL_ChipSelect(0);

	// Reset the display
	MCU_Delay_20ms();
	HAL_PowerDown(1);
	MCU_Delay_20ms();
	HAL_PowerDown(0);
	MCU_Delay_20ms();

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

	//	MCU_Delay_500ms();		// Optional delay can be commented so long as we check the REG_ID and REG_CPURESET

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
		MCU_PDlow();
	else
		MCU_PDhigh();
}


// ################# COMBINED ADDRESSING AND DATA FUNCTIONS ####################
void HAL_ChipSelect(int8_t state)
{

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

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, data, 4);
	//CS_High();
}
// -------------- Write a 16-bit value  --------------------
void HAL_Write16(uint16_t val16)
{
	
	uint8_t data[2];


	data[0] = (uint8_t)(val16 >> 8);
	data[1] = (uint8_t)(val16);

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, data, 2);
	//CS_High();
}

// -------------- Write an 8-bit value  --------------------
void HAL_Write8(uint8_t val8)
{
	uint8_t data[1];


	data[0] = (uint8_t)(val8);

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, data, 1);
	//CS_High();
}

// -------------- Write a 32-bit value to specified address --------------------
void HAL_MemWrite32(uint32_t address, uint32_t val32)
{
	uint8_t addr[3];
	uint8_t data[4];

	addr[0] = (uint8_t)(address >> 16);
	addr[1] = (uint8_t)(address >> 8);
	addr[2] = (uint8_t)(address);

	data[0] = (uint8_t)(val32 >> 24);
	data[1] = (uint8_t)(val32 >> 16);
	data[2] = (uint8_t)(val32 >> 8);
	data[3] = (uint8_t)(val32);

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, addr, 3);
	wiringPiSPIDataRW(SPI_CHANNEL, data, 4);
	//CS_High();
}

// -------------- Write a 16-bit value to specified address --------------------
void HAL_MemWrite16(uint32_t address, uint16_t val16)
{
	uint8_t addr[3];
	uint8_t data[2];

	addr[0] = (uint8_t)(address >> 16);
	addr[1] = (uint8_t)(address >> 8);
	addr[2] = (uint8_t)(address);

	data[0] = (uint8_t)(val16 >> 8);
	data[1] = (uint8_t)(val16);

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, addr, 3);
	wiringPiSPIDataRW(SPI_CHANNEL, data, 2);
	//CS_High();
}

// -------------- Write an 8-bit value to specified address --------------------
void HAL_MemWrite8(uint32_t address, uint8_t val8)
{
	uint8_t addr[3];
	uint8_t data[1];

	addr[0] = (uint8_t)(address >> 16);
	addr[1] = (uint8_t)(address >> 8);
	addr[2] = (uint8_t)(address);

	data[0] = (uint8_t)(val8);

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, addr, 3);
	wiringPiSPIDataRW(SPI_CHANNEL, data, 1);
	//CS_High();
}

// -------------- Read a 32-bit value from specified address --------------------
uint32_t HAL_MemRead32(uint32_t address)
{
	uint8_t addr[3];
	uint8_t data[4] = { 0,0,0,0 };
	uint32_t retval;

	addr[0] = (uint8_t)(address >> 16);
	addr[1] = (uint8_t)(address >> 8);
	addr[2] = (uint8_t)(address);

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, addr, 3);
	wiringPiSPIDataRW(SPI_CHANNEL, data, 4);
	//CS_High();

	retval = ((uint32_t)data[0] << 24) |
		((uint32_t)data[1] << 16) |
		((uint32_t)data[2] << 8) |
		((uint32_t)data[3]);

	return retval;

}
// -------------- Read a 16-bit value from specified address --------------------
uint16_t HAL_MemRead16(uint32_t address)
{
	uint8_t addr[3];
	uint8_t data[2] = { 0,0 };
	uint16_t retval;

	addr[0] = (uint8_t)(address >> 16);
	addr[1] = (uint8_t)(address >> 8);
	addr[2] = (uint8_t)(address);

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, addr, 3);
	wiringPiSPIDataRW(SPI_CHANNEL, data, 2);
	//CS_High();

	retval = ((uint16_t)data[0] << 8) | ((uint16_t)data[0]);

	return retval;

}
// -------------- Read an 8-bit value from specified address --------------------
uint8_t HAL_MemRead8(uint32_t address)
{
	uint8_t addr[3];
	uint8_t data[1] = { 0 };
	uint8_t retval;

	addr[0] = (uint8_t)(address >> 16);
	addr[1] = (uint8_t)(address >> 8);
	addr[2] = (uint8_t)(address);

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, addr, 3);
	wiringPiSPIDataRW(SPI_CHANNEL, data, 1);
	//CS_High();

	retval = ((uint8_t)data[0]);

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

	//CS_Low();
	wiringPiSPIDataRW(SPI_CHANNEL, data, 3);
	//CS_High();

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
