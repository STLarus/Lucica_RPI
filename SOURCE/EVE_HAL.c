/**
 @file EVE_HAL.c
 */


 /* Only compile for non-linux platforms or when MPSSE is being used. */
#if !defined(USE_LINUX_SPI_DEV) || defined(USE_MPSSE)

#include <string.h>
#include <stdint.h> // for Uint8/16/32 and Int8/16/32 data types

#include "../INCLUDE/EVE_config.h"
#include "../INCLUDE/FT8xx.h"
#include "../INCLUDE/HAL.h"
#include "../INCLUDE/MCU.h"
#include "../INCLUDE/EVE_config.h"
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define CS_LOW()	digitalWrite(CS_PIN, LOW)
#define CS_HIGH()	digitalWrite(CS_PIN, HIGH)


// Used to navigate command ring buffer
static uint16_t writeCmdPointer = 0x0000;


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
	//MCU_Setup();
}

// --------------------- Chip Select line ----------------------------------
void HAL_ChipSelect(int8_t enable)
{
	if (enable)
		MCU_CSlow();
	else
		MCU_CShigh();
}

// -------------------------- Power Down line --------------------------------------
void HAL_PowerDown(int8_t enable)
{
	if (enable)
		MCU_PDlow();
	else
		MCU_PDhigh();
}

// ------------------ Send FT81x register address for writing ------------------
void HAL_SetWriteAddress(uint32_t address)
{
	// Send three bytes of a register address which has to be subsequently
	// written. Ignore return values as this is an SPI write only.
	// Send high byte of address with 'write' bits set.
	uint8_t data[4] = { 0, 0, 0, 0 };
	data[0] = (uint8_t)(address >> 16);
	data[0] = data[0] | 0x80; //???? da li ovo treba ?
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 3);
}

// ------------------ Send FT81x register address for reading ------------------
void HAL_SetReadAddress(uint32_t address)
{
	// Send three bytes of a register address which has to be subsequently read.
	// Ignore return values as this is an SPI write only.
	// Send high byte of address with 'read' bits set.
	uint8_t data[4] = { 0, 0, 0, 0 };
	data[0] = (uint8_t)(address >> 16);
	data[0] = data[0] & 0x3F; //???? da li ovo treba ?
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 3);
}

// ------------------------ Send a block of data --------------------------

void HAL_Write(const uint8_t* buffer, uint32_t length)
{
	// Send multiple bytes of data after previously sending address. Ignore return
	// values as this is an SPI write only. Data must be the correct endianess
	// for the SPI bus.
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, buffer, length);
}

// ------------------------ Send a 32-bit data value --------------------------
void HAL_Write32(uint32_t val32)
{
	uint8_t data[4] = { 0, 0, 0, 0 };
	data[3] = (uint8_t)(val32 >> 24);
	data[2] = (uint8_t)(val32 >> 16);
	data[1] = (uint8_t)(val32 >> 8);
	data[0] = (uint8_t)(val32);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 4);
}

// ------------------------ Send a 16-bit data value --------------------------
void HAL_Write16(uint16_t val16)
{
	uint8_t data[4] = { 0, 0, 0, 0 };
	data[1] = (uint8_t)(val16 >> 8);
	data[0] = (uint8_t)(val16);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 2);
}

// ------------------------ Send an 8-bit data value ---------------------------
void HAL_Write8(uint8_t val8)
{
	// Send one byte of data after previously sending address. Ignore return
	// values as this is an SPI write only.
	uint8_t data[4] = { 0, 0, 0, 0 };
	
	data[0] = val8;
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 1);
}

// ------------------------ Read a 32-bit data value --------------------------
uint32_t HAL_Read32(void)
{
	// Read 4 bytes from a register has been previously addressed. Send dummy
	// 00 bytes as only the incoming value is important.

	uint8_t buf[4] = { 0, 0, 0, 0 };
	uint32_t val32;

	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, buf, 4);

	// Read low byte of data first.
	//val32 = buf[0] + ((uint32_t)buf[1] << 8) + ((uint32_t)buf[2] << 16) + ((uint32_t)buf[3] << 24);
	val32 = buf[3] + ((uint32_t)buf[2] << 8) + ((uint32_t)buf[1] << 16) + ((uint32_t)buf[0] << 24);

	// Return combined 32-bit value
	return val32;
}

// ------------------------ Read a 16-bit data value ---------------------------
uint16_t HAL_Read16(void)
{
	// Read 4 bytes from a register has been previously addressed. Send dummy
	// 00 bytes as only the incoming value is important.

	uint8_t buf[4] = { 0, 0, 0, 0 };
	uint16_t val16;

	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, buf, 2);

	// Read low byte of data first.
	//val16 = buf[0] + ((uint16_t)buf[1] << 8);
	val16 = buf[1] + ((uint16_t)buf[0] << 8);

	// Return combined 32-bit value
	return val16;
}

// ------------------------ Read an 8-bit data value ---------------------------
uint8_t HAL_Read8(void)
{
	uint8_t buf[4] = { 0, 0, 0, 0 };

	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, buf, 1);

	return buf[0];
}

// ################# COMBINED ADDRESSING AND DATA FUNCTIONS ####################

// This section has combined calls which carry out a full write or read cycle
// including chip select, address, and data transfer.
// This would often be used for register writes and reads. 

// -------------- Write a 32-bit value to specified address --------------------
void HAL_MemWrite32(uint32_t address, uint32_t val32)
{
	uint8_t data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	data[0] = (uint8_t)(address >> 16);
	data[0] = data[0] | 0x80; //???? da li ovo treba ?
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	/*data[3] = (uint8_t)(val32 >> 24);
	data[4] = (uint8_t)(val32 >> 16);
	data[5] = (uint8_t)(val32 >> 8);
	data[6] = (uint8_t)(val32);*/
	data[3] = (uint8_t)(val32); 
	data[4] = (uint8_t)(val32 >> 8);
	data[5] = (uint8_t)(val32 >> 16); 
	data[6] = (uint8_t)(val32 >> 24);
	
	
	

	CS_LOW();
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 7);
	CS_HIGH();
}

// -------------- Write a 16-bit value to specified address --------------------
void HAL_MemWrite16(uint32_t address, uint16_t val16)
{
	uint8_t data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	data[0] = (uint8_t)(address >> 16);
	data[0] = data[0] | 0x80; //???? da li ovo treba ?
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	/*data[3] = (uint8_t)(val16 >> 8);
	data[4] = (uint8_t)(val16);*/
	data[3] = (uint8_t)(val16);
	data[4] = (uint8_t)(val16 >> 8);
	
	CS_LOW();
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 5);
	CS_HIGH();
}

// -------------- Write an 8-bit value to specified address --------------------
void HAL_MemWrite8(uint32_t address, uint8_t val8)
{
	uint8_t data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	data[0] = (uint8_t)(address >> 16);
	data[0] = data[0] | 0x80; //???? da li ovo treba ?
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	data[3] = (uint8_t)(val8);

	CS_LOW();
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 4);
	CS_HIGH();
}

// -------------- Read a 32-bit value from specified address --------------------
uint32_t HAL_MemRead32(uint32_t address)
{
	uint32_t val32;

	uint8_t data[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	data[0] = (uint8_t)(address >> 16);
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	data[3] = 0; //DUMMY BYTE
	
	CS_LOW();
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 8);
	CS_HIGH();

	//val32 = data[7] + ((uint32_t)data[6] << 8) + ((uint32_t)data[5] << 16) + ((uint32_t)data[4] << 24);
	val32 = data[4] + ((uint32_t)data[5] << 8) + ((uint32_t)data[6] << 16) + ((uint32_t)data[7] << 24);
	return val32;
}
// -------------- Read a 16-bit value from specified address --------------------
uint16_t HAL_MemRead16(uint32_t address)
{
	uint16_t val16;

	
	uint8_t data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	data[0] = (uint8_t)(address >> 16);
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	data[3] = 0; //DUMMY BYTE

	CS_LOW();
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 6);
	CS_HIGH();

	//val16 = data[5] + ((uint16_t)data[4] << 8);
	val16 = data[4] + ((uint16_t)data[5] << 8);
	return val16;
}
// -------------- Read an 8-bit value from specified address --------------------
uint8_t HAL_MemRead8(uint32_t address)
{
	uint8_t val8;

	uint8_t data[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	data[0] = (uint8_t)(address >> 16);
	data[1] = (uint8_t)(address >> 8);
	data[2] = (uint8_t)(address);
	data[3] = 0; //DUMMY BYTE

	CS_LOW();
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 5);
	CS_HIGH();

	val8 = data[4];
	return val8;
}
// ############################# HOST COMMANDS #################################
// -------------------------- Write a host command -----------------------------
void HAL_HostCmdWrite(uint8_t cmd, uint8_t param)
{
	uint8_t data[4] = { 0, 0, 0, 0 };
	data[0] = cmd;
	data[1] = param;
	data[2] = 0; // DUMMY BYTE;
	CS_LOW();
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 3);
	CS_HIGH();

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


void jebiga(void)
{
	while (1)
	{
		HAL_MemRead8(EVE_REG_ID);
		usleep(10000);
	}

}

#endif // __linux__