#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "vdc.h"
double SOS_DF2Process(DirectForm2 *df2, double x1)
{
	double w1 = x1 - df2->a1*df2->v1L - df2->a2*df2->v2L;
	double y1 = df2->b0*w1 + df2->b1*df2->v1L + df2->b2*df2->v2L;
	df2->v2L = df2->v1L;
	df2->v1L = w1;
	return y1;
}
void SOS_DF2_StereoProcess(DirectForm2 *df2, double x1, double x2, double *Out_y1, double *Out_y2)
{
	double w1 = x1 - df2->a1*df2->v1L - df2->a2*df2->v2L;
	double y1 = df2->b0*w1 + df2->b1*df2->v1L + df2->b2*df2->v2L;
	df2->v2L = df2->v1L;
	df2->v1L = w1;
	*Out_y1 = y1;
	double w2 = x2 - df2->a1*df2->v1R - df2->a2*df2->v2R;
	double y2 = df2->b0*w2 + df2->b1*df2->v1R + df2->b2*df2->v2R;
	df2->v2R = df2->v1R;
	df2->v1R = w2;
	*Out_y2 = y2;
}
int countChars(char* s, char c)
{
	return *s == '\0' ? 0 : countChars(s + 1, c) + (*s == c);
}
int get_doubleVDC(char *val, double *F)
{
	char *eptr;
	errno = 0;
	double f = strtod(val, &eptr);
	if (eptr != val && errno != ERANGE)
	{
		*F = f;
		return 1;
	}
	return 0;
}
int DDCParser(char *DDCString, DirectForm2 ***ptrdf441, DirectForm2 ***ptrdf48)
{
	char *fs44_1 = strstr(DDCString, "SR_44100");
	char *fs48 = strstr(DDCString, "SR_48000");
	int numberCount = (countChars(fs48, ',') + 1);
	int sosCount = numberCount / 5;
	DirectForm2 **df441 = (DirectForm2**)malloc(sosCount * sizeof(DirectForm2*));
	DirectForm2 **df48 = (DirectForm2**)malloc(sosCount * sizeof(DirectForm2*));
	int i;
	for (i = 0; i < sosCount; i++)
	{
		df441[i] = (DirectForm2*)malloc(sizeof(DirectForm2));
		memset(df441[i], 0, sizeof(DirectForm2));
		df48[i] = (DirectForm2*)malloc(sizeof(DirectForm2));
		memset(df48[i], 0, sizeof(DirectForm2));
	}
	double number;
	i = 0;
	int counter = 0;
	int b0b1b2a1a2 = 0;
	char *startingPoint = fs44_1 + 9;
	while (counter < numberCount)
	{
		if (get_doubleVDC(startingPoint, &number))
		{
			double val = strtod(startingPoint, &startingPoint);
			counter++;
			if (!b0b1b2a1a2)
				df441[i]->b0 = val;
			else if (b0b1b2a1a2 == 1)
				df441[i]->b1 = val;
			else if (b0b1b2a1a2 == 2)
				df441[i]->b2 = val;
			else if (b0b1b2a1a2 == 3)
				df441[i]->a1 = -val;
			else if (b0b1b2a1a2 == 4)
			{
				df441[i]->a2 = -val;
				i++;
			}
			b0b1b2a1a2++;
			if (b0b1b2a1a2 == 5)
				b0b1b2a1a2 = 0;
		}
		else
			startingPoint++;
	}
	i = 0;
	counter = 0;
	b0b1b2a1a2 = 0;
	startingPoint = fs48 + 9;
	while (counter < numberCount)
	{
		if (get_doubleVDC(startingPoint, &number))
		{
			double val = strtod(startingPoint, &startingPoint);
			counter++;
			if (!b0b1b2a1a2)
				df48[i]->b0 = val;
			else if (b0b1b2a1a2 == 1)
				df48[i]->b1 = val;
			else if (b0b1b2a1a2 == 2)
				df48[i]->b2 = val;
			else if (b0b1b2a1a2 == 3)
				df48[i]->a1 = -val;
			else if (b0b1b2a1a2 == 4)
			{
				df48[i]->a2 = -val;
				i++;
			}
			b0b1b2a1a2++;
			if (b0b1b2a1a2 == 5)
				b0b1b2a1a2 = 0;
		}
		else
			startingPoint++;
	}
	*ptrdf441 = df441;
	*ptrdf48 = df48;
	return sosCount;
}