#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

uint32_t Timex=0;
struct timer
{
	uint32_t start;
	uint32_t interval;
};


// Handler za tajmer interrupt
void timerHandler(int signum) {
	Timex=Timex+10;
	}


void init_timer(void)
{
	struct sigaction sa;
	struct itimerval timer;

	// Postavi signal handler
	sa.sa_handler = &timerHandler;
	sa.sa_flags = SA_RESTART; // Restartiraj sistemske pozive ako su prekinuti signalom
	sigaction(SIGALRM, &sa, NULL);

	// Postavi tajmer na 10 ms
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 10000; // Prvi interrupt nakon 10 ms
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 10000; // Sledeći interrupt svakih 10 ms

	// Postavi tajmer
	setitimer(ITIMER_REAL, &timer, NULL);
}





//------------------------------------------------------------------------
int timer_expired(struct timer* t)
{
	return (int)(Timex - t->start) >= (int)t->interval;
} /**** timer_expired() ****/



//------------------------------------------------------------------------
void timer_set(struct timer* t, unsigned int interval)
{
	t->interval = interval;
	t->start = Timex;
}/**** timer_set() ***/

void IncrementTimer(void)
{
	Timex++;
}

