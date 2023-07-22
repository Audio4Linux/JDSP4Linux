#ifndef _HPACONF_H_
#define _HPACONF_H_

#define HPA_VERSION "1.7"
extern int XLITTLE_ENDIAN;
#define XULONG_BITSIZE (sizeof(unsigned long)*8)
#define XERR_DFL 1
// Here is the precision
// Available precision = 7, 11, 15, 19, 23, 27, 31
// 7 = 34 decimal places
// Algorithm become slower when XDIM > 19
// 31 = 150 decimal places
#define XDIM 15
#endif
