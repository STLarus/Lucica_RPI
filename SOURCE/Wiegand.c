#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>

#define D0_PIN 23  // WiringPi D0
#define D1_PIN 22  // WiringPI D1
#define TIMEOUT_MS 100 // Timeout u milisekundama

volatile int bitCount = 0;
volatile unsigned long data = 0;
volatile bool timeout = false;
uint8_t rdrBuf[64],rdrCount;
uint32_t CardCode;

// Funkcija za upravljanje timeout-om
void* timeoutHandler(void* arg) {
    usleep(TIMEOUT_MS * 1000); // Pretvori u mikrosekunde
    timeout = true;
    return NULL;
}

void d0Interrupt(void) {
	rdrBuf[bitCount++] = 0;
    timeout = false;  // Resetuj timeout
}

void d1Interrupt(void) {
	rdrBuf[bitCount++] = 1; 
	timeout = false;  // Resetuj timeout
}


/************************************************************************
Function:       TestParitet
Purpose:        Testira paritet procitanih podataka sa kartice.
Parameters:     Pointer na bafer u kojem su smjesteni podaci
Returns:        1...ako je paritet ispravan
		0...ako je paritet neispravan
************************************************************************/
unsigned char TestParitet26(unsigned char *pBuf)
{
	unsigned char Lpar, Rpar, cnt;
	Lpar = 0;
	Rpar = 0;
	for (cnt = 1; cnt < 13; cnt++)
		Lpar += *(pBuf + cnt);
	Lpar = Lpar & 0x01;
	if (Lpar != *(pBuf + 0))
		return 0;
	for (cnt = 13; cnt < 25; cnt++)
		Rpar += *(pBuf + cnt);
	Rpar = Rpar & 0x01;
	if (Rpar == *(pBuf + 25))
		return 0;
	return 1;

}/* TestParitet26()*/



/********************************************************************************
Funkcija:		Weigand26
Parameteri:     tbuf.....pointer na bufer u koji su upisani podaci
Return val:     kod procitane kartice
Desc:			Funkcija pretvara pristigli array u kod kartice. Ne provjerava
				cheksum
*********************************************************************************/
unsigned long Wiegand26(unsigned char *tbuf)
{
	unsigned char cnt;
	unsigned long cCode;
	cCode = 0;
	if (TestParitet26(tbuf) == 0)
		return 0;
	for (cnt = 1; cnt < 24; cnt++) //pretvaranje arraya iz LCDStringfera u sifru
	{
		if (*(tbuf + cnt) == 1)
			cCode++;
		cCode = cCode << 1;
	}
	if (*(tbuf + 24) == 1)
		cCode++;

	return cCode;
}/****** Wiegand26() *****/

/********************************************************************************
Funkcija:		Weigand32
Parameteri:     tbuf.....pointer na bufer u koji su upisani podaci
Return val:     kod procitane kartice
Desc:			Funkcija pretvara pristigli array u kod kartice. Ne provjerava
				cheksum
*********************************************************************************/
unsigned long Wiegand32(unsigned char *tbuf)
{
	unsigned char cnt;
	unsigned long cCode;
	cCode = 0;
	for (cnt = 0; cnt < 31; cnt++) //pretvaranje arraya iz LCDStringfera u sifru
	{
		if (*(tbuf + cnt) == 1)
			cCode++;
		cCode = cCode << 1;
	}
	if (*(tbuf + 31) == 1)
		cCode++;
	return cCode;
}/****** Wiegand32() *****/

/********************************************************************************
Funkcija:		Weigand34
Parameteri:     tbuf.....pointer na bufer u koji su upisani podaci
Return val:     kod procitane kartice
Desc:			Funkcija pretvara pristigli array u kod kartice. Ne provjerava
				cheksum
*********************************************************************************/
unsigned long Wiegand34(unsigned char *tbuf)
{
	unsigned char cnt;
	unsigned long cCode;
	cCode = 0;
	for (cnt = 1; cnt < 32; cnt++) //pretvaranje arraya iz LCDStringfera u sifru
	{
		if (*(tbuf + cnt) == 1)
			cCode++;
		cCode = cCode << 1;
	}
	if (*(tbuf + 32) == 1)
		cCode++;
	return cCode;
}/****** Wiegand34() *****/


/********************************************************************************
Funkcija:		Weigand37
Parameteri:     tbuf.....pointer na bufer u koji su upisani podaci
Return val:     kod procitane kartice
Desc:			Funkcija pretvara pristigli array u kod kartice. Ne provjerava
				cheksum
*********************************************************************************/
unsigned long Wiegand37(unsigned char *tbuf)
{
	unsigned char cnt;
	unsigned long cCode;
	cCode = 0;
	for (cnt = 17; cnt < 35; cnt++) //pretvaranje arraya iz LCDStringfera u sifru
	{
		if (*(tbuf + cnt) == 1)
			cCode++;
		cCode = cCode << 1;
	}
	if (*(tbuf + 35) == 1)
		cCode++;
	return cCode;
}/****** Wiegand37() *****/

/********************************************************************************
Funkcija:		Weigand35
Parameteri:     tbuf.....pointer na bufer u koji su upisani podaci
Return val:     kod procitane kartice
Desc:			Funkcija pretvara pristigli array u kod kartice. Ne provjerava
				cheksum
*********************************************************************************/
unsigned long Wiegand35(unsigned char *tbuf)
{
	unsigned char cnt;
	unsigned long cCode;
	cCode = 0;
	for (cnt = 14; cnt < 33; cnt++) //pretvaranje arraya iz LCDStringfera u sifru
	{
		if (*(tbuf + cnt) == 1)
			cCode++;
		cCode = cCode << 1;
	}
	if (*(tbuf + 33) == 1)
		cCode++;
	return cCode;
}/****** Wiegand35() *****/

void wiegand_setup() {
    //wiringPiSetup();  // inicijalizuj wiringPi

    pinMode(D0_PIN, INPUT);  // postavi D0 kao ulaz
    pinMode(D1_PIN, INPUT);  // postavi D1 kao ulaz

    // Postavi prekidače
    wiringPiISR(D0_PIN, INT_EDGE_FALLING, &d0Interrupt);
    wiringPiISR(D1_PIN, INT_EDGE_FALLING, &d1Interrupt);
}

void wiegand() {
    while (1) {
        if (timeout) {
            // Ako je timeout istekao, obradi primljene podatke
            if (bitCount > 0) {
	            if (bitCount == 34)
		            CardCode = Wiegand34(&rdrBuf[0]);
	            else if (bitCount == 32)
		            CardCode = Wiegand32(&rdrBuf[0]);
	            else if (bitCount == 26)
		            CardCode = Wiegand26(&rdrBuf[0]);
	            else if (bitCount == 37)
		            CardCode = Wiegand37(&rdrBuf[0]);
	            else if (bitCount == 35)
		            CardCode = Wiegand35(&rdrBuf[0]);
	            else
		            CardCode = 0;
                printf("Primljen Wiegand signal: %lu, broj bitova: %d\n", CardCode, bitCount);
                // Resetuj podatke
                data = 0;
                bitCount = 0;
            }
        }
        else {
            // Pokreni novi tajmer svaki put kada se detektuje impuls
            pthread_t timerThread;
            timeout = false; // Postavi timeout na false
            pthread_create(&timerThread, NULL, timeoutHandler, NULL);
            pthread_detach(timerThread); // Odvojimo thread
            usleep(1000); // Kratka pauza da ne saturiramo CPU
        }
    }
}