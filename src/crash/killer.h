#ifndef KILLER_H
#define KILLER_H
#include <signal.h>

int divide_by_zero()
{
  int a = 1;
  int b = 0;
  return a / b;
}

void cause_segfault()
{
  int * p = (int*)0x12345678;
  *p = 0;
}

void stack_overflow();
void stack_overflow()
{
  int foo[1000];
  (void)foo;
  stack_overflow();
}

/* break out with ctrl+c to test SIGINT handling */
void infinite_loop()
{
  while(1) {};
}

void illegal_instruction()
{
  /* I couldn't find an easy way to cause this one, so I'm cheating */
  raise(SIGILL);
}


#endif // KILLER_H
