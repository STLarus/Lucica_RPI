#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

// Handler za tajmer interrupt
void timerHandler(int signum) {
	static int count = 0;
	printf("Tajmer interrupt broj: %d\n", ++count);
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
	


unsigned int Timex;
uint32_t LocalTime,sysTyck;
struct timer
	{
	unsigned int start;
	unsigned int interval;
	};

//------------------------------------------------------------------------
int timer_expired(struct timer *t)
{
return (int)(Timex - t->start) >= (int)t->interval;
} /**** timer_expired() ****/



//------------------------------------------------------------------------
void timer_set(struct timer *t, unsigned int interval)
{
t->interval = interval;
t->start = Timex;
}/**** timer_set() ***/

void IncrementTimer(void)
{
Timex++;
LocalTime=LocalTime+10;
sysTyck++;
}

uint32_t sys_now(void)
{
 return sysTyck;
}

