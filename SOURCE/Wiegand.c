#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

#define D0_PIN 13  // GPIO pin za D0
#define D1_PIN 6  // GPIO pin za D1
#define TIMEOUT_MS 50 // Timeout u milisekundama

volatile int bitCount = 0;
volatile unsigned long data = 0;
volatile bool timeout = false;

// Funkcija za upravljanje timeout-om
void* timeoutHandler(void* arg) {
    usleep(TIMEOUT_MS * 1000); // Pretvori u mikrosekunde
    timeout = true;
    return NULL;
}

void d0Interrupt(void) {
    data = (data << 1);  // pomeri levo i dodaj 0
    bitCount++;
    timeout = false;  // Resetuj timeout
}

void d1Interrupt(void) {
    data = (data << 1) | 1;  // pomeri levo i dodaj 1
    bitCount++;
    timeout = false;  // Resetuj timeout
}

void wiegand_setup() {
    wiringPiSetup();  // inicijalizuj wiringPi

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
                printf("Primljen Wiegand signal: %lu, broj bitova: %d\n", data, bitCount);
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