#ifndef __AUTOCONVOLVER_H__
#define __AUTOCONVOLVER_H__
int ACFFTWThreadInit();
void ACFFTWClean(int threads);
double** PartitionHelperWisdomGetFromFile(const char *file, int *itemRet);
char* PartitionHelper(int s_max, int fs);
double** PartitionHelperDirect(int s_max, int fs);
#endif
