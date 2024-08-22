
#include <string.h>
#include <stdint.h> // for Uint8/16/32 and Int8/16/32 data types
#include <stdio.h>
#include <endian.h>

#include "../INCLUDE/MCU.h"
#include "../INCLUDE/EVE_config.h"
#include <wiringPi.h>
#include <wiringPiSPI.h>


#define PD_PIN	15	//GPIO14
#define CS_PIN	27	//GPIO16	//pin na koji je spojen CS
#define CS_PORT	28	//GPIO12	//chip celect pin (lažni je pin, na njega ništa nije spojeno. Koristi se da bi SPI1 mogao radiit sa ručnom kontrolom CS pina
#define SPI_CHANNEL	1
#define SPI_SPEED	1000000
#define SPI_MODE	0

// ------------------- MCU specific initialisation  ----------------------------
void MCU_Init(void)
{
	if (wiringPiSetup() == -1)
	{
		printf("WiringPI setup failure \n");
		return 1;
	}
	wiringPiSetupGpio();
	pinMode(CS_PIN , OUTPUT);
	digitalWrite(CS_PIN, HIGH);
	pinMode(PD_PIN, OUTPUT);
	int fd1 = wiringPiSPIxSetupMode(SPI_CHANNEL, CS_PORT, SPI_SPEED, SPI_MODE);
	if (fd1 == -1)
	{
		printf("SPI1 setup failure \n");
		return 1;
	}
}


// ########################### GPIO CONTROL ####################################

// --------------------- Chip Select line low ----------------------------------
inline void MCU_CSlow(void)
{
	digitalWrite(CS_PIN, LOW);
}

// --------------------- Chip Select line high ---------------------------------
inline void MCU_CShigh(void)
{
	digitalWrite(CS_PIN, HIGH);
}

// -------------------------- PD line low --------------------------------------
inline void MCU_PDlow(void)
{
	digitalWrite(PD_PIN, LOW);
}

// ------------------------- PD line high --------------------------------------
inline void MCU_PDhigh(void)
{
	digitalWrite(PD_PIN, HIGH);
}

// --------------------- SPI Send and Receive ----------------------------------

uint8_t MCU_SPIRead8(void)
{
	uint8_t data[2];
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 1);
	return data[0];
}

void MCU_SPIWrite8(uint8_t DataToWrite)
{
	uint8_t data[2];
	data[0] = DataToWrite;
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 1);
}

uint16_t MCU_SPIRead16(void)
{
	uint16_t retval;
	uint8_t data[4] = { 0,0,0,0 };
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 2);

	retval = ((uint16_t)data[0] << 8) | ((uint16_t)data[1]);
	return retval;
}

void MCU_SPIWrite16(uint16_t val16)
{
	uint8_t data[4] = { 0,0,0,0 };
	data[0] = (uint8_t)(val16 >> 8);
	data[1] = (uint8_t)(val16);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 2);
}

uint32_t MCU_SPIRead24(void)
{
	uint32_t retval;
	uint8_t data[4] = { 0,0,0,0 };
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 3);
	retval = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | ((uint32_t)data[2]);
	return retval;
}

void MCU_SPIWrite24(uint32_t val24)
{
	uint8_t data[4] = { 0,0,0,0 };
	data[0] = (uint8_t)(val24 >> 16);
	data[1] = (uint8_t)(val24 >> 8);
	data[2] = (uint8_t)(val24);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 3);
}

uint32_t MCU_SPIRead32(void)
{
	uint32_t retval;
	uint8_t data[5] = { 0,0,0,0,0 };
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 4);
	retval = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | ((uint32_t)data[3]);
	return retval;
}

void MCU_SPIWrite32(uint32_t val32)
{
	uint8_t data[4] = { 0,0,0,0 };
	data[0] = (uint8_t)(val32 >> 24);
	data[1] = (uint8_t)(val32 >> 16);
	data[2] = (uint8_t)(val32 >> 8);
	data[3] = (uint8_t)(val32);
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, data, 4);
}

void MCU_SPIWrite(const uint8_t* DataToWrite, uint32_t length)
{
	wiringPiSPIxDataRW(SPI_CHANNEL, CS_PORT, DataToWrite, length);
}

void MCU_Delay_20ms(void)
{
	usleep(20 * 1000);
}

void MCU_Delay_500ms(void)
{
	usleep(500 * 1000);
}

// RPI ZERO W is Little Endian.
// Use toolchain defined functions.
uint16_t MCU_htobe16(uint16_t h)
{
	return htobe16(h);
}

uint32_t MCU_htobe32(uint32_t h)
{
	return htobe32(h);
}

uint16_t MCU_htole16(uint16_t h)
{
	return htole16(h);
}

uint32_t MCU_htole32(uint32_t h)
{
	return htole32(h);
}

uint16_t MCU_be16toh(uint16_t h)
{
	return be16toh(h);
}

uint32_t MCU_be32toh(uint32_t h)
{
	return be32toh(h);;
}

uint16_t MCU_le16toh(uint16_t h)
{
	return le16toh(h);
}

uint32_t MCU_le32toh(uint32_t h)
{
	return le32toh(h);
}

