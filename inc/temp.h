
#ifndef TEMP_H_
#define TEMP_H_

void msecinit();





double msecs_relstartup();
double msecs();

double msellapse(void (*f)(void));
double msellapsecycle(int count, void (*f)(void));

#endif /* TEMP_H_ */
