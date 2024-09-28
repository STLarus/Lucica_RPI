#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>
#include <sys/time.h>
#define NUM_INPUTS 6


//int inputPins[NUM_INPUTS] = { 4, 17, 27, 22, 5, 26 }; // GPIO pinovi za ulaze
int pins[NUM_INPUTS] = { 7, 0, 2, 3, 21, 25 }; // wiringPI pinovi za ulaze
volatile int impulseCount[NUM_INPUTS] = { 0 }; // Brojači impulsa

volatile int pulseCount[NUM_INPUTS] = { 0 };      // Brojači impulsa za svaki pin
volatile long lastPulseTime[NUM_INPUTS] = { 0 };  // Vreme poslednjih impulsa

// Funkcija za dobijanje trenutnog vremena u mikrosekundama
long getCurrentTimeMicroseconds() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}


void pulseISR(int pinIndex) {
	pulseCount[pinIndex]++;  // Povećaj broj impulsa za taj pin

	long currentTime = getCurrentTimeMicroseconds();

	if (lastPulseTime[pinIndex] != 0) {
		long timeBetweenPulses = currentTime - lastPulseTime[pinIndex];  // Vreme između dva impulsa u mikrosekundama
		double timeBetweenPulsesSec = timeBetweenPulses / 1000000.0;  // Vreme u sekundama

		// Izračunaj trenutnu snagu (W), pretpostavljamo 1 impuls = 1 Wh
		double energyPerPulse = 1.0;  // Primer: energija po impulsu u Wh (promeni prema tvojim brojilima)
		double power = energyPerPulse / timeBetweenPulsesSec;  // Snaga u W

		printf("GPIO %d, Impuls %d, Vreme između impulsa: %.6f s, Trenutna snaga: %.2f W\n",
			pins[pinIndex], pulseCount[pinIndex], timeBetweenPulsesSec, power);
	}

	lastPulseTime[pinIndex] = currentTime;  // Ažuriraj vreme poslednjeg impulsa
}

// Wrapper funkcije za svaki GPIO pin
void pulseISR0() { pulseISR(0); }
void pulseISR1() { pulseISR(1); }
void pulseISR2() { pulseISR(2); }
void pulseISR3() { pulseISR(3); }
void pulseISR4() { pulseISR(4); }
void pulseISR5() { pulseISR(5); }




void input_setup() {
	// Postavi interrupt handler za svaki GPIO pin
	wiringPiISR(pins[0], INT_EDGE_RISING, &pulseISR0);
	wiringPiISR(pins[1], INT_EDGE_RISING, &pulseISR1);
	wiringPiISR(pins[2], INT_EDGE_RISING, &pulseISR2);
	wiringPiISR(pins[3], INT_EDGE_RISING, &pulseISR3);
	wiringPiISR(pins[4], INT_EDGE_RISING, &pulseISR4);
	wiringPiISR(pins[5], INT_EDGE_RISING, &pulseISR5);
}
