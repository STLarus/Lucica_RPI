#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>

#define NUM_INPUTS 6
#define POLL_INTERVAL_MS 20

int inputPins[NUM_INPUTS] = { 4, 17, 27, 22, 5, 26 }; // GPIO pinovi za ulaze
volatile int impulseCount[NUM_INPUTS] = { 0 }; // Brojači impulsa

void input_setup() {
    wiringPiSetup();  // Inicijalizuj WiringPi

    // Postavi pinove kao ulaze
    for (int i = 0; i < NUM_INPUTS; i++) {
        pinMode(inputPins[i], INPUT);
    }
}

void input_loop() {
    int previousState[NUM_INPUTS] = { 0 };

    while (1) {
        for (int i = 0; i < NUM_INPUTS; i++) {
            int currentState = digitalRead(inputPins[i]);

            // Proveri da li je došlo do promene (npr. opadajući signal)
            if (currentState == LOW && previousState[i] == HIGH) {
                impulseCount[i]++;
            }

            previousState[i] = currentState; // Ažuriraj prethodno stanje
        }

        // Ispiši trenutne brojače
        printf("Brojači impulsa:\n");
        for (int i = 0; i < NUM_INPUTS; i++) {
            printf("Pin %d: %d\n", inputPins[i], impulseCount[i]);
        }
        printf("\n");

        usleep(POLL_INTERVAL_MS * 1000); // Pauza od 20 ms
    }
}