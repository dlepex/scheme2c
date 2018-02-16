

#include <sys/timeb.h>
#include "temp.h"
#include "base.h"

static double Startup_time = 0;

void msecinit()
{
	global_once_preamble();
	Startup_time = msecs();

}


inline double msecs() {
	struct timeb tb;
	ftime(&tb);
	return (double) (tb.time) * 1.0e3 + tb.millitm;
}


double msecs_relstartup() {
	return msecs() - Startup_time;
}

double msellapse(void (*f)(void)) {
	double t1 = msecs();
	f();
	double t2 = msecs();
	return t2 - t1;
}


double msellapsecycle(int count, void (*f)(void)) {
	double t1 = msecs();
	for(; count; count--)
		f();
	double t2 = msecs();
	return t2 - t1;
}
