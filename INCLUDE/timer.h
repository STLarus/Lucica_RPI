#ifndef TIMEx_H
#define TIMER_H

#include <stdint.h>


struct timer { unsigned int start, interval; };
extern uint32_t Timex;
extern uint32_t LocalTime;
extern unsigned int timer_expired(struct timer* t);
extern void timer_set(struct timer* t, unsigned int usecs);

#endif
