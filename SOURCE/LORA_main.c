#include <stdio.h>
#include <stdio.h>
#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>


#define SPIport	1
#define SPI_CHANNEL	2
#define SPIspeed	1000000
#define SPImode	0
#define CS_PIN 16 // GPIO16 (pin 36), ručno kontroliran CS
#define RST_PIN	17



void rfm95_write_register(uint8_t reg, uint8_t value) {
	uint8_t buffer[2];

	// First byte: Register address with write flag (MSB = 0)
	buffer[0] = reg & 0x7F; // Ensure MSB is 0 for write operation

	// Second byte: Data to write
	buffer[1] = value;

	// Send the buffer via SPI
	if (wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2) == -1) {
		printf("Error writing to RFM95 register\n");
	}
}

uint8_t rfm95_read_register(uint8_t reg) {
		uint8_t buffer[2];

	// First byte: Register address with read flag (MSB = 1)
	buffer[0] = reg | 0x80; // Ensure MSB is 1 for read operation

	// Second byte: Placeholder for received data
	buffer[1] = 0x00;

	// Send the buffer via SPI and receive data
	if (wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2) == -1) {
		printf("Error reading from RFM95 register\n");
		return 0;  // Return 0 on error
	}

	// Return the received data (second byte)
	return buffer[1];
}


void rfm95_init(void) {
	// Resetovanje modula
	digitalWrite(RST_PIN, LOW);
	delayMicroseconds(100);
	digitalWrite(RST_PIN, HIGH);
	delay(5);

	// Provjera verzije
	uint8_t version = rfm95_read_register(0x42);
	if (version != 0x12) {
		printf("Greška: Neispravan RFM95 modul! Ver: 0x%X\n", version);
		return;
	}

	// Postavite LoRa mod
	rfm95_write_register(0x01, 0x80); // LoRa + Sleep
	delay(10);

	// Postavite frekvenciju (868 MHz)
	rfm95_write_register(0x06, 0xD9); // MSB
	rfm95_write_register(0x07, 0x00); // MID
	rfm95_write_register(0x08, 0x00); // LSB

	// Konfiguracija modema
	rfm95_write_register(0x1D, 0x72); // ModemConfig1: BW=125kHz, CR=4/5
	rfm95_write_register(0x1E, 0x74); // ModemConfig2: SF=7

	// Postavite snagu
	rfm95_write_register(0x09, 0xFF); // Maksimalna snaga (17 dBm)

	// Postavite preambulu
	rfm95_write_register(0x20, 0x00); // Preambula MSB
	rfm95_write_register(0x21, 0x08); // Preambula LSB

	// Aktivirajte Continuous RX mod
	rfm95_write_register(0x01, 0x85); // LoRa + RX Continuous
}



int main(void)
{

	int csPin = 16; // GPIO16 (pin 36), ručno kontroliran CS

	if (wiringPiSetup() == -1)
		printf("WiringPI setup failed! \n");
		
	
	// Inicijalizacija WiringPi GPIO sistema
	if (wiringPiSetupGpio() == -1) 
		fprintf(stderr, "Failed to initialize WiringPi.\n");

	
	
	
	pinMode(csPin, OUTPUT);
	digitalWrite(csPin, HIGH); // Postavite CS na HIGH

	// Inicijalizacija SPI1 bez hardverskog upravljanja CS-om
	//int fd = wiringPiSPISetupMode(spiChannel, speed, mode);
	//int wiringPiSPIxSetupMode(const int number, const int channel, const int speed, const int mode)

	int fd = wiringPiSPIxSetupMode(SPIport, SPI_CHANNEL, SPIspeed, SPImode);
	if (fd < 0) 
		fprintf(stderr, "Failed to initialize SPI1.\n");
		
	void rfm95_init(void);


	while (1) {

		sleep(1); // Pauza od 1 sekunde
		
		
	}
	



}