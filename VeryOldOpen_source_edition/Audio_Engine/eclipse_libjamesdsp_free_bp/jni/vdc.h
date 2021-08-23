typedef struct
{
	double b0, b1, b2, a1, a2;
	double v1L, v2L, v1R, v2R; // State
} DirectForm2;
double SOS_DF2Process(DirectForm2 *df2, double x);
void SOS_DF2_StereoProcess(DirectForm2 *df2, double x1, double x2, double *Out_y1, double *Out_y2);
int DDCParser(char *DDCString, DirectForm2 ***ptrdf441, DirectForm2 ***ptrdf48);
int PeakingFilterResampler(DirectForm2 **inputIIR, double inFs, DirectForm2 ***resampledIIR, double outFs, int sosCount);
