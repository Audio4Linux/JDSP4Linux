/*
  Expression Evaluator Library (NS-EEL) v2
  Copyright (C) 2004-2013 Cockos Incorporated
  Copyright (C) 1999-2003 Nullsoft, Inc.
  nseel-compiler.c
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
	 claim that you wrote the original software. If you use this software
	 in a product, an acknowledgment in the product documentation would be
	 appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <float.h>
#include "eelCommon.h"
#include "glue_port.h"
#include <assert.h>
#include "eel_matrix.h"
#include "numericSys/FFTConvolver.h"
#include "numericSys/HPFloat/xpre.h"
static void lstrcpyn_safe(char *o, const char *in, int32_t count)
{
	if (count > 0)
	{
		while (--count > 0 && *in) *o++ = *in++;
		*o = 0;
	}
}
static void lstrcatn(char *o, const char *in, int32_t count)
{
	if (count > 0)
	{
		while (*o) { if (--count < 1) return; o++; }
		while (--count > 0 && *in) *o++ = *in++;
		*o = 0;
	}
}
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
static void snprintf_append(char *o, int32_t count, const char *format, ...)
{
	if (count > 0)
	{
		va_list va;
		while (*o) { if (--count < 1) return; o++; }
		va_start(va, format);
		stbsp_vsnprintf(o, count, format, va);
		va_end(va);
	}
}
#define NSEEL_VARS_MALLOC_CHUNKSIZE 8
#define RET_MINUS1_FAIL(x) return -1;
#define MIN_COMPUTABLE_SIZE 32 // always use at least this big of a temp storage table (and reset the temp ptr when it goes past this boundary)
#define COMPUTABLE_EXTRA_SPACE 16 // safety buffer, if EEL_VALIDATE_WORKTABLE_USE set, used for magic-value-checking
/*
  P1 is rightmost parameter
  P2 is second rightmost, if any
  P3 is third rightmost, if any
  registers on x86 are  (RAX etc on x86-64)
	P1(ret) EAX
	P2 EDI
	P3 ECX
	WTP RSI
	x86_64: r12 is a pointer to ram_state
	x86_64: r13 is a pointer to closenessfactor
  registers on PPC are:
	P1(ret) r3
	P2 r14
	P3 r15
	WTP r16 (r17 has the original value)
	r13 is a pointer to ram_state
	ppc uses f31 and f30 and others for certain constants
  */
  // used by //#eel-no-optimize:xxx, in 0
#define OPTFLAG_NO_FPSTACK 2
#define OPTFLAG_NO_INLINEFUNC 4
#define MAX_SUB_NAMESPACES 32
typedef struct
{
	const char *namespacePathToThis;
	const char *subParmInfo[MAX_SUB_NAMESPACES];
} namespaceInformation;
static int32_t nseel_evallib_stats[5]; // source bytes, static code bytes, call code bytes, data bytes, segments
int32_t *NSEEL_getstats()
{
	return nseel_evallib_stats;
}
static int32_t findLineNumber(const char *exp, int32_t byteoffs)
{
	int32_t lc = 0;
	while (byteoffs-- > 0 && *exp) if (*exp++ == '\n') lc++;
	return lc;
}
static void *__newBlock(llBlock **start, int32_t size);
#define OPCODE_IS_TRIVIAL(x) ((x)->opcodeType <= OPCODETYPE_VARPTRPTR)
enum {
	OPCODETYPE_DIRECTVALUE = 0,
	OPCODETYPE_DIRECTVALUE_TEMPSTRING, // like directvalue, but will generate a new tempstring value on generate
	OPCODETYPE_VALUE_FROM_NAMESPACENAME, // this.* or namespace.* are encoded this way
	OPCODETYPE_VARPTR,
	OPCODETYPE_VARPTRPTR,
	OPCODETYPE_FUNC1,
	OPCODETYPE_FUNC2,
	OPCODETYPE_FUNC3,
	OPCODETYPE_FUNCX,
	OPCODETYPE_MOREPARAMS,
	OPCODETYPE_INVALID,
};
struct opcodeRec
{
	int32_t opcodeType;
	int32_t fntype;
	void *fn;
	union {
		struct opcodeRec *parms[3];
		struct {
			float directValue;
			float *valuePtr; // if direct value, valuePtr can be cached
		} dv;
	} parms;
	int32_t namespaceidx;
	// OPCODETYPE_VALUE_FROM_NAMESPACENAME (relname is either empty or blah)
	// OPCODETYPE_VARPTR if it represents a global variable, will be nonempty
	// OPCODETYPE_FUNC* with fntype=FUNCTYPE_EELFUNC
	const char *relname;
};
float *dataSectionToRamDisk(void *opaque, size_t len)
{
	compileContext *c = (compileContext*)opaque;
	return c->ram_state + (NSEEL_RAM_ITEMSPERBLOCK - len);
}
static void *newTmpBlock(compileContext *ctx, int32_t size)
{
	const int32_t align = 8;
	const int32_t a1 = align - 1;
	char *p = (char*)__newBlock(&ctx->tmpblocks_head, size + a1);
	return p + ((align - (((INT_PTR)p)&a1))&a1);
}
static void *__newBlock_align(compileContext *ctx, int32_t size, int32_t align, int32_t isForCode)
{
	const int32_t a1 = align - 1;
	char *p = (char*)__newBlock(
		(
			isForCode < 0 ? (isForCode == -2 ? &ctx->pblocks : &ctx->tmpblocks_head) :
			isForCode > 0 ? &ctx->blocks_head :
			&ctx->blocks_head_data), size + a1);
	return p + ((align - (((INT_PTR)p)&a1))&a1);
}
static opcodeRec *newOpCode(compileContext *ctx, const char *str, int32_t opType)
{
	const size_t strszfull = str ? strlen(str) : 0;
	const size_t str_sz = min(NSEEL_MAX_VARIABLE_NAMELEN, strszfull);
	opcodeRec *rec = (opcodeRec*)__newBlock_align(ctx,
		(int32_t)(sizeof(opcodeRec) + (str_sz > 0 ? str_sz + 1 : 0)),
		8, ctx->isSharedFunctions ? 0 : -1);
	if (rec)
	{
		memset(rec, 0, sizeof(*rec));
		rec->opcodeType = opType;
		if (str_sz > 0)
		{
			char *p = (char *)(rec + 1);
			memcpy(p, str, str_sz);
			p[str_sz] = 0;
			rec->relname = p;
		}
		else
		{
			rec->relname = "";
		}
	}
	return rec;
}
#define newCodeBlock(x,a) __newBlock_align(ctx,x,a,1)
#define newDataBlock(x,a) __newBlock_align(ctx,x,a,0)
#define newCtxDataBlock(x,a) __newBlock_align(ctx,x,a,-2)
static void freeBlocks(llBlock **start);
#ifndef DECL_ASMFUNC
#define DECL_ASMFUNC(x)         \
  void nseel_asm_##x(void);        \
  void nseel_asm_##x##_end(void);
void _asm_megabuf(void);
void _asm_megabuf_end(void);
#endif
#define FUNCTIONTYPE_PARAMETERCOUNTMASK 0xff
#define BIF_NPARAMS_MASK       0x7ffff00
#define BIF_RETURNSONSTACK     0x0000100
#define BIF_LASTPARMONSTACK    0x0000200
#define BIF_RETURNSBOOL        0x0000400
#define BIF_LASTPARM_ASBOOL    0x0000800
//                             0x00?0000 -- taken by FP stack flags
#define BIF_TAKES_VARPARM      0x0400000
#define BIF_TAKES_VARPARM_EX   0x0C00000 // this is like varparm but check count exactly
#define BIF_WONTMAKEDENORMAL   0x0100000
#define BIF_CLEARDENORMAL      0x0200000
#if defined(GLUE_HAS_FXCH) && GLUE_MAX_FPSTACK_SIZE > 0
#define BIF_SECONDLASTPARMST 0x0001000 // use with BIF_LASTPARMONSTACK only (last two parameters get passed on fp stack)
#define BIF_LAZYPARMORDERING 0x0002000 // allow optimizer to avoid fxch when using BIF_TWOPARMSONFPSTACK_LAZY etc
#define BIF_REVERSEFPORDER   0x0004000 // force a fxch (reverse order of last two parameters on fp stack, used by comparison functions)
#ifndef BIF_FPSTACKUSE
#define BIF_FPSTACKUSE(x) (((x)>=0&&(x)<8) ? ((7-(x))<<16):0)
#endif
#ifndef BIF_GETFPSTACKUSE
#define BIF_GETFPSTACKUSE(x) (7 - (((x)>>16)&7))
#endif
#else
  // do not support fp stack use unless GLUE_HAS_FXCH and GLUE_MAX_FPSTACK_SIZE>0
#define BIF_SECONDLASTPARMST 0
#define BIF_LAZYPARMORDERING 0
#define BIF_REVERSEFPORDER   0
#define BIF_FPSTACKUSE(x) 0
#define BIF_GETFPSTACKUSE(x) 0
#endif
#define BIF_TWOPARMSONFPSTACK (BIF_SECONDLASTPARMST|BIF_LASTPARMONSTACK)
#define BIF_TWOPARMSONFPSTACK_LAZY (BIF_LAZYPARMORDERING|BIF_SECONDLASTPARMST|BIF_LASTPARMONSTACK)
float NSEEL_CGEN_CALL nseel_int_rand(float f);
#define FNPTR_HAS_CONDITIONAL_EXEC(op)  \
  (op->fntype == FN_LOGICAL_AND || \
   op->fntype == FN_LOGICAL_OR ||  \
   op->fntype == FN_IF_ELSE || \
   op->fntype == FN_WHILE || \
   op->fntype == FN_LOOP)
// Add custom functions
static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
static unsigned char* base64_encode(const unsigned char *src, size_t len, size_t *out_len)
{
	unsigned char *out, *pos;
	const unsigned char *end, *in;
	size_t olen;
	olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
	if (olen < len)
		return NULL; /* integer overflow */
	out = (unsigned char*)malloc(olen);
	if (out == NULL)
		return NULL;
	end = src + len;
	in = src;
	pos = out;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		}
		else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) |
				(in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}
	*pos = '\0';
	if (out_len)
		*out_len = pos - out;
	return out;
}
/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
static unsigned char* base64_decode(const unsigned char *src, size_t len, size_t *out_len)
{
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t i, count, olen;
	int32_t pad = 0;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char)i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#endif
static float NSEEL_CGEN_CALL _eel_sleep(float amt)
{
	if (amt >= 0.0f)
	{
#ifdef _WIN32
		if (amt > 30000000.0) Sleep(30000000);
		else Sleep((DWORD)(amt + 0.5f));
#else
		if (amt > 30000000.0) usleep(((useconds_t)30000000) * 1000);
		else usleep((useconds_t)(amt * 1000.0 + 0.5f));
#endif
	}
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_time(void *opaque)
{
	return (float)time(NULL);
}
static float NSEEL_CGEN_CALL _eel_time_precise(void *opaque)
{
#ifdef _WIN32
	LARGE_INTEGER freq, now;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&now);
	return (float)now.QuadPart / (float)freq.QuadPart;
#else
	struct timeval tm = { 0, };
	gettimeofday(&tm, NULL);
	return (float)tm.tv_sec + (float)tm.tv_usec * 0.000001;
#endif
}
void discreteHartleyTransform(double *A, const int32_t nPoints, const double *sinTab)
{
	int32_t i, j, n, n2, theta_inc, nptDiv2;
	double alpha, beta;
	for (i = 0; i < nPoints; i += 4)
	{
		const double	x0 = A[i];
		const double	x1 = A[i + 1];
		const double	x2 = A[i + 2];
		const double	x3 = A[i + 3];
		const double	y0 = x0 + x1;
		const double	y1 = x0 - x1;
		const double	y2 = x2 + x3;
		const double	y3 = x2 - x3;
		A[i] = y0 + y2;
		A[i + 2] = y0 - y2;
		A[i + 1] = y1 + y3;
		A[i + 3] = y1 - y3;
	}
	for (i = 0; i < nPoints; i += 8)
	{
		alpha = A[i];
		beta = A[i + 4];
		A[i] = alpha + beta;
		A[i + 4] = alpha - beta;
		alpha = A[i + 2];
		beta = A[i + 6];
		A[i + 2] = alpha + beta;
		A[i + 6] = alpha - beta;
		alpha = A[i + 1];
		const double beta1 = 0.70710678118654752440084436210485*(A[i + 5] + A[i + 7]);
		const double beta2 = 0.70710678118654752440084436210485*(A[i + 5] - A[i + 7]);
		A[i + 1] = alpha + beta1;
		A[i + 5] = alpha - beta1;
		alpha = A[i + 3];
		A[i + 3] = alpha + beta2;
		A[i + 7] = alpha - beta2;
	}
	n = 16;
	n2 = 8;
	theta_inc = nPoints >> 4;
	nptDiv2 = nPoints >> 2;
	while (n <= nPoints)
	{
		for (i = 0; i < nPoints; i += n)
		{
			int32_t theta = theta_inc;
			const int32_t n4 = n2 >> 1;
			alpha = A[i];
			beta = A[i + n2];
			A[i] = alpha + beta;
			A[i + n2] = alpha - beta;
			alpha = A[i + n4];
			beta = A[i + n2 + n4];
			A[i + n4] = alpha + beta;
			A[i + n2 + n4] = alpha - beta;
			for (j = 1; j < n4; j++)
			{
				double sinval = sinTab[theta];
				double cosval = sinTab[theta + nptDiv2];
				double alpha1 = A[i + j];
				double alpha2 = A[i - j + n2];
				double beta1 = A[i + j + n2] * cosval + A[i - j + n] * sinval;
				double beta2 = A[i + j + n2] * sinval - A[i - j + n] * cosval;
				theta += theta_inc;
				A[i + j] = alpha1 + beta1;
				A[i + j + n2] = alpha1 - beta1;
				A[i - j + n2] = alpha2 + beta2;
				A[i - j + n] = alpha2 - beta2;
			}
		}
		n <<= 1;
		n2 <<= 1;
		theta_inc >>= 1;
	}
}
void discreteHartleyTransformFloat(float *A, const int32_t nPoints, const float *sinTab)
{
	int32_t i, j, n, n2, theta_inc, nptDiv2;
	float alpha, beta;
	for (i = 0; i < nPoints; i += 4)
	{
		const float	x0 = A[i];
		const float	x1 = A[i + 1];
		const float	x2 = A[i + 2];
		const float	x3 = A[i + 3];
		const float	y0 = x0 + x1;
		const float	y1 = x0 - x1;
		const float	y2 = x2 + x3;
		const float	y3 = x2 - x3;
		A[i] = y0 + y2;
		A[i + 2] = y0 - y2;
		A[i + 1] = y1 + y3;
		A[i + 3] = y1 - y3;
	}
	for (i = 0; i < nPoints; i += 8)
	{
		alpha = A[i];
		beta = A[i + 4];
		A[i] = alpha + beta;
		A[i + 4] = alpha - beta;
		alpha = A[i + 2];
		beta = A[i + 6];
		A[i + 2] = alpha + beta;
		A[i + 6] = alpha - beta;
		alpha = A[i + 1];
		const float beta1 = 0.70710678118654752440084436210485f*(A[i + 5] + A[i + 7]);
		const float beta2 = 0.70710678118654752440084436210485f*(A[i + 5] - A[i + 7]);
		A[i + 1] = alpha + beta1;
		A[i + 5] = alpha - beta1;
		alpha = A[i + 3];
		A[i + 3] = alpha + beta2;
		A[i + 7] = alpha - beta2;
	}
	n = 16;
	n2 = 8;
	theta_inc = nPoints >> 4;
	nptDiv2 = nPoints >> 2;
	while (n <= nPoints)
	{
		for (i = 0; i < nPoints; i += n)
		{
			int32_t theta = theta_inc;
			const int32_t n4 = n2 >> 1;
			alpha = A[i];
			beta = A[i + n2];
			A[i] = alpha + beta;
			A[i + n2] = alpha - beta;
			alpha = A[i + n4];
			beta = A[i + n2 + n4];
			A[i + n4] = alpha + beta;
			A[i + n2 + n4] = alpha - beta;
			for (j = 1; j < n4; j++)
			{
				float sinval = sinTab[theta];
				float cosval = sinTab[theta + nptDiv2];
				float alpha1 = A[i + j];
				float alpha2 = A[i - j + n2];
				float beta1 = A[i + j + n2] * cosval + A[i - j + n] * sinval;
				float beta2 = A[i + j + n2] * sinval - A[i - j + n] * cosval;
				theta += theta_inc;
				A[i + j] = alpha1 + beta1;
				A[i + j + n2] = alpha1 - beta1;
				A[i - j + n2] = alpha2 + beta2;
				A[i - j + n] = alpha2 - beta2;
			}
		}
		n <<= 1;
		n2 <<= 1;
		theta_inc >>= 1;
	}
}
#define M_PIDouble 3.1415926535897932384626433832795
void getAsymmetricWindow(float *analysisWnd, float *synthesisWnd, int32_t k, int32_t m, float freq_temporal)
{
	int32_t i;
	memset(synthesisWnd, 0, k * sizeof(float));
	if (freq_temporal < 0.4f)
		freq_temporal = 0.4f;
	if (freq_temporal > 1.8f)
		freq_temporal = 1.8f;
	int32_t n = ((k - m) << 1) + 2;
	for (i = 0; i < k - m; ++i)
		analysisWnd[i] = (float)pow(sqrt(0.5 * (1.0 - cos(2.0 * M_PIDouble * (i + 1.0) / (double)n))), freq_temporal);
	n = (m << 1) + 2;
	if (freq_temporal > 1.5f)
		freq_temporal = 1.5f;
	for (i = k - m; i < k; ++i)
		analysisWnd[i] = (float)pow(sqrt(0.5 * (1.0 - cos(2.0 * M_PIDouble * ((m + i - (k - m)) + 1.0) / (double)n))), freq_temporal);
	n = m << 1;
	for (i = k - (m << 1); i < k; ++i)
		synthesisWnd[i - (k - (m << 1))] = (float)(0.5 * (1.0 - cos(2.0 * M_PIDouble * (double)(i - (k - (m << 1))) / (double)n))) / analysisWnd[i];
}
void getwnd(float *wnd, unsigned int m, unsigned int n, char *mode)
{
	unsigned int i;
	double x;
	if (!strcmp(mode, "hann"))
	{
		for (i = 0; i < m; i++)
		{
			x = i / (double)(n - 1);
			wnd[i] = (float)(0.5 - 0.5 * cos(2 * M_PIDouble * x));
		}
	}
	else if (!strcmp(mode, "hamming"))
	{
		for (i = 0; i < m; i++)
		{
			x = i / (double)(n - 1);
			wnd[i] = (float)(0.54 - 0.46 * cos(2 * M_PIDouble * x));
		}
	}
	else if (!strcmp(mode, "blackman"))
	{
		for (i = 0; i < m; i++)
		{
			x = i / (double)(n - 1);
			wnd[i] = (float)(0.42 - 0.5 * cos(2 * M_PIDouble * x) + 0.08 * cos(4 * M_PIDouble * x));
		}
	}
	else if (!strcmp(mode, "flattop"))
	{
		double a0 = 0.21557895;
		double a1 = 0.41663158;
		double a2 = 0.277263158;
		double a3 = 0.083578947;
		double a4 = 0.006947368;
		for (i = 0; i < m; i++)
		{
			x = i / (double)(n - 1);
			wnd[i] = (float)(a0 - a1 * cos(2 * M_PIDouble * x) + a2 * cos(4 * M_PIDouble * x) - a3 * cos(6 * M_PIDouble * x) + a4 * cos(8 * M_PIDouble * x));
		}
	}
}
void genWnd(float *wnd, unsigned int N, char *type)
{
	unsigned int plus1 = N + 1;
	unsigned int half;
	unsigned int i;
	if (plus1 % 2 == 0)
	{
		half = plus1 / 2;
		getwnd(wnd, half, plus1, type);
		for (i = 0; i < half - 1; i++)
			wnd[i + half] = wnd[half - i - 1];
	}
	else
	{
		half = (plus1 + 1) / 2;
		getwnd(wnd, half, plus1, type);
		for (i = 0; i < half - 2; i++)
			wnd[i + half] = wnd[half - i - 2];
	}
}
void STFT_DynInit(int32_t *indexFw, float *analysisWnd)
{
	int32_t i;
	int32_t ovpSmps = indexFw[0] / indexFw[1];
	int32_t bufferSize = (indexFw[0] * 6) + indexFw[3] + indexFw[3] + (int32_t)((float)(indexFw[0] * sizeof(uint32_t)) / (float)(sizeof(float) / sizeof(uint32_t)));
	memset(analysisWnd, 0, bufferSize * sizeof(float));
	float *synthesisWnd = analysisWnd + indexFw[0];
	uint32_t *bitRevTbl = (uint32_t*)(analysisWnd + (indexFw[0] * 6) + indexFw[3] + indexFw[3]);
	uint32_t bitsConst = 0;
	uint32_t v = indexFw[0];
	while (v > 1)
	{
		++bitsConst;
		v >>= 1;
	}
	for (i = 0; i < indexFw[0]; ++i)
	{
		uint32_t bits = bitsConst;
		uint32_t x = i;
		bitRevTbl[i] = 0;
		while (bits--)
		{
			bitRevTbl[i] = (bitRevTbl[i] + bitRevTbl[i]) + (x & 1);
			x >>= 1;
		}
	}
	float *sineTbl = synthesisWnd + indexFw[0];
	float pi2dN = (M_PIDouble * 2.0f) / indexFw[0];
	for (i = 0; i < indexFw[0]; ++i)
		sineTbl[i] = (float)sin(pi2dN * i);
	getAsymmetricWindow(analysisWnd, synthesisWnd, indexFw[0], ovpSmps, indexFw[5] / (float)32767);
	// Pre-shift window function
	for (i = 0; i < indexFw[0] - indexFw[2]; i++)
		synthesisWnd[i] = synthesisWnd[i] * (1.0f / indexFw[0]) * 0.5f;
}
int32_t STFTCartesian(float *indexer, float *analysisWnd, float *ptr)
{
	int32_t *indexFw = (int32_t*)indexer;
	float *mSineTab = analysisWnd + indexFw[0] * 2;
	float *mInput = mSineTab + indexFw[0];
	float *mTempBuffer = mInput + indexFw[0];
	uint32_t *bitRevTbl = (uint32_t*)(analysisWnd + (indexFw[0] * 6) + indexFw[3] + indexFw[3]);
	int32_t i, symIdx;
	for (i = 0; i < indexFw[0]; ++i)
		mTempBuffer[bitRevTbl[i]] = mInput[(i + indexFw[4]) & (indexFw[0] - 1)] * analysisWnd[i];
	discreteHartleyTransformFloat(mTempBuffer, indexFw[0], mSineTab);
	ptr[0] = mTempBuffer[0] * 2.0f;
	ptr[1] = 0.0f;
	float lR, lI;
	for (i = 1; i < ((indexFw[0] >> 1) + 1); i++)
	{
		symIdx = indexFw[0] - i;
		lR = mTempBuffer[i] + mTempBuffer[symIdx];
		lI = mTempBuffer[i] - mTempBuffer[symIdx];
		ptr[i << 1] = lR;
		ptr[(i << 1) + 1] = -lI;
	}
	return indexFw[0] + 2;
}
int32_t STFTCartesianInverse(float *indexer, float *analysisWnd, float *ptr)
{
	int32_t *indexFw = (int32_t*)indexer;
	float *synthesisWnd = analysisWnd + indexFw[0];
	float *mSineTab = synthesisWnd + indexFw[0];
	float *timeDomainOut = mSineTab + indexFw[0] * 3;
	float *mOutputBuffer = timeDomainOut + indexFw[0];
	float *mOverlapStage2Ldash = mOutputBuffer + indexFw[3];
	uint32_t *bitRevTbl = (uint32_t*)(analysisWnd + (indexFw[0] * 6) + indexFw[3] + indexFw[3]);
	int32_t i;
	timeDomainOut[0] = ptr[0];
	float lR, lI;
	for (i = 1; i < ((indexFw[0] >> 1) + 1); i++)
	{
		lR = ptr[i << 1];
		lI = -ptr[(i << 1) + 1];
		timeDomainOut[bitRevTbl[i]] = (lR + lI);
		timeDomainOut[bitRevTbl[indexFw[0] - i]] = (lR - lI);
	}
	discreteHartleyTransformFloat(timeDomainOut, indexFw[0], mSineTab);
	for (i = 0; i < indexFw[0] - indexFw[2]; i++)
		timeDomainOut[i] = timeDomainOut[i + indexFw[2]] * synthesisWnd[i];
	for (i = 0; i < indexFw[3]; ++i)
	{
		mOutputBuffer[i] = mOverlapStage2Ldash[i] + timeDomainOut[i];
		mOverlapStage2Ldash[i] = timeDomainOut[indexFw[3] + i];
	}
	return indexFw[3];
}
static float NSEEL_CGEN_CALL stftInit(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	float *start1 = parms[0];
	int32_t offs1 = (int32_t)(*start1 + NSEEL_CLOSEFACTOR);
	float *start2 = parms[1];
	int32_t offs2 = (int32_t)(*start2 + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	float *stftFloatStruct = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	int32_t *indexFw = (int32_t*)indexer;
	STFT_DynInit(indexFw, stftFloatStruct);
	return (float)indexFw[3];
}
void STFT_SetWnd(int32_t *indexFw, float *stftFloatStruct, float *desired_analysisWnd, float *desired_synthesisWnd)
{
	int32_t i;
	float *synthesisWnd = stftFloatStruct + indexFw[0];
	for (i = 0; i < indexFw[0]; i++)
		stftFloatStruct[i] = desired_analysisWnd[i];
	for (i = 0; i < indexFw[0] - indexFw[2]; i++)
		synthesisWnd[i] = desired_synthesisWnd[i] * (1.0f / indexFw[0]) * 0.5f;
}
static float NSEEL_CGEN_CALL stftSetAsymWnd(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext *)opaque;
	float *blocks = c->ram_state;
	float *start1 = parms[0];
	int32_t offs1 = (int32_t)(*start1 + NSEEL_CLOSEFACTOR);
	float *start2 = parms[1];
	int32_t offs2 = (int32_t)(*start2 + NSEEL_CLOSEFACTOR);
	float *start3 = parms[2];
	int32_t offs3 = (int32_t)(*start3 + NSEEL_CLOSEFACTOR);
	float *start4 = parms[3];
	int32_t offs4 = (int32_t)(*start4 + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	float *stftFloatStruct = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	float *desired_analysisWnd = __NSEEL_RAMAlloc(blocks, (uint64_t)offs3);
	float *desired_synthesisWnd = __NSEEL_RAMAlloc(blocks, (uint64_t)offs4);
	int32_t *indexFw = (int32_t *)indexer;
	STFT_SetWnd(indexFw, stftFloatStruct, desired_analysisWnd, desired_synthesisWnd);
	return 0;
}
static float NSEEL_CGEN_CALL stftGetWindowPower(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	float *start1 = parms[0];
	int32_t offs1 = (int32_t)(*start1 + NSEEL_CLOSEFACTOR);
	float *start2 = parms[1];
	int32_t offs2 = (int32_t)(*start2 + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	float *stftFloatStruct = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	int32_t *indexFw = (int32_t*)indexer;
	float sumOfPreWindow = 0.0f;
	for (int i = 0; i < indexFw[0]; i++)
		sumOfPreWindow += stftFloatStruct[i];
	return 1.0f / sumOfPreWindow;
}
static float NSEEL_CGEN_CALL stftForward(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *ptr = __NSEEL_RAMAlloc(blocks, (uint64_t)offs);
	int32_t offs1 = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	int32_t offs2 = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	float *stftFloatStruct = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	int32_t *indexFw = (int32_t*)indexer;
	float *mInput = stftFloatStruct + indexFw[0] * 3;
	memcpy(&mInput[indexFw[4]], ptr, indexFw[3] * sizeof(float));
	indexFw[4] = (indexFw[4] + indexFw[3]) & (indexFw[0] - 1);
	return (float)STFTCartesian(indexer, stftFloatStruct, ptr);
}
static float NSEEL_CGEN_CALL stftBackward(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *ptr = __NSEEL_RAMAlloc(blocks, (uint64_t)offs);
	int32_t offs1 = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	int32_t offs2 = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	float *stftFloatStruct = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	int32_t *indexFw = (int32_t*)indexer;
	int32_t ret;
	ret = STFTCartesianInverse(indexer, stftFloatStruct, ptr);
	memcpy(ptr, stftFloatStruct + indexFw[0] * 6, indexFw[3] * sizeof(float));
	return (float)ret;
}
int32_t STFT_DynConstructor(float *indexer, int32_t fftLen, int32_t analysisOvp, float tf_res)
{
	int32_t ovpSmps = fftLen / analysisOvp;
	int32_t sampleShift = (fftLen - (ovpSmps << 1));
	int32_t *indexFw = (int32_t*)indexer;
	indexFw[0] = fftLen;
	indexFw[1] = analysisOvp;
	indexFw[2] = sampleShift;
	indexFw[3] = ovpSmps;
	indexFw[4] = 0;
	indexFw[5] = (int32_t)(tf_res * (float)32767);
	return ((fftLen * 6) + ovpSmps + ovpSmps + (int32_t)((float)(fftLen * sizeof(uint32_t)) / (float)(sizeof(float) / sizeof(uint32_t))));
}
static float NSEEL_CGEN_CALL stftCheckMemoryRequirement(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	uint32_t offs1 = (uint32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int32_t fftlen = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	int32_t analyOv = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	return (float)STFT_DynConstructor(indexer, fftlen, analyOv, *parms[3]);
}
static inline double ipowp(double x, long n)
{
	assert(n >= 0);
	double z = 1.0;
	while (n != 0)
	{
		if ((n & 1) != 0)
			z *= x;
		n >>= 1;
		x *= x;
	}
	return z;
}
static inline void compute_transition_param(double *k, double *q, double transition)
{
	assert(transition > 0);
	assert(transition < 0.5);
	*k = tan((1 - transition * 2) * M_PIDouble / 4);
	*k *= *k;
	assert(*k < 1);
	assert(*k > 0);
	double         kksqrt = pow(1.0 - *k * *k, 0.25);
	const double   e = 0.5 * (1 - kksqrt) / (1 + kksqrt);
	const double   e2 = e * e;
	const double   e4 = e2 * e2;
	*q = e * (1 + e4 * (2 + e4 * (15 + 150 * e4)));
	assert(*q > 0);
}
static inline double compute_acc_num(double q, int order, int c)
{
	assert(c >= 1);
	assert(c < order * 2);
	int            i = 0;
	int            j = 1;
	double         acc = 0;
	double         q_ii1;
	do
	{
		q_ii1 = ipowp(q, i * (i + 1));
		q_ii1 *= sin((i * 2 + 1) * c * M_PIDouble / order) * j;
		acc += q_ii1;
		j = -j;
		++i;
	} while (fabs(q_ii1) > 1e-100);
	return acc;
}
static inline double compute_acc_den(double q, int order, int c)
{
	assert(c >= 1);
	assert(c < order * 2);
	int            i = 1;
	int            j = -1;
	double         acc = 0;
	double         q_i2;
	do
	{
		q_i2 = ipowp(q, i * i);
		q_i2 *= cos(i * 2 * c * M_PIDouble / order) * j;
		acc += q_i2;
		j = -j;
		++i;
	} while (fabs(q_i2) > 1e-100);
	return acc;
}
static inline double compute_coef(int index, double k, double q, int order)
{
	assert(index >= 0);
	assert(index * 2 < order);
	const int      c = index + 1;
	const double   num = compute_acc_num(q, order, c) * pow(q, 0.25);
	const double   den = compute_acc_den(q, order, c) + 0.5;
	const double   ww = num / den;
	const double   wwsq = ww * ww;
	const double   x = sqrt((1 - wwsq * k) * (1 - wwsq / k)) / (1 + wwsq);
	const double   coef = (1 - x) / (1 + x);
	return coef;
}
static inline void compute_coefs_spec_order_tbw(double coef_arr[], int nbr_coefs, double transition)
{
	assert(nbr_coefs > 0);
	assert(transition > 0);
	assert(transition < 0.5);
	double         k;
	double         q;
	compute_transition_param(&k, &q, transition);
	const int      order = nbr_coefs * 2 + 1;
	// Coefficient calculation
	for (int index = 0; index < nbr_coefs; ++index)
		coef_arr[index] = compute_coef(index, k, q, order);
}
static inline double IIR2thOrder(double *Xi, double *b, double z[2])
{
	double Yi = *b * *Xi + z[0];
	z[0] = z[1];
	z[1] = *b * Yi - *Xi;
	return Yi;
}
typedef struct
{
	unsigned int stages;
	double *path, *z;
	double imOld;
} IIRHilbert;
static inline void ProcessIIRHilbert(IIRHilbert *hil, double Xi, double *re, double *im)
{
	double imTmp;
	imTmp = *re = Xi;
	for (unsigned int j = 0; j < hil->stages; j++)
	{
		imTmp = IIR2thOrder(&imTmp, &hil->path[j], &hil->z[2 * j]);
		*re = IIR2thOrder(re, &hil->path[hil->stages + j], &hil->z[hil->stages * 2 + 2 * j]);
	}
	*im = hil->imOld;
	hil->imOld = imTmp;
}
static float NSEEL_CGEN_CALL iirHilbertProcess(float *blocks, float *offptr, float *x, float *y)
{
	uint32_t offs1 = (uint32_t)(*offptr + NSEEL_CLOSEFACTOR);
	IIRHilbert *hil = (IIRHilbert *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	uint32_t offs2 = (uint32_t)(*y + NSEEL_CLOSEFACTOR);
	float *out = (float *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	double re, im;
	ProcessIIRHilbert(hil, *x, &re, &im);
	out[0] = re;
	out[1] = im;
	return 2;
}
static float NSEEL_CGEN_CALL iirHilbertInit(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext *)opaque;
	float *blocks = c->ram_state;
	float *start1 = parms[0];
	int32_t offs1 = (int32_t)(*start1 + NSEEL_CLOSEFACTOR);
	IIRHilbert *hil = (IIRHilbert *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	unsigned int numStages = (unsigned int)(*parms[1] + NSEEL_CLOSEFACTOR);
	float transition = *parms[2];
	unsigned int numCoefs = numStages << 1;
	size_t memSize = sizeof(IIRHilbert) + (numCoefs + (numStages << 2)) * sizeof(double);
	hil->stages = numStages;
	double *coefs = (double *)malloc(numCoefs * sizeof(double));
	hil->path = (double*)(hil + 1);
	hil->z = hil->path + numCoefs;
	memset(hil->z, 0, (hil->stages << 2) * sizeof(double));
	compute_coefs_spec_order_tbw(coefs, numCoefs, transition);
	unsigned int i;
	// Phase reference path coefficients
	for (i = 1; i < numCoefs; i += 2)
		hil->path[i >> 1] = coefs[i];
	// +90 deg path coefficients
	for (i = 0; i < numCoefs; i += 2)
		hil->path[hil->stages + (i >> 1)] = coefs[i];
	free(coefs);
	hil->imOld = 0.0;
	return (float)(1 + memSize / sizeof(float));
}
typedef struct
{
	float data;
	float maximum, minimum;
} node;
typedef struct
{
	unsigned int capacity, top;
	node *items;
} stack;
static inline void insertDat(stack *s2, float val)
{
	if (!s2->top)
	{
		s2->items[s2->top].data = val;
		s2->items[s2->top].maximum = val;
		s2->items[s2->top++].minimum = val;
	}
	else
	{
		s2->items[s2->top].data = val;
		s2->items[s2->top].minimum = min(val, s2->items[s2->top - 1].minimum);
		s2->items[s2->top++].maximum = max(val, s2->items[s2->top - 1].maximum);
	}
}
static inline void removeDat(stack *s1, stack *s2)
{
	if (s1->top)
		s1->top--;
	else
	{
		while (s2->top)
		{
			insertDat(s1, s2->items[s2->top - 1].data);
			s2->top--;
		}
		s1->top--;
	}
}
typedef struct mmStk
{
	stack s1, s2;
	unsigned int windowSize, cnt;
	void(*process)(struct mmStk *stk, float, float *, float *);
} runningMinMax;
void ProcessStkMaxLater(runningMinMax *stk, float a, float *minV, float *maxV)
{
	removeDat(&stk->s1, &stk->s2);
	insertDat(&stk->s2, a);
	// the maximum of both stack will be the maximum of overall window
	if (stk->s1.top)
	{
		*minV = min(stk->s1.items[stk->s1.top - 1].minimum, stk->s2.items[stk->s2.top - 1].minimum);
		*maxV = max(stk->s1.items[stk->s1.top - 1].maximum, stk->s2.items[stk->s2.top - 1].maximum);
	}
	else
	{
		*minV = stk->s2.items[stk->s2.top - 1].minimum;
		*maxV = stk->s2.items[stk->s2.top - 1].maximum;
	}
}
void ProcessStkBeginning(runningMinMax *stk, float a, float *minV, float *maxV)
{
	stk->cnt++;
	if (stk->cnt >= stk->windowSize)
		stk->process = ProcessStkMaxLater;
	insertDat(&stk->s2, a);
	// the maximum of both stack will be the maximum of overall window
	*minV = stk->s2.items[stk->s2.top - 1].minimum;
	*maxV = stk->s2.items[stk->s2.top - 1].maximum;
}
static float NSEEL_CGEN_CALL movingMinMaxProcess(float *blocks, float *offptr, float *x, float *y)
{
	uint32_t offs1 = (uint32_t)(*offptr + NSEEL_CLOSEFACTOR);
	runningMinMax *stk = (runningMinMax *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	uint32_t offs2 = (uint32_t)(*y + NSEEL_CLOSEFACTOR);
	float *out = (float *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	float minV = 0, maxV = 0;
	stk->process(stk, *x, &minV, &maxV);
	out[0] = minV;
	out[1] = maxV;
	return 2;
}
static float NSEEL_CGEN_CALL movingMinMaxInit(float *blocks, float *offptr, float *parm1)
{
	uint32_t offs1 = (uint32_t)(*offptr + NSEEL_CLOSEFACTOR);
	runningMinMax *stk = (runningMinMax *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	uint32_t windowSize = (uint32_t)(*parm1 + NSEEL_CLOSEFACTOR);
	size_t memSize = sizeof(runningMinMax) + sizeof(node) * windowSize * 2;
	stk->s1.top = 0;
	stk->s1.capacity = windowSize;
	stk->s2.top = 0;
	stk->s2.capacity = windowSize;
	stk->s1.items = (node*)(stk + 1);
	stk->s2.items = (node*)(((char*)stk->s1.items) + sizeof(node) * windowSize);
	stk->windowSize = windowSize;
	stk->cnt = 0;
	stk->process = ProcessStkBeginning;
	return (float)(1 + memSize / sizeof(float));
}
#define ItemLess(a,b)  ((a)<(b))
#define ItemMean(a,b)  (((a)+(b))/2)
typedef struct
{
	float *data;  //circular queue of values
	int *pos;   //index into `heap` for each value
	int *heap;  //max/median/min heap holding indexes into `data`.
	int   N;     //allocated size.
	int   idx;   //position in circular queue
	int   ct;    //count of items in queue
} runningMedian;
#define minCt(m) (((m)->ct-1)/2) //count of items in minheap
#define maxCt(m) (((m)->ct)/2)   //count of items in maxheap 
//returns 1 if heap[i] < heap[j]
static inline int mmless(runningMedian *m, int i, int j)
{
	return ItemLess(m->data[m->heap[i]], m->data[m->heap[j]]);
}
//swaps items i&j in heap, maintains indexes
static inline int mmexchange(runningMedian *m, int i, int j)
{
	int t = m->heap[i];
	m->heap[i] = m->heap[j];
	m->heap[j] = t;
	m->pos[m->heap[i]] = i;
	m->pos[m->heap[j]] = j;
	return 1;
}
//swaps items i&j if i<j;  returns true if swapped
static inline int mmCmpExch(runningMedian *m, int i, int j)
{
	return mmless(m, i, j) && mmexchange(m, i, j);
}
//maintains minheap property for all items below i/2.
static inline void minSortDown(runningMedian *m, int i)
{
	for (; i <= minCt(m); i *= 2)
	{
		if (i > 1 && i < minCt(m) && mmless(m, i + 1, i))
			++i;
		if (!mmCmpExch(m, i, i / 2))
			break;
	}
}
//maintains maxheap property for all items below i/2. (negative indexes)
static inline void maxSortDown(runningMedian *m, int i)
{
	for (; i >= -maxCt(m); i *= 2)
	{
		if (i<-1 && i > -maxCt(m) && mmless(m, i, i - 1))
			--i;
		if (!mmCmpExch(m, i / 2, i))
			break;
	}
}
//maintains minheap property for all items above i, including median
//returns true if median changed
static inline int minSortUp(runningMedian *m, int i)
{
	while (i > 0 && mmCmpExch(m, i, i / 2))
		i /= 2;
	return i == 0;
}
//maintains maxheap property for all items above i, including median
//returns true if median changed
static inline int maxSortUp(runningMedian *m, int i)
{
	while (i < 0 && mmCmpExch(m, i / 2, i))
		i /= 2;
	return i == 0;
}
//Inserts item, maintains median in O(lg nItems)
float runningMedianInsert(runningMedian *m, float v)
{
	int isNew = (m->ct < m->N);
	int p = m->pos[m->idx];
	float old = m->data[m->idx];
	m->data[m->idx] = v;
	m->idx = (m->idx + 1) % m->N;
	m->ct += isNew;
	if (p > 0) //new item is in minHeap
	{
		if (!isNew && ItemLess(old, v))
			minSortDown(m, p * 2);
		else if (minSortUp(m, p))
			maxSortDown(m, -1);
	}
	else if (p < 0) //new item is in maxheap
	{
		if (!isNew && ItemLess(v, old))
			maxSortDown(m, p * 2);
		else if (maxSortUp(m, p))
			minSortDown(m, 1);
	}
	else //new item is at median
	{
		if (maxCt(m))
			maxSortDown(m, -1);
		if (minCt(m))
			minSortDown(m, 1);
	}
	v = m->data[m->heap[0]];
	if ((m->ct & 1) == 0)
		v = ItemMean(v, m->data[m->heap[-1]]);
	return v;
}
static float NSEEL_CGEN_CALL movingMedianProcess(float *blocks, float *offptr, float *x)
{
	uint32_t offs1 = (uint32_t)(*offptr + NSEEL_CLOSEFACTOR);
	runningMedian *m = (runningMedian *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	return runningMedianInsert(m, *x);
}
static float NSEEL_CGEN_CALL movingMedianInit(float *blocks, float *offptr, float *parm1)
{
	uint32_t offs1 = (uint32_t)(*offptr + NSEEL_CLOSEFACTOR);
	runningMedian *m = (runningMedian *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	uint32_t windowSize = (uint32_t)(*parm1 + NSEEL_CLOSEFACTOR);
	size_t memSize = sizeof(runningMedian) + windowSize * (sizeof(float) + sizeof(int) * 2);
	m->data = (float *)(m + 1);
	m->pos = (int *)(m->data + windowSize);
	m->heap = m->pos + windowSize + (windowSize / 2); //points to middle of storage.
	m->N = windowSize;
	m->ct = m->idx = 0;
	while (windowSize--)  //set up initial heap fill pattern: median,max,min,max,...
	{
		m->pos[windowSize] = ((windowSize + 1) / 2) * ((windowSize & 1) ? -1 : 1);
		m->heap[m->pos[windowSize]] = windowSize;
	}
	return (float)(1 + memSize / sizeof(float));
}
void runStateArburg(double *Z, double *b, double x, int n)
{
	for (int j = 1; j < n; j++) // Update conditions
		Z[j - 1] = -b[j] * x + Z[j];
	Z[n - 1] = -b[n] * x;
}
double predictArburg(double *Z, double *a, int n)
{
	double Yi = Z[0]; // Filtered value
	for (int j = 1; j < n; j++) // Update conditions
		Z[j - 1] = Z[j] - a[j] * Yi;
	Z[n - 1] = -a[n] * Yi;
	return Yi; // Write to output
}
void TrainArburg(char *bg, float *xn, int positionStart, int positionEnd)
{
	unsigned int i, j;
	char getPredictionState = *bg;
	unsigned int *flag = (unsigned int*)(bg + 1);
	const unsigned int _mCoefficientsNumber = *flag;
	int to = positionEnd + 1;

	// Creates internal variables with desirable length
	double *predictionCoefficients = (double*)(flag + 1);
	memset(predictionCoefficients, 0, (_mCoefficientsNumber + 1) * sizeof(double));
	double *reflectionCoefficient = predictionCoefficients + (_mCoefficientsNumber + 1);
	memset(reflectionCoefficient, 0, (_mCoefficientsNumber + 1) * sizeof(double));
	double *_r = reflectionCoefficient + (_mCoefficientsNumber + 1);
	memset(_r, 0, (_mCoefficientsNumber + 1) * sizeof(double));
	double *_c = _r + (_mCoefficientsNumber + 1);
	double *_deltaRAndAProduct = _c + (_mCoefficientsNumber + 1);
	double *_g = _deltaRAndAProduct + (_mCoefficientsNumber + 1);
	memset(_g, 0, (_mCoefficientsNumber + 2) * sizeof(double));
	double *tmp = _g + (_mCoefficientsNumber + 2);
	double *forwardState = tmp + (_mCoefficientsNumber + 2);
	double *backwardState = forwardState + (_mCoefficientsNumber + 1);

	// Initializes i_iterationCounter and vectors. For details see step 0 of algorithm on page 3 of the paper
	/// Calculates autocorrelations. For details see step 0 of algorithm on page 3 of the paper
	for (j = 0; j <= _mCoefficientsNumber; j++)
	{
		_c[j] = 0.0;
		for (i = 0; i <= positionEnd - j; i++)
			_c[j] += xn[i + positionStart] * xn[i + positionStart + j];
	}
	unsigned int _iIterationCounter = 0;
	predictionCoefficients[0] = 1.0;
	_g[0] = 2.0 * _c[0] - fabs(xn[0 + positionStart]) * fabs(xn[0 + positionStart]) - fabs(xn[positionEnd + positionStart]) * fabs(xn[positionEnd + positionStart]);
	_g[1] = 2.0 * _c[1];
	// the paper says r[1], error in paper?
	_r[0] = 2.0 * _c[1];

	while (_iIterationCounter <= _mCoefficientsNumber)
	{
		// Computes vector of reflection coefficients. For details see step 1 of algorithm on page 3 of the paper
		double nominator = 0.0;
		double denominator = ((double)FLT_EPSILON) * 100.0;
		for (i = 0; i <= _iIterationCounter + 1; i++)
		{
			nominator += predictionCoefficients[i] * _g[(_iIterationCounter + 1) - i];
			denominator += predictionCoefficients[i] * _g[i];
		}
		if (fabs(nominator) < ((double)FLT_EPSILON) * 10.0 && fabs(denominator) < ((double)FLT_EPSILON) * 10.0)
			reflectionCoefficient[_iIterationCounter] = -1.0 + FLT_EPSILON;
		else
			reflectionCoefficient[_iIterationCounter] = -nominator / denominator;
		// Updates vector of prediction coefficients. For details see step 2 of algorithm on page 3 of the paper
		memcpy(tmp, predictionCoefficients, (_mCoefficientsNumber + 1) * sizeof(double));
		for (i = 0; i <= _iIterationCounter + 1; i++)
			predictionCoefficients[i] = tmp[i] + reflectionCoefficient[_iIterationCounter] * tmp[(_iIterationCounter + 1) - i];
		_iIterationCounter++;
		if (_iIterationCounter == _mCoefficientsNumber)
		{
			if (getPredictionState)
			{
				memset(forwardState, 0, (_mCoefficientsNumber + 1) * sizeof(double));
				memset(backwardState, 0, (_mCoefficientsNumber + 1) * sizeof(double));
				int idx2 = to - 1 + positionStart;
				for (int i = positionStart; i < to; i++)
				{
					runStateArburg(forwardState, predictionCoefficients, xn[i], _mCoefficientsNumber); // Forward
					runStateArburg(backwardState, predictionCoefficients, xn[idx2 - i], _mCoefficientsNumber); // Backward
				}
			}
			break;
		}
		// Updates vector r. For details see step 5 of algorithm on page 3 of the paper
		memcpy(tmp, _r, (_mCoefficientsNumber + 1) * sizeof(double));
		for (i = 0; i <= _iIterationCounter - 1; i++)
			_r[i + 1] = tmp[i] - xn[i + positionStart] * xn[_iIterationCounter + positionStart] - xn[positionEnd - i + positionStart] * xn[positionEnd - _iIterationCounter + positionStart];
		_r[0] = 2.0 * _c[_iIterationCounter + 1];

		// Calculates vector deltaRAndAProduct. For details see step 6 of algorithm on page 3 of the paper
		int posBegin = _iIterationCounter;
		int posEnd = positionEnd - _iIterationCounter;
		for (i = 0; i <= _iIterationCounter; i++)
		{
			double innerProduct1 = 0.0;
			double innerProduct2 = 0.0;
			for (j = 0; j <= _iIterationCounter; j++)
			{
				innerProduct1 += xn[posBegin - j + positionStart] * predictionCoefficients[j];
				innerProduct2 += xn[posEnd + j + positionStart] * predictionCoefficients[j];
			}
			_deltaRAndAProduct[i] = -xn[posBegin - i + positionStart] * innerProduct1 - xn[posEnd + i + positionStart] * innerProduct2;
		}
		// Updates vector g. For details see step 7 of algorithm on page 3 of the paper
		memcpy(tmp, _g, (_mCoefficientsNumber + 2) * sizeof(double));
		// g.Length is i_iterationCounter + 1
		for (i = 0; i <= _iIterationCounter; i++)
			_g[i] = tmp[i] + reflectionCoefficient[_iIterationCounter - 1] * tmp[_iIterationCounter - i] + _deltaRAndAProduct[i];
		for (i = 0; i <= _iIterationCounter; i++)
			_g[_iIterationCounter + 1] += _r[i] * predictionCoefficients[i];
	}
}
static float NSEEL_CGEN_CALL arburgCheckMemoryRequirement(float *blocks, float *parm0, float *parm1)
{
	unsigned int coefficientsNumber = (unsigned int)(*parm0 + NSEEL_CLOSEFACTOR);
	char getPredictionState = (char)(*parm1 + NSEEL_CLOSEFACTOR);
	size_t totalMemSize = sizeof(unsigned int) + ((coefficientsNumber + 1) * (getPredictionState == 1 ? 7 : 5) + (coefficientsNumber + 2) * 2) * sizeof(double);
	return (float)((totalMemSize / sizeof(float) + 1));
}
static float NSEEL_CGEN_CALL arburgTrainModel(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	char *burg = (char*)__NSEEL_RAMAlloc(blocks, (uint64_t)offs);
	int32_t offs1 = (int32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	float *xn = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	*burg = (char)(*parms[2] + NSEEL_CLOSEFACTOR);
	unsigned int *flag = (unsigned int*)(burg + 1);
	*flag = (unsigned int)(*parms[1] + NSEEL_CLOSEFACTOR);
	int32_t from = (int32_t)(*parms[4] + NSEEL_CLOSEFACTOR);
	int32_t to = (int32_t)(*parms[5] + NSEEL_CLOSEFACTOR);
	TrainArburg(burg, xn, from, to);
	return 0.0f;
}
static float NSEEL_CGEN_CALL arburgPredictBackward(float *blocks, float *start)
{
	int32_t offs = (int32_t)(*start + NSEEL_CLOSEFACTOR);
	char *burg = (char*)__NSEEL_RAMAlloc(blocks, (uint64_t)offs);
	unsigned int *flag = (unsigned int*)(burg + 1);
	double *predictionCoefficients = (double*)(flag + 1);
	double *backwardState = ((double*)(flag + 1)) + (*flag + 1) * 5 + (*flag + 2) * 2 + (*flag + 1);
	double output = predictArburg(backwardState, predictionCoefficients, *flag);
	return (float)output;
}
static float NSEEL_CGEN_CALL arburgPredictForward(float *blocks, float *start)
{
	int32_t offs = (int32_t)(*start + NSEEL_CLOSEFACTOR);
	char *burg = (char*)__NSEEL_RAMAlloc(blocks, (uint64_t)offs);
	unsigned int *flag = (unsigned int*)(burg + 1);
	double *predictionCoefficients = (double*)(flag + 1);
	double *forwardState = ((double*)(flag + 1)) + (*flag + 1) * 5 + (*flag + 2) * 2;
	double output = predictArburg(forwardState, predictionCoefficients, *flag);
	return (float)output;
}
void reverse(float *arr, int32_t start, int32_t end)
{
	while (start < end)
	{
		float tmp = arr[start];
		arr[start] = arr[end];
		arr[end] = tmp;
		start++;
		end--;
	}
}
void shift(float *arr, int32_t k, int32_t n)
{
	k = k % n;
	reverse(arr, 0, n - 1);
	reverse(arr, 0, n - k - 1);
	reverse(arr, n - k, n - 1);
}
float * NSEEL_CGEN_CALL __NSEEL_circshift(float *blocks, float *offptr, float *shiftptr, float *lenptr)
{
	uint32_t offs = (uint32_t)(*offptr + NSEEL_CLOSEFACTOR);
	float *arr = __NSEEL_RAMAlloc(blocks, (uint64_t)offs);
	int32_t k = (int32_t)*shiftptr;
	int32_t n = (int32_t)*lenptr;
	k < 0 ? shift(arr, -k, n) : shift(arr, n - k, n);
	return offptr;
}
static float NSEEL_CGEN_CALL _eel_vectorizeAssignScalar(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t resultPtr = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *result = __NSEEL_RAMAlloc(blocks, (uint64_t)resultPtr);
	int32_t numElements = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	for (int32_t i = 0; i < numElements; i++)
		result[i] = *parms[2];
	return 1;
}
static float NSEEL_CGEN_CALL _eel_vectorizeAdd(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t resultPtr = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *result = __NSEEL_RAMAlloc(blocks, (uint64_t)resultPtr);
	int32_t numElements = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	int32_t APtr = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *A = __NSEEL_RAMAlloc(blocks, (uint64_t)APtr);
	int32_t BPtr = (int32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	float *B = __NSEEL_RAMAlloc(blocks, (uint64_t)BPtr);
	for (int32_t i = 0; i < numElements; i++)
		result[i] = A[i] + B[i];
	return 1;
}
static float NSEEL_CGEN_CALL _eel_vectorizeMinus(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t resultPtr = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *result = __NSEEL_RAMAlloc(blocks, (uint64_t)resultPtr);
	int32_t numElements = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	int32_t APtr = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *A = __NSEEL_RAMAlloc(blocks, (uint64_t)APtr);
	int32_t BPtr = (int32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	float *B = __NSEEL_RAMAlloc(blocks, (uint64_t)BPtr);
	for (int32_t i = 0; i < numElements; i++)
		result[i] = A[i] - B[i];
	return 1;
}
static float NSEEL_CGEN_CALL _eel_vectorizeMultiply(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t resultPtr = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *result = __NSEEL_RAMAlloc(blocks, (uint64_t)resultPtr);
	int32_t numElements = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	int32_t APtr = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *A = __NSEEL_RAMAlloc(blocks, (uint64_t)APtr);
	int32_t BPtr = (int32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	float *B = __NSEEL_RAMAlloc(blocks, (uint64_t)BPtr);
	for (int32_t i = 0; i < numElements; i++)
		result[i] = A[i] * B[i];
	return 1;
}
static float NSEEL_CGEN_CALL _eel_vectorizeDivide(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t resultPtr = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *result = __NSEEL_RAMAlloc(blocks, (uint64_t)resultPtr);
	int32_t numElements = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	int32_t APtr = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *A = __NSEEL_RAMAlloc(blocks, (uint64_t)APtr);
	int32_t BPtr = (int32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	float *B = __NSEEL_RAMAlloc(blocks, (uint64_t)BPtr);
	for (int32_t i = 0; i < numElements; i++)
		result[i] = A[i] / B[i];
	return 1;
}
float expint(float x)
{
	if (x <= 1.0f)
		return -0.57721566490153286060651209f - logf(x) + x * (-x / 4.0f + 1.0f);
	float term = 0.0f;
	for (int32_t k = 1 + (int32_t)floorf(80.0f / x); k >= 1; k--)
		term = k / (1.0f + k / (x + term));
	return expf(-x) / (x + term);
}
// Domain: 0.001 ~ 0.5f
static const float ExpintTable1[500] = { 6.33153936413615f, 5.63939143396494f, 5.23492507691165f, 4.94824125651360f, 4.72609545858444f, 4.54477115683906f, 4.39161723405586f, 4.25908210080260f, 4.14229482717614f, 4.03792957653811f, 3.94361416507444f, 3.85759706007702f, 3.77854812837773f, 3.70543343651052f, 3.63743334995231f, 3.57388711871546f, 3.51425429210118f, 3.45808717909408f, 3.40501076461633f, 3.35470778330971f, 3.30690743883812f, 3.26137674984624f, 3.21791382119158f, 3.17634254828989f, 3.13650840321517f, 3.09827504776313f, 3.06152158606426f, 3.02614031708686f, 2.99203488170511f, 2.95911872402128f, 2.92731380507851f, 2.89654952085833f, 2.86676178682571f, 2.83789225917520f, 2.80988766899124f, 2.78269925022877f, 2.75628224608441f, 2.73059548120986f, 2.70560098950218f, 2.68126368902528f, 2.65755109707773f, 2.63443307960029f, 2.61188163007365f, 2.58987067383731f, 2.56837589440108f, 2.54737457884799f, 2.52684547986474f, 2.50676869229857f, 2.48712554244313f, 2.46789848850997f, 2.44907103095643f, 2.43062763152119f, 2.41255363997231f, 2.39483522770240f, 2.37745932741712f, 2.36041357825803f, 2.34368627578265f, 2.32726632629475f, 2.31114320507867f, 2.29530691814378f, 2.27974796713116f, 2.26445731707363f, 2.24942636673543f, 2.23464692128761f, 2.22011116710190f, 2.20581164846892f, 2.19174124606712f, 2.17789315702680f, 2.16426087644945f, 2.15083818025680f, 2.13761910925637f, 2.12459795432145f, 2.11176924259316f, 2.09912772462139f, 2.08666836236871f, 2.07438631800884f, 2.06227694345737f, 2.05033577057783f, 2.03855850201183f, 2.02694100258574f, 2.01547929125139f, 2.00416953352107f, 1.99300803436109f, 1.98199123151100f, 1.97111568919802f, 1.96037809221915f, 1.94977524036537f, 1.93930404316446f, 1.92896151492086f, 1.91874477003266f, 1.90865101856733f, 1.89867756207924f, 1.88882178965323f, 1.87908117415985f, 1.86945326870865f, 1.85993570328723f, 1.85052618157445f, 1.84122247791703f, 1.83202243445968f, 1.82292395841939f, 1.81392501949540f, 1.80502364740671f, 1.79621792954979f, 1.78750600876935f, 1.77888608123583f, 1.77035639442354f, 1.76191524518360f, 1.75356097790666f, 1.74529198277014f, 1.73710669406567f, 1.72900358860208f, 1.72098118418005f, 1.71303803813460f, 1.70517274594176f, 1.69738393988603f, 1.68967028778564f, 1.68203049177242f, 1.67446328712364f, 1.66696744114317f, 1.65954175208938f, 1.65218504814758f, 1.64489618644475f, 1.63767405210444f, 1.63051755734000f, 1.62342564058417f, 1.61639726565340f, 1.60943142094514f, 1.60252711866667f, 1.59568339409389f, 1.58889930485880f, 1.58217393026415f, 1.57550637062431f, 1.56889574663089f, 1.56234119874219f, 1.55584188659529f, 1.54939698843990f, 1.54300570059289f, 1.53666723691273f, 1.53038082829278f, 1.52414572217289f, 1.51796118206826f, 1.51182648711501f, 1.50574093163171f, 1.49970382469613f, 1.49371448973666f, 1.48777226413783f, 1.48187649885916f, 1.47602655806709f, 1.47022181877919f, 1.46446167052028f, 1.45874551499005f, 1.45307276574158f, 1.44744284787039f, 1.44185519771371f, 1.43630926255935f, 1.43080450036407f, 1.42534037948083f, 1.41991637839478f, 1.41453198546754f, 1.40918669868949f, 1.40388002543983f, 1.39861148225397f, 1.39338059459819f, 1.38818689665114f, 1.38302993109194f, 1.37790924889475f, 1.37282440912947f, 1.36777497876843f, 1.36276053249873f, 1.35778065254024f, 1.35283492846881f, 1.34792295704476f, 1.34304434204620f, 1.33819869410729f, 1.33338563056104f, 1.32860477528666f, 1.32385575856114f, 1.31913821691513f, 1.31445179299275f, 1.30979613541537f, 1.30517089864914f, 1.30057574287617f, 1.29601033386927f, 1.29147434287001f, 1.28696744647023f, 1.28248932649660f, 1.27803966989839f, 1.27361816863814f, 1.26922451958530f, 1.26485842441262f, 1.26051958949533f, 1.25620772581287f, 1.25192254885323f, 1.24766377851975f, 1.24343113904031f, 1.23922435887882f, 1.23504317064902f, 1.23088731103039f, 1.22675652068626f, 1.22265054418389f, 1.21856912991660f, 1.21451203002782f, 1.21047900033696f, 1.20646980026724f, 1.20248419277510f, 1.19852194428150f, 1.19458282460475f, 1.19066660689505f, 1.18677306757053f, 1.18290198625489f, 1.17905314571646f, 1.17522633180872f, 1.17142133341224f, 1.16763794237793f, 1.16387595347169f, 1.16013516432024f, 1.15641537535834f, 1.15271638977706f, 1.14903801347337f, 1.14538005500082f, 1.14174232552131f, 1.13812463875802f, 1.13452681094936f, 1.13094866080393f, 1.12739000945649f, 1.12385068042499f, 1.12033049956838f, 1.11682929504554f, 1.11334689727494f, 1.10988313889530f, 1.10643785472705f, 1.10301088173459f, 1.09960205898947f, 1.09621122763424f, 1.09283823084711f, 1.08948291380742f, 1.08614512366176f, 1.08282470949081f, 1.07952152227697f, 1.07623541487250f, 1.07296624196851f, 1.06971386006443f, 1.06647812743823f, 1.06325890411715f, 1.06005605184912f, 1.05686943407471f, 1.05369891589963f, 1.05054436406786f, 1.04740564693527f, 1.04428263444374f, 1.04117519809585f, 1.03808321093007f, 1.03500654749642f, 1.03194508383258f, 1.02889869744056f, 1.02586726726374f, 1.02285067366439f, 1.01984879840164f, 1.01686152460984f, 1.01388873677738f, 1.01093032072589f, 1.00798616358984f, 1.00505615379652f, 1.00214018104644f, 0.999238136294059f, 0.996349911728865f, 0.993475400756876f, 0.990614497982423f, 0.987767099190299f, 0.984933101328234f, 0.982112402489699f, 0.979304901897022f, 0.976510499884809f, 0.973729097883682f, 0.970960598404302f, 0.968204905021682f, 0.965461922359790f, 0.962731556076425f, 0.960013712848367f, 0.957308300356788f, 0.954615227272931f, 0.951934403244034f, 0.949265738879512f, 0.946609145737375f, 0.943964536310890f, 0.941331824015476f, 0.938710923175825f, 0.936101749013249f, 0.933504217633244f, 0.930918246013275f, 0.928343751990763f, 0.925780654251284f, 0.923228872316966f, 0.920688326535087f, 0.918158938066864f, 0.915640628876432f, 0.913133321720014f, 0.910636940135260f, 0.908151408430781f, 0.905676651675847f, 0.903212595690257f, 0.900759167034383f, 0.898316292999373f, 0.895883901597516f, 0.893461921552770f, 0.891050282291436f, 0.888648913932996f, 0.886257747281089f, 0.883876713814638f, 0.881505745679123f, 0.879144775677990f, 0.876793737264197f, 0.874452564531905f, 0.872121192208285f, 0.869799555645474f, 0.867487590812641f, 0.865185234288193f, 0.862892423252093f, 0.860609095478304f, 0.858335189327350f, 0.856070643738996f, 0.853815398225033f, 0.851569392862186f, 0.849332568285124f, 0.847104865679583f, 0.844886226775585f, 0.842676593840777f, 0.840475909673856f, 0.838284117598100f, 0.836101161455002f, 0.833926985597995f, 0.831761534886266f, 0.829604754678678f, 0.827456590827773f, 0.825316989673861f, 0.823185898039208f, 0.821063263222303f, 0.818949032992211f, 0.816843155583011f, 0.814745579688315f, 0.812656254455868f, 0.810575129482225f, 0.808502154807510f, 0.806437280910244f, 0.804380458702259f, 0.802331639523674f, 0.800290775137951f, 0.798257817727021f, 0.796232719886476f, 0.794215434620836f, 0.792205915338876f, 0.790204115849027f, 0.788209990354839f, 0.786223493450506f, 0.784244580116458f, 0.782273205715011f, 0.780309325986082f, 0.778352897042962f, 0.776403875368148f, 0.774462217809232f, 0.772527881574849f, 0.770600824230679f, 0.768681003695509f, 0.766768378237340f, 0.764862906469556f, 0.762964547347142f, 0.761073260162954f, 0.759189004544038f, 0.757311740448002f, 0.755441428159437f, 0.753578028286381f, 0.751721501756837f, 0.749871809815336f, 0.748028914019544f, 0.746192776236916f, 0.744363358641393f, 0.742540623710147f, 0.740724534220363f, 0.738915053246067f, 0.737112144154998f, 0.735315770605516f, 0.733525896543553f, 0.731742486199606f, 0.729965504085765f, 0.728194914992784f, 0.726430683987183f, 0.724672776408401f, 0.722921157865967f, 0.721175794236724f, 0.719436651662080f, 0.717703696545296f, 0.715976895548810f, 0.714256215591593f, 0.712541623846539f, 0.710833087737889f, 0.709130574938690f, 0.707434053368279f, 0.705743491189805f, 0.704058856807781f, 0.702380118865662f, 0.700707246243463f, 0.699040208055393f, 0.697378973647534f, 0.695723512595532f, 0.694073794702333f, 0.692429789995936f, 0.690791468727175f, 0.689158801367534f, 0.687531758606980f, 0.685910311351833f, 0.684294430722649f, 0.682684088052144f, 0.681079254883129f, 0.679479902966478f, 0.677886004259118f, 0.676297530922046f, 0.674714455318365f, 0.673136750011346f, 0.671564387762516f, 0.669997341529763f, 0.668435584465468f, 0.666879089914658f, 0.665327831413178f, 0.663781782685891f, 0.662240917644895f, 0.660705210387756f, 0.659174635195774f, 0.657649166532257f, 0.656128779040825f, 0.654613447543725f, 0.653103147040173f, 0.651597852704708f, 0.650097539885575f, 0.648602184103115f, 0.647111761048182f, 0.645626246580576f, 0.644145616727490f, 0.642669847681983f, 0.641198915801459f, 0.639732797606177f, 0.638271469777766f, 0.636814909157763f, 0.635363092746164f, 0.633915997699998f, 0.632473601331907f, 0.631035881108752f, 0.629602814650226f, 0.628174379727492f, 0.626750554261822f, 0.625331316323269f, 0.623916644129338f, 0.622506516043683f, 0.621100910574808f, 0.619699806374795f, 0.618303182238032f, 0.616911017099967f, 0.615523290035870f, 0.614139980259605f, 0.612761067122424f, 0.611386530111769f, 0.610016348850085f, 0.608650503093652f, 0.607288972731423f, 0.605931737783878f, 0.604578778401895f, 0.603230074865621f, 0.601885607583366f, 0.600545357090505f, 0.599209304048392f, 0.597877429243282f, 0.596549713585273f, 0.595226138107252f, 0.593906683963851f, 0.592591332430423f, 0.591280064902018f, 0.589972862892376f, 0.588669708032933f, 0.587370582071829f, 0.586075466872932f, 0.584784344414874f, 0.583497196790091f, 0.582214006203880f, 0.580934754973459f, 0.579659425527039f, 0.578388000402911f, 0.577120462248533f, 0.575856793819634f, 0.574596977979323f, 0.573340997697209f, 0.572088836048530f, 0.570840476213289f, 0.569595901475400f, 0.568355095221847f, 0.567118040941839f, 0.565884722225992f, 0.564655122765501f, 0.563429226351332f, 0.562207016873418f, 0.560988478319864f, 0.559773594776161f };
// Domain: 0.5f ~ 1.5
static const float ExpintTable2[251] = { 0.559773594776161f, 0.554950295774654f, 0.550184228566447f, 0.545474419724144f, 0.540819918846431f, 0.536219797845636f, 0.531673150262605f, 0.527179090607617f, 0.522736753726183f, 0.518345294188611f, 0.514003885702249f, 0.509711720545442f, 0.505468009022217f, 0.501271978936804f, 0.497122875087133f, 0.493019958776493f, 0.488962507342567f, 0.484949813703121f, 0.480981185917635f, 0.477055946764210f, 0.473173433331126f, 0.469332996622433f, 0.465534001177009f, 0.461775824700544f, 0.458057857709903f, 0.454379503189402f, 0.450740176258502f, 0.447139303850470f, 0.443576324401589f, 0.440050687550483f, 0.436561853847192f, 0.433109294471587f, 0.429692490960805f, 0.426310934945320f, 0.422964127893360f, 0.419651580863333f, 0.416372814263966f, 0.413127357621880f, 0.409914749356315f, 0.406734536560751f, 0.403586274791166f, 0.400469527860700f, 0.397383867640485f, 0.394328873866420f, 0.391304133951693f, 0.388309242804826f, 0.385343802653065f, 0.382407422870921f, 0.379499719813686f, 0.376620316655740f, 0.373768843233509f, 0.370944935892892f, 0.368148237341009f, 0.365378396502136f, 0.362635068377678f, 0.359917913910035f, 0.357226599850257f, 0.354560798629336f, 0.351920188233033f, 0.349304452080114f, 0.346713278903895f, 0.344146362636973f, 0.341603402299064f, 0.339084101887818f, 0.336588170272547f, 0.334115321090748f, 0.331665272647358f, 0.329237747816627f, 0.326832473946554f, 0.324449182765786f, 0.322087610292923f, 0.319747496748133f, 0.317428586467031f, 0.315130627816727f, 0.312853373114005f, 0.310596578545543f, 0.308360004090134f, 0.306143413442834f, 0.303946573940985f, 0.301769256492064f, 0.299611235503289f, 0.297472288812947f, 0.295352197623386f, 0.293250746435620f, 0.291167722985511f, 0.289102918181468f, 0.287056126043634f, 0.285027143644513f, 0.283015771050992f, 0.281021811267725f, 0.279045070181840f, 0.277085356508928f, 0.275142481740288f, 0.273216260091375f, 0.271306508451441f, 0.269413046334320f, 0.267535695830332f, 0.265674281559272f, 0.263828630624464f, 0.261998572567834f, 0.260183939326000f, 0.258384565187321f, 0.256600286749911f, 0.254830942880567f, 0.253076374674603f, 0.251336425416562f, 0.249610940541772f, 0.247899767598750f, 0.246202756212403f, 0.244519758048025f, 0.242850626776061f, 0.241195218037623f, 0.239553389410736f, 0.237925000377293f, 0.236309912290710f, 0.234707988344255f, 0.233119093540036f, 0.231543094658634f, 0.229979860229366f, 0.228429260501155f, 0.226891167414003f, 0.225365454571042f, 0.223851997211154f, 0.222350672182151f, 0.220861357914491f, 0.219383934395520f, 0.217918283144238f, 0.216464287186559f, 0.215021831031070f, 0.213590800645265f, 0.212171083432249f, 0.210762568207898f, 0.209365145178468f, 0.207978705918639f, 0.206603143349984f, 0.205238351719856f, 0.203884226580681f, 0.202540664769647f, 0.201207564388785f, 0.199884824785426f, 0.198572346533028f, 0.197270031412369f, 0.195977782393090f, 0.194695503615591f, 0.193423100373256f, 0.192160479095018f, 0.190907547328241f, 0.189664213721924f, 0.188430388010206f, 0.187205980996190f, 0.185990904536040f, 0.184785071523391f, 0.183588395874023f, 0.182400792510829f, 0.181222177349038f, 0.180052467281716f, 0.178891580165522f, 0.177739434806718f, 0.176595950947427f, 0.175461049252136f, 0.174334651294438f, 0.173216679544007f, 0.172107057353797f, 0.171005708947475f, 0.169912559407060f, 0.168827534660787f, 0.167750561471176f, 0.166681567423308f, 0.165620480913305f, 0.164567231136999f, 0.163521748078805f, 0.162483962500777f, 0.161453805931852f, 0.160431210657273f, 0.159416109708196f, 0.158408436851462f, 0.157408126579554f, 0.156415114100704f, 0.155429335329182f, 0.154450726875736f, 0.153479226038189f, 0.152514770792199f, 0.151557299782162f, 0.150606752312267f, 0.149663068337703f, 0.148726188455997f, 0.147796053898504f, 0.146872606522027f, 0.145955788800575f, 0.145045543817253f, 0.144141815256283f, 0.143244547395151f, 0.142353685096878f, 0.141469173802417f, 0.140590959523164f, 0.139718988833596f, 0.138853208864020f, 0.137993567293430f, 0.137140012342489f, 0.136292492766605f, 0.135450957849129f, 0.134615357394647f, 0.133785641722382f, 0.132961761659695f, 0.132143668535687f, 0.131331314174900f, 0.130524650891108f, 0.129723631481212f, 0.128928209219219f, 0.128138337850319f, 0.127353971585044f, 0.126575065093524f, 0.125801573499819f, 0.125033452376345f, 0.124270657738376f, 0.123513146038632f, 0.122760874161947f, 0.122013799420013f, 0.121271879546203f, 0.120535072690475f, 0.119803337414337f, 0.119076632685907f, 0.118354917875021f, 0.117638152748432f, 0.116926297465068f, 0.116219312571358f, 0.115517158996635f, 0.114819798048592f, 0.114127191408815f, 0.113439301128373f, 0.112756089623470f, 0.112077519671165f, 0.111403554405148f, 0.110734157311576f, 0.110069292224971f, 0.109408923324170f, 0.108753015128340f, 0.108101532493039f, 0.107454440606342f, 0.106811704985010f, 0.106173291470726f, 0.105539166226369f, 0.104909295732348f, 0.104283646782986f, 0.103662186482950f, 0.103044882243734f, 0.102431701780185f, 0.101822613107082f, 0.101217584535761f, 0.100616584670779f, 0.100019582406633f };
// Domain 1.5 ~ 3.5
static const float ExpintTable3[126] = { 0.100019582406633f, 0.0976709372158198f, 0.0953838384149070f, 0.0931564276459467f, 0.0909869129830044f, 0.0888735660844122f, 0.0868147194907422f, 0.0848087640597390f, 0.0828541465300625f, 0.0809493672062480f, 0.0790929777578066f, 0.0772835791258635f, 0.0755198195311743f, 0.0738003925777604f, 0.0721240354467908f, 0.0704895271756689f, 0.0688956870176292f, 0.0673413728774234f, 0.0658254798189725f, 0.0643469386411095f, 0.0629047145177793f, 0.0614978056992866f, 0.0601252422713888f, 0.0587860849692256f, 0.0574794240432553f, 0.0562043781745348f, 0.0549600934368431f, 0.0537457423032842f, 0.0525605226951504f, 0.0514036570709528f, 0.0502743915536392f, 0.0491719950941388f, 0.0480957586694767f, 0.0470449945137897f, 0.0460190353806841f, 0.0450172338354382f, 0.0440389615756621f, 0.0430836087790745f, 0.0421505834771471f, 0.0412393109534225f, 0.0403492331653865f, 0.0394798081888161f, 0.0386305096836003f, 0.0378008263800681f, 0.0369902615849173f, 0.0361983327058716f, 0.0354245707942568f, 0.0346685201047068f, 0.0339297376712618f, 0.0332077928991558f, 0.0325022671716214f, 0.0318127534710799f, 0.0311388560141009f, 0.0304801898995688f, 0.0298363807694920f, 0.0292070644819443f, 0.0285918867956309f, 0.0279905030656117f, 0.0274025779497217f, 0.0268277851252616f, 0.0262658070155479f, 0.0257163345259216f, 0.0251790667888503f, 0.0246537109177589f, 0.0241399817692477f, 0.0236376017133719f, 0.0231463004116726f, 0.0226658146026547f, 0.0221958878944312f, 0.0217362705642599f, 0.0212867193647089f, 0.0208469973362054f, 0.0204168736257225f, 0.0199961233113807f, 0.0195845272327421f, 0.0191818718265862f, 0.0187879489679693f, 0.0184025558163696f, 0.0180254946667384f, 0.0176565728052777f, 0.0172956023697725f, 0.0169424002143170f, 0.0165967877782784f, 0.0162585909593461f, 0.0159276399905230f, 0.0156037693209245f, 0.0152868175002407f, 0.0149766270667522f, 0.0146730444387568f, 0.0143759198093052f, 0.0140851070441270f, 0.0138004635826326f, 0.0135218503418941f, 0.0132491316235018f, 0.0129821750231990f, 0.0127208513432019f, 0.0124650345071144f, 0.0122146014773545f, 0.0119694321750053f, 0.0117294094020133f, 0.0114944187656559f, 0.0112643486052043f, 0.0110390899207104f, 0.0108185363038491f, 0.0106025838707484f, 0.0103911311967455f, 0.0101840792530053f, 0.00998133134494335f, 0.00978279305239503f, 0.00958837217147620f, 0.00939797865808188f, 0.00921152457297160f, 0.00902892402839181f, 0.00885009313618758f, 0.00867494995735709f, 0.00850341445300474f, 0.00833540843664957f, 0.00817085552784730f, 0.00800968110708622f, 0.00785181227191778f, 0.00769717779428456f, 0.00754570807900947f, 0.00739733512341117f, 0.00725199247801176f, 0.00710961520830429f, 0.00697013985754840f };
// Domain 3.5 ~ 11.436
static const float ExpintTable4[63] = { 0.00697013985754843f, 0.00595165118439979f, 0.00508661641561241f, 0.00435102957490895f, 0.00372481546087525f, 0.00319115004588169f, 0.00273590601139406f, 0.00234719870089483f, 0.00201501303487467f, 0.00173089598574009f, 0.00148770235377602f, 0.00127938403852772f, 0.00110081492605982f, 0.000947645033184157f, 0.000816178756486768f, 0.000703273036244310f, 0.000606252016086186f, 0.000522835399236460f, 0.000451078202821178f, 0.000389320017508240f, 0.000336142209771689f, 0.000290331773353231f, 0.000250850756880151f, 0.000216810375484428f, 0.000187449063132408f, 0.000162113845196945f, 0.000140244512382263f, 0.000121360161310043f, 0.000105047737015578f, 9.09522708167131e-05f, 7.87685555639048e-05f, 6.82340408359011e-05f, 5.91227645848651e-05f, 5.12401661828662e-05f, 4.44186497036028e-05f, 3.85137863511616e-05f, 3.34010618509128e-05f, 2.89730888669951e-05f, 2.51372165380621e-05f, 2.18134793867002e-05f, 1.89328364567230e-05f, 1.64356588152578e-05f, 1.42704297310207e-05f, 1.23926270802452e-05f, 1.07637619830281e-05f, 9.35055145753116e-06f, 8.12420610016641e-06f, 7.05981654290921e-06f, 6.13582477694144e-06f, 5.33356842622777e-06f, 4.63688775713486e-06f, 4.03178666455650e-06f, 3.50614011821845e-06f, 3.04944161624825e-06f, 2.65258510327885e-06f, 2.30767658985817e-06f, 2.00787137790676e-06f, 1.74723336968152e-06f, 1.52061342901041e-06f, 1.32354418525242e-06f, 1.15214903255314e-06f, 1.00306338807548e-06f, 8.73366540296160e-07f };
float linear_value(float Start, float Step, float Input, const float *Space)
{
	int32_t Index = (int32_t)((Input - Start) / Step);
	float X1 = Start + Step * Index;
	return Space[Index] + (Space[Index + 1] - Space[Index]) / (Start + Step * (Index + 1) - X1) * (Input - X1);
}
float expint_interpolation(float x)
{
	if (x < 0.001f)
		return 6.33153936413615f;
	else if (x < 0.5f)
		return (float)linear_value(0.001f, 0.001f, (float)x, ExpintTable1);// 0.001:0.001:0.5f
	else if (x < 1.5f)
		return (float)linear_value(0.5f, 0.004f, (float)x, ExpintTable2); // 0.5f:0.004:1.5
	else if (x < 3.5f)
		return (float)linear_value(1.5f, 0.016f, (float)x, ExpintTable3);// 1.5:0.016:3.5
	else if (x < 11.436f)
		return (float)linear_value(3.5f, 0.128f, (float)x, ExpintTable4);// 3.5:0.128:11.5
	else
		return 8.73366540296160e-07f;
}
float invsqrt(float x)
{
	return 1.0f / sqrtf(x);
}
#include "fft.h"
static void fft_reorder_buffer(int32_t bitsz, WDL_FFT_COMPLEX *data, int32_t fwd)
{
	const int32_t *tab = fft_reorder_table_for_bitsize(bitsz);
	if (!fwd)
	{
		while (*tab)
		{
			const int32_t sidx = *tab++;
			WDL_FFT_COMPLEX a = data[sidx];
			for (;;)
			{
				WDL_FFT_COMPLEX ta;
				const int32_t idx = *tab++;
				if (!idx) break;
				ta = data[idx];
				data[idx] = a;
				a = ta;
			}
			data[sidx] = a;
		}
	}
	else
	{
		while (*tab)
		{
			const int32_t sidx = *tab++;
			int32_t lidx = sidx;
			const WDL_FFT_COMPLEX sta = data[lidx];
			for (;;)
			{
				const int32_t idx = *tab++;
				if (!idx) break;
				data[lidx] = data[idx];
				lidx = idx;
			}
			data[lidx] = sta;
		}
	}
}
// 0=fw, 1=iv, 2=fwreal, 3=ireal, 4=permutec, 6=permuter
// low bit: is inverse
// second bit: was isreal, but no longer used
// third bit: is permute
static void FFT(int32_t sizebits, float *data, int32_t dir)
{
	if (dir >= 4 && dir < 8)
	{
		if (dir == 4 || dir == 5)
			fft_reorder_buffer(sizebits, (WDL_FFT_COMPLEX*)data, dir == 4);
	}
	else if (dir >= 0 && dir < 2)
		WDL_fft((WDL_FFT_COMPLEX*)data, 1 << sizebits, dir & 1);
	else if (dir >= 2 && dir < 4)
		WDL_real_fft((float*)data, 1 << sizebits, dir & 1);
}
static float * fft_func(int32_t dir, float *blocks, float *start, float *length)
{
	const int32_t offs = (uint32_t)(*start + NSEEL_CLOSEFACTOR);
	const int32_t itemSizeShift = (dir & 2) ? 0 : 1;
	int32_t l = (uint32_t)(*length + NSEEL_CLOSEFACTOR);
	int32_t bitl = 0;
	int32_t ilen;
	float *ptr;
	while (l > 1 && bitl < EEL_FFT_MAXBITLEN)
	{
		bitl++;
		l >>= 1;
	}
	if (bitl < ((dir & 4) ? EEL_FFT_MINBITLEN_REORDER : EEL_FFT_MINBITLEN))  // smallest FFT is 16 item, smallest reorder is 8 item
		return start;
	ilen = 1 << bitl;
	// check to make sure we don't cross a boundary
	if (offs / NSEEL_RAM_ITEMSPERBLOCK != (offs + (ilen << itemSizeShift) - 1) / NSEEL_RAM_ITEMSPERBLOCK)
	{
		return start;
	}
	ptr = __NSEEL_RAMAlloc(blocks, (uint64_t)offs);
	FFT(bitl, ptr, dir);
	return start;
}
static float NSEEL_CGEN_CALL  eel_max(float *blocks, float *start, float *length)
{
	float *ptr = __NSEEL_RAMAlloc(blocks, (uint32_t)(*start + NSEEL_CLOSEFACTOR));
	float ma = ptr[0];
	for (uint32_t i = 1; i < (uint32_t)(*length + NSEEL_CLOSEFACTOR); i++)
	{
		if (fabsf(ptr[i]) > fabsf(ma))
			ma = ptr[i];
	}
	return ma;
}
static float NSEEL_CGEN_CALL  eel_min(float *blocks, float *start, float *length)
{
	float *ptr = __NSEEL_RAMAlloc(blocks, (uint32_t)(*start + NSEEL_CLOSEFACTOR));
	float mi = ptr[0];
	for (uint32_t i = 1; i < (uint32_t)(*length + NSEEL_CLOSEFACTOR); i++)
	{
		if (fabsf(ptr[i]) < fabsf(mi))
			mi = ptr[i];
	}
	return mi;
}
static float NSEEL_CGEN_CALL  eel_mean(float *blocks, float *start, float *length)
{
	float *ptr = __NSEEL_RAMAlloc(blocks, (uint32_t)(*start + NSEEL_CLOSEFACTOR));
	float mm = ptr[0];
	for (uint32_t i = 1; i < (uint32_t)(*length + NSEEL_CLOSEFACTOR); i++)
		mm += ptr[i];
	return mm / *length;
}
#define ELEM_SWAP(a,b) { register float t=(a);(a)=(b);(b)=t; }
/*---------------------------------------------------------------------------
   Function :   kth_smallest()
   In       :   array of elements, # of elements in the array, rank k
   Out      :   one element
   Job      :   find the kth smallest element in the array
   Notice   :   use the median() macro defined below to get the median.

				Reference:

				  Author: Wirth, Niklaus
				   Title: Algorithms + data structures = programs
			   Publisher: Englewood Cliffs: Prentice-Hall, 1976
	Physical description: 366 p.
				  Series: Prentice-Hall Series in Automatic Computation

 ---------------------------------------------------------------------------*/
float kth_smallest(float a[], int n, int k)
{
	register i, j, l, m;
	register float x;

	l = 0; m = n - 1;
	while (l < m) {
		x = a[k];
		i = l;
		j = m;
		do {
			while (a[i] < x) i++;
			while (x < a[j]) j--;
			if (i <= j) {
				ELEM_SWAP(a[i], a[j]);
				i++; j--;
			}
		} while (i <= j);
		if (j < k) l = i;
		if (k < i) m = j;
	}
	if (n % 2)
		return a[k];
	else
		return (a[k] + a[k + 1]) * 0.5f;
}
#define median_wirth(a,n) kth_smallest(a,n,(((n)&1)?((n)/2):(((n)/2)-1)))
static float NSEEL_CGEN_CALL  eel_median(float *blocks, float *start, float *length)
{
	float *ptr = __NSEEL_RAMAlloc(blocks, (uint32_t)(*start + NSEEL_CLOSEFACTOR));
	uint32_t n = (uint32_t)(*length + NSEEL_CLOSEFACTOR);
	float *tmp = (float*)malloc(n * sizeof(float));
	memcpy(tmp, ptr, n * sizeof(float));
	float med = median_wirth(tmp, n);
	free(tmp);
	return med;
}
static float * NSEEL_CGEN_CALL  eel_fft(float *blocks, float *start, float *length)
{
	return fft_func(0, blocks, start, length);
}
static float * NSEEL_CGEN_CALL  eel_ifft(float *blocks, float *start, float *length)
{
	return fft_func(1, blocks, start, length);
}
static float * NSEEL_CGEN_CALL  eel_fft_real(float *blocks, float *start, float *length)
{
	return fft_func(2, blocks, start, length);
}
static float * NSEEL_CGEN_CALL  eel_ifft_real(float *blocks, float *start, float *length)
{
	return fft_func(3, blocks, start, length);
}
static float * NSEEL_CGEN_CALL  eel_fft_permute(float *blocks, float *start, float *length)
{
	return fft_func(4, blocks, start, length);
}
static float * NSEEL_CGEN_CALL  eel_ifft_permute(float *blocks, float *start, float *length)
{
	return fft_func(5, blocks, start, length);
}
static float * NSEEL_CGEN_CALL eel_convolve_c(float *blocks, float *dest, float *src, float *lenptr)
{
	const int32_t dest_offs = (int32_t)(*dest + NSEEL_CLOSEFACTOR);
	const int32_t src_offs = (int32_t)(*src + NSEEL_CLOSEFACTOR);
	const int32_t len = ((int32_t)(*lenptr + NSEEL_CLOSEFACTOR)) * 2;
	float *srcptr, *destptr;
	if (len < 1 || len > NSEEL_RAM_ITEMSPERBLOCK || dest_offs < 0 || src_offs < 0 ||
		dest_offs >= NSEEL_RAM_ITEMSPERBLOCK || src_offs >= NSEEL_RAM_ITEMSPERBLOCK) return dest;
	if ((dest_offs&(NSEEL_RAM_ITEMSPERBLOCK - 1)) + len > NSEEL_RAM_ITEMSPERBLOCK) return dest;
	if ((src_offs&(NSEEL_RAM_ITEMSPERBLOCK - 1)) + len > NSEEL_RAM_ITEMSPERBLOCK) return dest;
	srcptr = __NSEEL_RAMAlloc(blocks, (uint64_t)src_offs);
	destptr = __NSEEL_RAMAlloc(blocks, (uint64_t)dest_offs);
	WDL_fft_complexmul((WDL_FFT_COMPLEX*)destptr, (WDL_FFT_COMPLEX*)srcptr, (len / 2)&~1);
	return dest;
}
static inline int32_t nseel_stringsegments_tobuf(char *bufOut, int32_t bufout_sz, eelStringSegmentRec *list) // call with NULL to calculate size, or non-null to generate to buffer (returning size used)
{
	int32_t pos = 0;
	while (list)
	{
		if (!bufOut)
			pos += list->str_len;
		else if (list->str_len > 1)
		{
			if (pos >= bufout_sz) break;
			pos += nseel_filter_escaped_string(bufOut + pos, bufout_sz - pos, list->str_start + 1, list->str_len - 1, list->str_start[0]);
		}
		list = list->_next;
	}
	return pos;
}
static inline void eelThread_start(abstractThreads *info)
{
	// ensure worker is waiting
	pthread_mutex_lock(&(info->work_mtx));
	// set job information & state
	info->state = EEL_WORKING;
	// wake-up signal
	pthread_cond_signal(&(info->work_cond));
	pthread_mutex_unlock(&(info->work_mtx));
}
static inline void eelThread_wait(abstractThreads *info)
{
	while (1)
	{
		pthread_cond_wait(&(info->boss_cond), &(info->boss_mtx));
		if (EEL_IDLE == info->state)
			break;
	}
}
void* GetStringForIndex(eel_builtin_memRegion *st, float val, int32_t write)
{
	int32_t idx = (int32_t)(val + 0.5f);
	if (!write)
	{
		s_str *tmp = &st->memRegion[idx];
		const char *s = s_str_c_str(tmp);
		return (void*)s;
	}
	else
		return (void*)&st->memRegion[idx];
}
static inline int32_t getEmptyRegion(eel_builtin_memRegion *st)
{
	int32_t x;
	for (x = 0; x < st->slot; x++)
		if (!st->memRegion[x])
			break;
	if (st->inuse > (st->slot - 1))
	{
		st->slot += 10;
		st->memRegion = (void**)realloc(st->memRegion, st->slot * sizeof(void*));
		st->memRegion[st->inuse] = 0;
		st->type = (char*)realloc(st->type, st->slot * sizeof(char));
		for (int32_t i = st->inuse; i < st->slot; i++)
		{
			st->memRegion[i] = 0;
			st->type[i] = 0;
		}
	}
	st->inuse++;
	return x;
}
void DeleteSlot(eel_builtin_memRegion *st, float val)
{
	int32_t idx = (int32_t)(val + 0.5f);
	if (!st->memRegion[idx])
		goto decrement;
	if (st->type[idx] == 0)
		s_str_destroy(&st->memRegion[idx]);
	else if (st->type[idx] == 1)
	{
		FFTConvolver1x1 *conv = (FFTConvolver1x1 *)st->memRegion[idx];
		FFTConvolver1x1Free(conv);
		free(conv);
	}
	else if (st->type[idx] == 2)
	{
		FFTConvolver2x2 *conv = (FFTConvolver2x2 *)st->memRegion[idx];
		FFTConvolver2x2Free(conv);
		free(conv);
	}
	else if (st->type[idx] == 3)
	{
		FFTConvolver2x4x2 *conv = (FFTConvolver2x4x2 *)st->memRegion[idx];
		FFTConvolver2x4x2Free(conv);
		free(conv);
	}
	else if (st->type[idx] == 4)
	{
		abstractThreads *ptr = (abstractThreads *)st->memRegion[idx];
		// ensure the worker is waiting
		if (ptr->state == EEL_WORKING)
			eelThread_wait(ptr);
		pthread_mutex_lock(&(ptr->work_mtx));
		ptr->state = EEL_GET_OFF_FROM_WORK;
		// wake-up signal
		pthread_cond_signal(&(ptr->work_cond));
		pthread_mutex_unlock(&(ptr->work_mtx));
		// wait for thread to exit
		pthread_join(ptr->threadID, NULL);
		pthread_mutex_destroy(&(ptr->work_mtx));
		pthread_cond_destroy(&(ptr->work_cond));
		pthread_mutex_unlock(&(ptr->boss_mtx));
		pthread_mutex_destroy(&(ptr->boss_mtx));
		pthread_cond_destroy(&(ptr->boss_cond));
		NSEEL_code_free(ptr->codePtr);
		free(ptr);
	}
	else
		free(st->memRegion[idx]);
	st->memRegion[idx] = 0;
decrement:
	st->inuse--;
}
void FreeRegion_context(eel_builtin_memRegion *st)
{
	for (int32_t i = 0; i < st->slot; i++)
		DeleteSlot(st, (float)i + 0.1f);
	free(st->type);
	free(st->memRegion);
	st->slot = 0;
	st->inuse = 0;
}
int32_t AddData(eel_builtin_memRegion *st, void *ns, char type)
{
	int32_t x = getEmptyRegion(st);
	if (type == 0)
	{
		st->memRegion[x] = s_str_create_from_c_str((char *)ns);
		free(ns);
	}
	else
		st->memRegion[x] = ns;
	st->type[x] = type;
	return x;
}
static float addStringCallback(void *opaque, eelStringSegmentRec *list)
{
	compileContext *c = (compileContext*)opaque;
	// could probably do a faster implementation using AddRaw() etc but this should also be OK
	int32_t sz = nseel_stringsegments_tobuf(NULL, 0, list);
	char *ns = (char*)malloc(sz + 32);
	memset(ns, 0, sz + 32);
	sz = nseel_stringsegments_tobuf(ns, sz, list) + 1;
	ns = (char*)realloc(ns, sz);
	int32_t id = AddData(c->region_context, (void*)ns, 0);
	return (float)id;
}
static const xpr *xConstant[19] = { &xZero, &xOne, &xTwo, &xTen, &xPinf, &xMinf, &xNaN, &xPi, &xPi2, &xPi4, &xEe, &xSqrt2, &xLn2, &xLn10, &xLog2_e, &xLog2_10, &xLog10_e, &HPA_MIN, &HPA_MAX };
static const char xVarName[19][10] = { "$hZero", "$hOne", "$hTwo", "$hTen", "$hPinf", "$hMinf", "$hNaN", "$hPi", "$hPi2", "$hPi4", "$hEe", "$hSqrt2", "$hLn2", "$hLn10", "$hLog2_e", "$hLog2_10", "$hLog10_e", "$HPA_MIN", "$HPA_MAX" };
static inline void InitRegion_context(eel_builtin_memRegion *st)
{
	st->inuse = 0;
	st->slot = 30;
	st->type = (char *)malloc(st->slot * sizeof(char));
	st->memRegion = (s_str *)malloc(st->slot * sizeof(s_str));
	for (int32_t i = 0; i < st->slot; i++)
	{
		st->type[i] = 0;
		st->memRegion[i] = 0;
	}
	for (int32_t i = 0; i < 19; i++)
	{
		xpr br = *xConstant[i];
		void *ptr = malloc(sizeof(xpr));
		memcpy(ptr, (void *)&br, sizeof(xpr));
		int32_t id = AddData(st, ptr, 5);
	}
}
static int32_t eel_string_match(compileContext *c, const char *fmt, const char *msg, int32_t match_fmt_pos, int32_t ignorecase, const char *fmt_endptr, const char *msg_endptr, int32_t num_fmt_parms, float **fmt_parms)
{
	// %d=12345
	// %f=12345[.678]
	// %c=any nonzero char, ascii value
	// %x=12354ab
	// %*, %?, %+, %% literals
	// * ? +  match minimal groups of 0+,1, or 1+ chars
	for (;;)
	{
		if (fmt >= fmt_endptr)
		{
			if (msg >= msg_endptr) return 1;
			return 0; // format ends before matching string
		}

		// if string ends and format is not on a wildcard, early-out to 0
		if (msg >= msg_endptr && *fmt != '*' && *fmt != '%') return 0;

		switch (*fmt)
		{
		case '*':
		case '+':
			// if last char of search pattern, we're done!
			if (fmt + 1 >= fmt_endptr || (fmt[1] == '?' && fmt + 2 >= fmt_endptr)) return *fmt == '*' || msg < msg_endptr;

			if (fmt[0] == '+')  msg++; // skip a character for + . Note that in this case msg[1] is valid, because of the !*msg && *fmt != '*' check above

			fmt++;
			if (*fmt == '?')
			{
				// *? or +? are lazy matches
				fmt++;

				while (msg < msg_endptr && !eel_string_match(c, fmt, msg, match_fmt_pos, ignorecase, fmt_endptr, msg_endptr, num_fmt_parms, fmt_parms)) msg++;
				return msg < msg_endptr;
			}
			else
			{
				// greedy match
				int32_t len = (int32_t)(msg_endptr - msg);
				while (len >= 0 && !eel_string_match(c, fmt, msg + len, match_fmt_pos, ignorecase, fmt_endptr, msg_endptr, num_fmt_parms, fmt_parms)) len--;
				return len >= 0;
			}
			break;
		case '?':
			fmt++;
			msg++;
			break;
		case '%':
		{
			fmt++;
			unsigned short fmt_minlen = 1, fmt_maxlen = 0;
			if (*fmt >= '0' && *fmt <= '9')
			{
				fmt_minlen = *fmt++ - '0';
				while (*fmt >= '0' && *fmt <= '9') fmt_minlen = fmt_minlen * 10 + (*fmt++ - '0');
				fmt_maxlen = fmt_minlen;
			}
			if (*fmt == '-')
			{
				fmt++;
				fmt_maxlen = 0;
				while (*fmt >= '0' && *fmt <= '9') fmt_maxlen = fmt_maxlen * 10 + (*fmt++ - '0');
			}
			const char *dest_varname = NULL;
			if (*fmt == '{')
			{
				dest_varname = ++fmt;
				while (*fmt && fmt < fmt_endptr && *fmt != '}') fmt++;
				if (fmt >= fmt_endptr - 1 || *fmt != '}') return 0; // malformed %{var}s
				fmt++; // skip '}'
			}
			char fmt_char = *fmt++;
			if (!fmt_char) return 0; // malformed
			if (fmt_char == '*' || fmt_char == '?' || fmt_char == '+' || fmt_char == '%')
			{
				if (*msg++ != fmt_char) return 0;
			}
			else if (fmt_char == 'c')
			{
				float *varOut = NULL;
				float vv = 0.0f;
				if (!dest_varname)
				{
					if (match_fmt_pos < num_fmt_parms)
						varOut = fmt_parms[match_fmt_pos];
					match_fmt_pos++;
				}
				if (msg >= msg_endptr) return 0; // out of chars
				if (varOut)
				{
					if (varOut == &vv) // %{#foo}c
					{
						s_str *wr = (s_str*)GetStringForIndex(c->region_context, vv, 1);
						if (wr)
							s_str_destroy(wr);
						*wr = s_str_create_from_c_str(msg);
					}
					else
					{
						*varOut = (float)*(unsigned char *)msg;
					}
				}
				msg++;
			}
			else
			{
				int32_t len = 0;
				int32_t lazy = 0;
				if (fmt_char >= 'A'&&fmt_char <= 'Z') { lazy = 1; fmt_char += 'a' - 'A'; }
				if (fmt_char == 's')
				{
					len = (int32_t)(msg_endptr - msg);
				}
				else if (fmt_char == 'x')
				{
					while ((msg[len] >= '0' && msg[len] <= '9') || (msg[len] >= 'A' && msg[len] <= 'F') || (msg[len] >= 'a' && msg[len] <= 'f'))
						len++;
				}
				else if (fmt_char == 'f')
				{
					if (msg[len] == '-') len++;
					while (msg[len] >= '0' && msg[len] <= '9') len++;
					if (msg[len] == '.')
					{
						len++;
						while (msg[len] >= '0' && msg[len] <= '9') len++;
					}
				}
				else if (fmt_char == 'd' || fmt_char == 'u' || fmt_char == 'i')
				{
					if (fmt_char != 'u' && msg[len] == '-') len++;
					while (msg[len] >= '0' && msg[len] <= '9') len++;
				}
				else
				{
					// bad format
					return 0;
				}
				if (fmt_maxlen > 0 && len > fmt_maxlen) len = fmt_maxlen;
				if (!dest_varname) match_fmt_pos++;
				if (lazy)
				{
					if (fmt_maxlen<1 || fmt_maxlen>len) fmt_maxlen = len;
					len = fmt_minlen;
					while (len <= fmt_maxlen && !eel_string_match(c, fmt, msg + len, match_fmt_pos, ignorecase, fmt_endptr, msg_endptr, num_fmt_parms, fmt_parms)) len++;
					if (len > fmt_maxlen) return 0;
				}
				else
				{
					while (len >= fmt_minlen && !eel_string_match(c, fmt, msg + len, match_fmt_pos, ignorecase, fmt_endptr, msg_endptr, num_fmt_parms, fmt_parms)) len--;
					if (len < fmt_minlen) return 0;
				}
				float vv = 0.0f;
				float *varOut = NULL;
				if (!dest_varname)
				{
					if (match_fmt_pos > 0 && match_fmt_pos - 1 < num_fmt_parms)
						varOut = fmt_parms[match_fmt_pos - 1];
				}
				if (varOut)
				{
					if (fmt_char == 's')
					{
						s_str *wr = (s_str*)GetStringForIndex(c->region_context, *varOut, 1);
						if (wr)
							s_str_destroy(wr);
						if (wr)
						{
							const char *strS = s_str_c_str(wr);
							int32_t le = strlen(strS);
							if (msg_endptr >= strS && msg_endptr <= strS + le)
							{
#ifdef EEL_STRING_DEBUGOUT
								EEL_STRING_DEBUGOUT("match: destination specifier passed is also haystack, will not update");
#endif
							}
							else if (fmt_endptr >= strS && fmt_endptr <= strS + le)
							{
#ifdef EEL_STRING_DEBUGOUT
								EEL_STRING_DEBUGOUT("match: destination specifier passed is also format, will not update");
#endif
							}
							else
							{
								*wr = s_str_create_from_c_str(msg);
							}
						}
						else
						{
#ifdef EEL_STRING_DEBUGOUT
							EEL_STRING_DEBUGOUT("match: bad destination specifier passed as %d: %f", match_fmt_pos, *varOut);
#endif
						}
					}
					else
					{
						char tmp[128];
						lstrcpyn_safe(tmp, msg, min(len + 1, (int32_t)sizeof(tmp)));
						if (varOut == &vv)
						{
							s_str *wr = (s_str*)GetStringForIndex(c->region_context, vv, 1);
							if (wr)
								s_str_destroy(wr);
							*wr = s_str_create_from_c_str(tmp);
						}
						else
						{
							char *bl = (char*)msg;
							if (fmt_char == 'u')
								*varOut = (float)strtoul(tmp, &bl, 10);
							else if (fmt_char == 'x')
								*varOut = (float)strtoul(msg, &bl, 16);
							else
								*varOut = (float)atof(tmp);
						}
					}
				}
				return 1;
			}
		}
		break;
		default:
			if (ignorecase ? (toupper(*fmt) != toupper(*msg)) : (*fmt != *msg)) return 0;
			fmt++;
			msg++;
			break;
		}
	}
}
static float NSEEL_CGEN_CALL getCosineWindows(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext *)opaque;
	float *blocks = c->ram_state;
	float *start1 = parms[0];
	int32_t offs2 = (int32_t)(*start1 + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	float *fmt_index = parms[2];
	const char *fmt = (const char *)GetStringForIndex(c->region_context, *fmt_index, 0);
	float *start2 = parms[1];
	genWnd(indexer, (int32_t)(*start2 + NSEEL_CLOSEFACTOR), fmt);
	return 0;
}
static float NSEEL_CGEN_CALL getAsymmetricCosine(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext *)opaque;
	float *blocks = c->ram_state;
	uint32_t offs1 = (uint32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *analysisWnd = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	uint32_t offs2 = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	float *synthesisWnd = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	int32_t fftlen = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	int32_t ovpSmps = (int32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	float tf_res = *parms[4];
	getAsymmetricWindow(analysisWnd, synthesisWnd, fftlen, ovpSmps, tf_res);
	return 0;
}
static float NSEEL_CGEN_CALL _eel_match(void *opaque, INT_PTR num_parms, float **parms)
{
	if (num_parms >= 2)
	{
		compileContext *c = (compileContext*)opaque;
		const char *fmt = (const char*)GetStringForIndex(c->region_context, *parms[0], 0);
		const char *msg = (const char*)GetStringForIndex(c->region_context, *parms[1], 0);
		if (fmt && msg)
			return eel_string_match(c, fmt, msg, 0, 0, fmt + strlen(fmt), msg + strlen(msg), (int32_t)num_parms - 2, parms + 2) ? 1.0f : 0.0f;
	}
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_matchi(void *opaque, INT_PTR num_parms, float **parms)
{
	if (num_parms >= 2)
	{
		compileContext *c = (compileContext*)opaque;
		const char *fmt = (const char*)GetStringForIndex(c->region_context, *parms[0], 0);
		const char *msg = (const char*)GetStringForIndex(c->region_context, *parms[1], 0);
		if (fmt && msg)
			return eel_string_match(opaque, fmt, msg, 0, 1, fmt + strlen(fmt), msg + strlen(msg), (int32_t)num_parms - 2, parms + 2) ? 1.0f : 0.0f;
	}
	return 0.0f;
}
static int32_t eel_validate_format_specifier(const char *fmt_in, char *typeOut, char *fmtOut, int32_t fmtOut_sz, char *varOut, int32_t varOut_sz)
{
	const char *fmt = fmt_in + 1;
	int32_t state = 0;
	if (fmt_in[0] != '%') return 0; // ugh passed a non-
	*varOut = 0;
	if (fmtOut_sz-- < 2) return 0;
	*fmtOut++ = '%';
	while (*fmt)
	{
		const char c = *fmt++;
		if (fmtOut_sz < 2) return 0;
		if (c == 'f' || c == 'F' || c == 'e' || c == 'E' || c == 'g' || c == 'G' || c == 'd' || c == 'u' || c == 'x' || c == 'X' || c == 'c' || c == 'C' || c == 's' || c == 'S' || c == 'i')
		{
			*typeOut = c;
			fmtOut[0] = c;
			fmtOut[1] = 0;
			return (int32_t)(fmt - fmt_in);
		}
		else if (c == '.')
		{
			*fmtOut++ = c; fmtOut_sz--;
			if (state&(2)) break;
			state |= 2;
		}
		else if (c == '+')
		{
			*fmtOut++ = c; fmtOut_sz--;
			if (state&(32 | 16 | 8 | 4)) break;
			state |= 8;
		}
		else if (c == '-' || c == ' ')
		{
			*fmtOut++ = c; fmtOut_sz--;
			if (state&(32 | 16 | 8 | 4)) break;
			state |= 16;
		}
		else if (c >= '0' && c <= '9')
		{
			*fmtOut++ = c; fmtOut_sz--;
			state |= 4;
		}
		else
			break;
	}
	return 0;
}
int32_t eel_format_strings(void *opaque, const char *fmt, const char *fmt_end, char *buf, uint32_t buf_sz, int32_t num_fmt_parms, float **fmt_parms)
{
	int32_t fmt_parmpos = 0;
	char *op = buf;
	while ((fmt_end ? fmt < fmt_end : *fmt) && op < buf + buf_sz - 128)
	{
		if (fmt[0] == '%' && fmt[1] == '%')
		{
			*op++ = '%';
			fmt += 2;
		}
		else if (fmt[0] == '%')
		{
			char ct = 0;
			char fs[128];
			char varname[128];
			const int32_t l = eel_validate_format_specifier(fmt, &ct, fs, sizeof(fs), varname, sizeof(varname));
			if (!l || !ct)
			{
				*op = 0;
				return -1;
			}
			const float *varptr = NULL;
			if (fmt_parmpos < num_fmt_parms)
				varptr = fmt_parms[fmt_parmpos];
			fmt_parmpos++;
			float v = varptr ? (float)*varptr : 0.0f;
			if (ct == 'x' || ct == 'X' || ct == 'd' || ct == 'u' || ct == 'i')
			{
				stbsp_snprintf(op, 128, fs, (int32_t)(v));
			}
			else if (ct == 's' || ct == 'S')
			{
				compileContext *c = (compileContext*)opaque;
				const char *str = (const char*)GetStringForIndex(c->region_context, v, 0);
				const size_t maxl = (size_t)(buf + buf_sz - 2u - op);
				stbsp_snprintf(op, maxl, fs, str ? str : "");
			}
			else if (ct == 'F')
			{
				compileContext *c = (compileContext*)opaque;
				int32_t idx = (int32_t)(v + NSEEL_CLOSEFACTOR);
				xpr *br = (xpr *)c->region_context->memRegion[idx];
				char *str = xpr_asprint(*br, 1, 0, (XDIM * 48) / 10 - 2);
				int maxl = strlen(str);
				stbsp_snprintf(op, min(maxl + 1, 128), "%s", str ? str : "");
				free(str);
			}
			else if (ct == 'c')
			{
				*op++ = (char)(int32_t)v;
				*op = 0;
			}
			else if (ct == 'C')
			{
				const uint32_t iv = (uint32_t)v;
				int32_t bs = 0;
				if (iv & 0xff000000) bs = 24;
				else if (iv & 0x00ff0000) bs = 16;
				else if (iv & 0x0000ff00) bs = 8;
				while (bs >= 0)
				{
					const char c = (char)(iv >> bs);
					*op++ = c ? c : ' ';
					bs -= 8;
				}
				*op = 0;
			}
			else
				stbsp_snprintf(op, 128, fs, v);
			while (*op) op++;
			fmt += l;
		}
		else
			*op++ = *fmt++;
	}
	*op = 0;
	return (int32_t)(op - buf);
}
float NSEEL_CGEN_CALL _eel_printf(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param > 0)
	{
		compileContext *c = (compileContext*)opaque;
		const char *fmt = (const char*)GetStringForIndex(c->region_context, *(parms[0]), 0);
		if (fmt)
		{
			int32_t stringLength = strlen(fmt);
			const int32_t len = eel_format_strings(opaque, fmt, fmt ? (fmt + stringLength) : NULL, c->printfbuf, sizeof(c->printfbuf), num_param - 1, parms + 1);
			if (len > 0)
			{
				EEL_STRING_STDOUT_WRITE(c->printfbuf, len);
				return 1.0f;
			}
			else
			{
				const char *badStr = "printf: bad format string";
				EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
			}
		}
		else
		{
			const char *badStr = "printf: bad format specifier passed";
			EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
		}
	}
	return 0.0f;
}
float NSEEL_CGEN_CALL _eel_sprintf(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param > 0)
	{
		compileContext *c = (compileContext*)opaque;
		s_str *wr = (s_str*)GetStringForIndex(c->region_context, *(parms[0]), 1);
		if (wr)
			s_str_destroy(wr);
		const char *fmt = (const char*)GetStringForIndex(c->region_context, *(parms[1]), 0);
		if (wr && fmt)
		{
			int32_t stringLength = strlen(fmt);
			const int32_t len = eel_format_strings(opaque, fmt, fmt ? (fmt + stringLength) : NULL, c->printfbuf, sizeof(c->printfbuf), num_param - 2, parms + 2);
			if (len > 0)
			{
				*wr = s_str_create_from_c_str(c->printfbuf);
				return 1.0f;
			}
			else
			{
				const char *badStr = "printf: bad format string";
				EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
			}
		}
		else
		{
			const char *badStr = "printf: bad format specifier passed";
			EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
		}
	}
	return 0.0f;
}
static float _eel_strcmp_int(const char *a, int32_t a_len, const char *b, int32_t b_len, int32_t ml, int32_t ignorecase)
{
	// binary-safe comparison (at least if a_len>=0 etc)
	int32_t pos = 0;
	for (;;)
	{
		if (ml > 0 && pos == ml) return 0.0f;
		const int32_t a_end = a_len >= 0 ? pos == a_len : !a[pos];
		const int32_t b_end = b_len >= 0 ? pos == b_len : !b[pos];
		if (a_end || b_end)
		{
			if (!b_end) return -1.0f; // b[pos] is nonzero, a[pos] is zero
			if (!a_end) return 1.0f;
			return 0.0f;
		}
		char av = a[pos];
		char bv = b[pos];
		if (ignorecase)
		{
			av = toupper(av);
			bv = toupper(bv);
		}
		if (bv > av) return -1.0f;
		if (av > bv) return 1.0f;
		pos++;
	}
}
static float NSEEL_CGEN_CALL _eel_strncmp(void *opaque, float *aa, float *bb, float *maxlen)
{
	compileContext *c = (compileContext*)opaque;
	const char *a = (const char*)GetStringForIndex(c->region_context, *aa, 0);
	const char *b = (const char*)GetStringForIndex(c->region_context, *bb, 0);
	if (!a || !b)
	{
		const char *badStr = "strncmp: bad specifier(s)";
		EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
	}
	else
	{
		const int32_t ml = maxlen ? (int32_t)(*maxlen + NSEEL_CLOSEFACTOR) : -1;
		if (!ml || a == b) return 0; // strncmp(x,y,0) == 0
		return _eel_strcmp_int(a, a ? strlen(a) : -1, b, b ? strlen(b) : -1, ml, 0);
	}
	return -1.0f;
}
static float NSEEL_CGEN_CALL _eel_strnicmp(void *opaque, float *aa, float *bb, float *maxlen)
{
	compileContext *c = (compileContext*)opaque;
	const char *a = (const char*)GetStringForIndex(c->region_context, *aa, 0);
	const char *b = (const char*)GetStringForIndex(c->region_context, *bb, 0);
	if (!a || !b)
	{
		const char *badStr = "strnicmp: bad specifier(s)";
		EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
	}
	else
	{
		const int32_t ml = maxlen ? (int32_t)(*maxlen + NSEEL_CLOSEFACTOR) : -1;
		if (!ml || a == b) return 0; // strncmp(x,y,0) == 0
		return _eel_strcmp_int(a, a ? strlen(a) : -1, b, b ? strlen(b) : -1, ml, 1);
	}
	return -1.0f;
}
static float NSEEL_CGEN_CALL _eel_strcmp(void *opaque, float *strOut, float *fmt_index)
{
	return _eel_strncmp(opaque, strOut, fmt_index, NULL);
}
static float NSEEL_CGEN_CALL _eel_stricmp(void *opaque, float *strOut, float *fmt_index)
{
	return _eel_strnicmp(opaque, strOut, fmt_index, NULL);
}
static float NSEEL_CGEN_CALL _eel_strlen(void *opaque, float *fmt_index)
{
	compileContext *c = (compileContext*)opaque;
	const char *fmt = (const char*)GetStringForIndex(c->region_context, *fmt_index, 0);
	if (fmt)
		return (float)strlen(fmt);
	return 0.0f;
}
#ifdef _WIN32
#include <direct.h>
#include "dirent.h"
#else
#include <unistd.h>
#include <dirent.h>
#endif
static float NSEEL_CGEN_CALL _eel_ls(void *opaque, float *fmt_index)
{
	DIR *dir;
	struct dirent *ent;
	char badStr[128];
	if ((dir = opendir(".")) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			stbsp_snprintf(badStr, 128, "%s\n", ent->d_name);
			EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
		}
		closedir(dir);
	}
	else
		return -1;
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_cd(void *opaque, float *fmt_index)
{
	if (opaque)
	{
		compileContext *c = (compileContext*)opaque;
		const char *fmt = (const char*)GetStringForIndex(c->region_context, *fmt_index, 0);
		if (fmt)
#ifdef _WIN32
			return (float)_chdir(fmt);
#else
			return chdir(fmt);
#endif
	}
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_eval(void *opaque, float *s)
{
	compileContext *r = (compileContext*)opaque;
	NSEEL_CODEHANDLE ch = NULL;
	if (r)
	{
		const char *str = (const char*)GetStringForIndex(r->region_context, *s, 0);
		if (!str)
		{
			const char *badStr = "eval() passed invalid string handle\n";
			EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
		}
		else
		{
			if (!ch) ch = NSEEL_code_compile(r, str, 0);
			if (ch)
			{
				NSEEL_code_execute(ch);
				NSEEL_code_free(ch);
				return 1.0f;
			}
			else
			{
				const char *err = NSEEL_code_getcodeerror(r);
				if (err)
				{
					//				EEL_STRING_DEBUGOUT("eval() error: %s", err);
				}
			}
		}
	}
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_evalFile(void *opaque, float *s)
{
	compileContext *r = (compileContext*)opaque;
	if (r)
	{
		const char *str = (const char*)GetStringForIndex(r->region_context, *s, 0);
		if (!str)
		{
			const char *badStr = "eval() passed invalid string handle\n";
			EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
		}
		else
		{
			FILE *fp = fopen(str, "rb");
			NSEEL_CODEHANDLE ch;
			if (fp)
			{
				s_str code = s_str_create();
				s_str_clear(&code);
				char line[4096];
				for (;;)
				{
					line[0] = 0;
					fgets(line, sizeof(line), fp);
					if (!line[0]) break;
					s_str_append_c_str(&code, line);
				}
				fclose(fp);
				const char *codetext = s_str_c_str(&code);
				ch = NSEEL_code_compile(r, codetext, 0);
				s_str_destroy(&code);
				if (ch)
				{
					NSEEL_code_execute(ch);
					NSEEL_code_free(ch);
					return 1.0f;
				}
				else
				{
					const char *err = NSEEL_code_getcodeerror(r);
					if (err)
					{
						EEL_STRING_STDOUT_WRITE(err, strlen(err));
					}
				}
			}
		}
	}
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_base64_encode(void *opaque, float *destination, float *source, float *maxlen)
{
	if (opaque)
	{
		compileContext *c = (compileContext*)opaque;
		s_str *dest = (s_str*)GetStringForIndex(c->region_context, *destination, 1);
		if (dest)
			s_str_destroy(dest);
		s_str *srcClass = (s_str*)GetStringForIndex(c->region_context, *source, 1);
		size_t len = (size_t)(*maxlen + NSEEL_CLOSEFACTOR);
		if (len > s_str_capacity(srcClass))
			return -1;
		const char *src = s_str_c_str(srcClass);
		if (dest && src)
		{
			unsigned char *out, *pos;
			const unsigned char *end, *in;
			size_t olen;
			int32_t line_len;
			olen = len * 4 / 3 + 4; // 3-byte blocks to 4-byte
			olen += olen / 72; // line feeds
			olen++; // nul termination
			if (olen < len)
				return 0; // integer overflow
			out = (unsigned char*)malloc(olen);
			if (out == 0)
				return 0;
			end = src + len;
			in = src;
			pos = out;
			line_len = 0;
			while (end - in >= 3)
			{
				*pos++ = base64_table[in[0] >> 2];
				*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
				*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
				*pos++ = base64_table[in[2] & 0x3f];
				in += 3;
				line_len += 4;
				if (line_len >= 72)
				{
					*pos++ = '\n';
					line_len = 0;
				}
			}
			if (end - in)
			{
				*pos++ = base64_table[in[0] >> 2];
				if (end - in == 1) {
					*pos++ = base64_table[(in[0] & 0x03) << 4];
					*pos++ = '=';
				}
				else
				{
					*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
					*pos++ = base64_table[(in[1] & 0x0f) << 2];
				}
				*pos++ = '=';
				line_len += 4;
			}
			if (line_len)
				*pos++ = '\n';
			*pos = '\0';
			size_t out_len = pos - out;
			*dest = s_str_create_from_c_str(out);
			free(out);
			return (float)out_len;
		}
	}
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_base64_encodeBinaryToTextFile(void *opaque, float *destination, float *source)
{
	if (opaque)
	{
		compileContext *c = (compileContext*)opaque;
		const char *dest = (const char*)GetStringForIndex(c->region_context, *destination, 0);
		const char *src = (const char *)GetStringForIndex(c->region_context, *source, 0);
		if (dest && src)
		{
			unsigned char *buffer = 0;
			long length;
			FILE *textFile = fopen(src, "rb");
			if (textFile)
			{
				fseek(textFile, 0, SEEK_END);
				length = ftell(textFile);
				fseek(textFile, 0, SEEK_SET);
				buffer = (unsigned char*)malloc(length + 1);
				if (buffer)
					fread((void*)buffer, 1, length, textFile);
				fclose(textFile);
				buffer[length] = '\0';
			}
			size_t outLen = 0;
			unsigned char *out;
			if (buffer)
			{
				out = base64_encode(buffer, length, &outLen);
				free(buffer);
				FILE *fp = fopen(dest, "w");
				if (fp)
				{
					fwrite(out, 1, outLen, fp);
					fclose(fp);
				}
				free(out);
			}
			return (float)outLen;
		}
	}
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_base64_decode(void *opaque, float *destination, float *source, float *maxlen)
{
	if (opaque)
	{
		compileContext *c = (compileContext*)opaque;
		s_str *dest = (s_str*)GetStringForIndex(c->region_context, *destination, 1);
		if (dest)
			s_str_destroy(dest);
		s_str *srcClass = (s_str*)GetStringForIndex(c->region_context, *source, 1);
		size_t len = (size_t)(*maxlen + NSEEL_CLOSEFACTOR);
		if (len > s_str_capacity(srcClass))
			return -1;
		const char *src = s_str_c_str(srcClass);
		if (dest && src)
		{
			unsigned char dtable[256], *out, *pos, block[4], tmp;
			size_t i, count, olen;
			int32_t pad = 0;
			memset(dtable, 0x80, 256);
			for (i = 0; i < sizeof(base64_table) - 1; i++)
				dtable[base64_table[i]] = (unsigned char)i;
			dtable['='] = 0;
			count = 0;
			for (i = 0; i < len; i++)
				if (dtable[src[i]] != 0x80)
					count++;
			if (count == 0 || count % 4)
				return 0;
			olen = count / 4 * 3;
			pos = out = (unsigned char*)malloc(olen);
			if (out == NULL)
				return 0;
			count = 0;
			for (i = 0; i < len; i++)
			{
				tmp = dtable[src[i]];
				if (tmp == 0x80)
					continue;
				if (src[i] == '=')
					pad++;
				block[count] = tmp;
				count++;
				if (count == 4)
				{
					*pos++ = (block[0] << 2) | (block[1] >> 4);
					*pos++ = (block[1] << 4) | (block[2] >> 2);
					*pos++ = (block[2] << 6) | block[3];
					count = 0;
					if (pad)
					{
						if (pad == 1)
							pos--;
						else if (pad == 2)
							pos -= 2;
						else
						{
							// Invalid padding
							free(out);
							return 0;
						}
						break;
					}
				}
			}
			size_t out_len = pos - out;
			*dest = s_str_create_from_c_str_0Inc(out, out_len);
			free(out);
			return (float)out_len;
		}
	}
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_base64_decodeBinaryToTextFile(void *opaque, float *destination, float *source)
{
	if (opaque)
	{
		compileContext *c = (compileContext*)opaque;
		const char *dest = (const char*)GetStringForIndex(c->region_context, *destination, 0);
		const char *src = (const char *)GetStringForIndex(c->region_context, *source, 0);
		if (dest && src)
		{
			unsigned char *buffer = 0;
			long length;
			FILE *textFile = fopen(src, "r");
			if (textFile)
			{
				fseek(textFile, 0, SEEK_END);
				length = ftell(textFile);
				fseek(textFile, 0, SEEK_SET);
				buffer = (unsigned char*)malloc(length + 1);
				if (buffer)
					fread(buffer, 1, length, textFile);
				fclose(textFile);
				buffer[length] = '\0';
			}
			size_t outLen = 0;
			unsigned char *out;
			if (buffer)
			{
				out = base64_decode(buffer, length, &outLen);
				free(buffer);
				FILE *fp = fopen(dest, "wb");
				if (fp)
				{
					fwrite(out, 1, outLen, fp);
					fclose(fp);
				}
				free(out);
			}
			return (float)outLen;
		}
	}
	return 0.0f;
}
static float NSEEL_CGEN_CALL _eel_resetSysMemRegion(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	eel_builtin_memRegion *state = c->region_context;
	FreeRegion_context(state);
	InitRegion_context(state);
	return 0.0f;
}
int32_t get_float(char *val, float *F)
{
	char *eptr;
	errno = 0;
	float f = strtof(val, &eptr);
	if (eptr != val && errno != ERANGE)
	{
		*F = f;
		return 1;
	}
	return 0;
}
float* string2FloatArray(char *frArbitraryEqString, int32_t *elements)
{
	char *p = frArbitraryEqString;
	char *counter = frArbitraryEqString;
	int32_t i = 0, count = 0;
	float number;
	while (*p)
	{
		if (get_float(p, &number))
		{
			strtod(p, &p);
			count++;
		}
		else
			p++;
	}
	*elements = count;
	float *arrayF = (float*)malloc(count * sizeof(float));
	while (*counter)
	{
		if (get_float(counter, &number))
		{
			arrayF[i] = (float)strtod(counter, &counter);
			i++;
		}
		else
			counter++;
	}
	return arrayF;
}
static float NSEEL_CGEN_CALL _eel_importFloatArrayFromString(void *opaque, float *fn_index, float *pointer)
{
	compileContext *c = (compileContext*)opaque;
	const char *FLTBuf = (const char*)GetStringForIndex(c->region_context, *fn_index, 0);
	uint32_t offs1 = (uint32_t)(*pointer + NSEEL_CLOSEFACTOR);
	float *userspaceFLT = __NSEEL_RAMAlloc(c->ram_state, offs1);
	int32_t elements;
	float *convertedFLT = string2FloatArray((char*)FLTBuf, &elements);
	memcpy(userspaceFLT, convertedFLT, elements * sizeof(float));
	free(convertedFLT);
	return (float)elements;
}
void fractionalDelayLine_clear(float *fdl)
{
	for (int32_t i = 0; i < (int32_t)fdl[2]; i++)
		fdl[5 + i] = 0.;
}
static float NSEEL_CGEN_CALL fractionalDelayLineInit(float *blocks, float *start, float *length)
{
	int32_t offs1 = (int32_t)(*start + NSEEL_CLOSEFACTOR);
	int32_t max_length = (int32_t)(*length + NSEEL_CLOSEFACTOR);
	float *fdl = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	fdl[0] = 0;
	fdl[1] = 0;
	fdl[2] = (float)max_length;
	fractionalDelayLine_clear(fdl);
	return (float)(max_length + 5);
}
static float NSEEL_CGEN_CALL fractionalDelayLineClear(float *blocks, float *start)
{
	int32_t offs1 = (int32_t)(*start + NSEEL_CLOSEFACTOR);
	float *fdl = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	fractionalDelayLine_clear(fdl);
	return 1;
}
static float NSEEL_CGEN_CALL fractionalDelayLineSetDelay(float *blocks, float *start, float *lg)
{
	uint32_t offs1 = (uint32_t)(*start + NSEEL_CLOSEFACTOR);
	float lag = *lg;
	float *fdl = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	float outPointer;
	if (lag > fdl[2] - 1.0f)
		outPointer = fdl[0] + 1.0f; // force delay to max_length
	else
		outPointer = fdl[0] - lag; // read chases write
	while (outPointer < 0)
		outPointer += fdl[2]; // modulo maximum length
	fdl[1] = (float)((int32_t)outPointer); // integer part
	fdl[3] = outPointer - fdl[1]; // fractional part
	fdl[4] = 1.0f - fdl[3]; // 1.0f - fractional part (more efficient)
	return 1;
}
static float NSEEL_CGEN_CALL fractionalDelayLineProcess(float *blocks, float *start, float *x)
{
	uint32_t offs1 = (uint32_t)(*start + NSEEL_CLOSEFACTOR);
	float *fdl = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int32_t inPtr = (int32_t)(fdl[0] + NSEEL_CLOSEFACTOR);
	fdl[5 + inPtr++] = *x;
	int32_t len = (int32_t)(fdl[2] + NSEEL_CLOSEFACTOR);
	if (inPtr == len) // Check for end condition
		inPtr -= len;
	fdl[0] = (float)inPtr;
	int32_t outPtr = (int32_t)(fdl[1] + NSEEL_CLOSEFACTOR);
	float lastOutput = fdl[5 + outPtr++] * fdl[4]; // first 1/2 of interpolation
	if (outPtr < len) // Check for end condition
		lastOutput += fdl[5 + outPtr] * fdl[3]; // second 1/2 of interpolation
	else
	{
		lastOutput += fdl[5] * fdl[3]; // second 1/2 of interpolation
		outPtr -= len;
	}
	fdl[1] = (float)outPtr;
	return lastOutput;
}
static float NSEEL_CGEN_CALL FIRInit(float *blocks, float *start, float *length)
{
	uint32_t offs1 = (uint32_t)(*start + NSEEL_CLOSEFACTOR);
	int32_t hlen = (int32_t)(*length + NSEEL_CLOSEFACTOR);
	float *fir = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	memset(fir, 0, (hlen + 2) * sizeof(float));
	fir[1] = (float)hlen;
	return (float)(hlen + 2);
}
static float NSEEL_CGEN_CALL FIRProcess(float *blocks, float *start, float *x, float *coe)
{
	int32_t offs1 = (int32_t)(*start + NSEEL_CLOSEFACTOR);
	float *fir = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int32_t offs2 = (int32_t)(*coe + NSEEL_CLOSEFACTOR);
	float *coeffs = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	float *coeff = coeffs;
	int32_t coeffslength = (int32_t)(fir[1] + NSEEL_CLOSEFACTOR);
	float *coeff_end = coeffs + coeffslength;
	int32_t pos = (int32_t)(fir[0] + NSEEL_CLOSEFACTOR);
	float *dline = &fir[2];
	float *buf_val = dline + pos;
	*buf_val = *x;
	float y = 0.0f;
	while (buf_val >= dline)
		y += *buf_val-- * *coeff++;
	buf_val = dline + coeffslength - 1;
	while (coeff < coeff_end)
		y += *buf_val-- * *coeff++;
	if (++pos >= coeffslength)
		fir[0] = (float)0;
	else
		fir[0] = (float)pos;
	return y;
}
#define NRAND 624
#define MRAND 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */
float NSEEL_CGEN_CALL nseel_int_rand(float amplitude)
{
	uint32_t y;
	static uint32_t mag01[2] = { 0x0UL, MATRIX_A };
	/* mag01[x] = x * MATRIX_A  for x=0,1 */
	static uint32_t mt[NRAND]; /* the array for the state vector  */
	static uint32_t __idx;
	uint32_t mti = __idx;
	if (!mti)
	{
		uint32_t s = 0x4141f00d;
		mt[0] = s & 0xffffffffUL;
		for (mti = 1; mti < NRAND; mti++)
		{
			mt[mti] =
				(1812433253UL * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);
			/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
			/* In the previous versions, MSBs of the seed affect   */
			/* only MSBs of the array mt[].                        */
			/* 2002/01/09 modified by Makoto Matsumoto             */
			mt[mti] &= 0xffffffffUL;
			/* for >32 bit machines */
		}
		__idx = NRAND; // mti = N (from loop)
	}
	if (mti >= NRAND) { /* generate N words at one time */
		int32_t kk;
		__idx = 1;

		for (kk = 0; kk < NRAND - MRAND; kk++) {
			y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
			mt[kk] = mt[kk + MRAND] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		for (; kk < NRAND - 1; kk++) {
			y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
			mt[kk] = mt[kk + (MRAND - NRAND)] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		y = (mt[NRAND - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
		mt[NRAND - 1] = mt[MRAND - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];

		mti = 0;
	}
	else
		__idx++;
	y = mt[mti];
	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);
	return (float)(y*(1.0f / (float)0xFFFFFFFF) * fabsf(amplitude));
}
// Calculate pseudo-random 32 bit number based on linear congruential method.
static unsigned long GenerateRandomNumber(void)
{
	static unsigned long randSeed = 22222;  // Change this for different random sequences.
	randSeed = (randSeed * 196314165) + 907633515;
	return randSeed;
}
#define PINK_MAX_RANDOM_ROWS 31
#define PINK_RANDOM_BITS 24
#define PINK_RANDOM_SHIFT ((sizeof(long)*8)-PINK_RANDOM_BITS)
typedef struct
{
	long      pink_Rows[PINK_MAX_RANDOM_ROWS];
	long      pink_RunningSum;   // Used to optimize summing of generators
	int       pink_Index;        // Incremented each sample
	int       pink_IndexMask;    // Index wrapped by ANDing with this mask
	float     pink_Scalar;       // Used to scale within range of -1.0f to +1.0f
} PinkNoise;
// EEL_SETUP PinkNoise structure for N rows of generators
void InitializePinkNoise(PinkNoise *pink, unsigned int numRows)
{
	unsigned int i;
	long pmax;
	pink->pink_Index = 0;
	pink->pink_IndexMask = (1 << numRows) - 1;
	// Calculate maximum possible signed random value. Extra 1 for white noise always added
	pmax = (numRows + 1) * (1 << (PINK_RANDOM_BITS - 1));
	pink->pink_Scalar = 1.0f / pmax;
	// Initialize rows
	for (i = 0; i < numRows; i++) pink->pink_Rows[i] = 0;
	pink->pink_RunningSum = 0;
}
// Generate Pink noise values between -1.0f and +1.0f
float GeneratePinkNoise(PinkNoise *pink)
{
	long newRandom;
	// Increment and mask index
	pink->pink_Index = (pink->pink_Index + 1) & pink->pink_IndexMask;
	// If index is zero, don't update any random values
	if (pink->pink_Index)
	{
		// Determine how many trailing zeros in PinkIndex
		// This algorithm will hang if n==0 so test first
		int numZeros = 0;
		int n = pink->pink_Index;
		while ((n & 1) == 0)
		{
			n = n >> 1;
			numZeros++;
		}
		// Replace the indexed ROWS random value.
		// Subtract and add back to RunningSum instead of adding all the random
		// values together. Only one changes each time.
		pink->pink_RunningSum -= pink->pink_Rows[numZeros];
		newRandom = ((long)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
		pink->pink_RunningSum += newRandom;
		pink->pink_Rows[numZeros] = newRandom;
	}
	// Add extra white noise value
	newRandom = ((long)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
	return pink->pink_Scalar * (float)(pink->pink_RunningSum + newRandom);
}
static float NSEEL_CGEN_CALL pinkNoiseInit(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param < 2)
		return 0;
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs1 = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int pinkness = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	if (pinkness < 1)
		pinkness = 1;
	if (pinkness > 30)
		pinkness = 30;
	memset(indexer, 0, sizeof(PinkNoise));
	InitializePinkNoise(((PinkNoise*)indexer), pinkness);
	return sizeof(PinkNoise) / sizeof(float) + 1;
}
static float NSEEL_CGEN_CALL pinkNoiseGen(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs1 = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	PinkNoise *noiseStruct = (PinkNoise*)indexer;
	return GeneratePinkNoise(noiseStruct) * *parms[1];
}
#include "numericSys/FilterDesign/polyphaseFilterbank.h"
static float NSEEL_CGEN_CALL PolyphaseFilterbankInit(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param < 4)
		return 0;
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	float fs = *parms[0];
	unsigned int N = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	if (N < 2)
	{
		*parms[1] = 2;
		N = 2;
		char badStr[128];
		stbsp_snprintf(badStr, 128, "Number of subbands cannot be less than 2\n");
		EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
	}
	unsigned int m = (uint32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	size_t requiredMemSize = getMemSizeWarpedPFB(N, m);
	size_t aligned = requiredMemSize / sizeof(float) + 1;
	int32_t offs1 = (int32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int32_t offs2 = 0;
	float *indexer2 = 0;
	if (num_param == 5)
	{
		indexer2 = __NSEEL_RAMAlloc(blocks, offs1 + aligned);
		*parms[4] = offs1 + aligned;
	}
	WarpedPFB *pfbPtr = (WarpedPFB*)indexer;
	WarpedPFB *pfbPtr2 = (WarpedPFB*)indexer2;
	initWarpedPFB(pfbPtr, fs, N, m);
	if (pfbPtr2)
		assignPtrWarpedPFB(pfbPtr2, N, m);
	return (float)requiredMemSize;
}
static float NSEEL_CGEN_CALL PolyphaseFilterbankChangeWarpingFactor(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param < 3)
		return 0;
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs1 = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	float fs = *parms[1];
	float warpFact = *parms[2];
	WarpedPFB *pfbPtr = (WarpedPFB*)indexer;
	changeWarpingFactorWarpedPFB(pfbPtr, fs, warpFact);
	return 0;
}
static float NSEEL_CGEN_CALL PolyphaseFilterbankGetPhaseCorrector(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param < 3)
		return 0;
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs1 = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	float corrFact = *parms[1];
	int32_t offs2 = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *phaseCorr = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	WarpedPFB *pfbPtr = (WarpedPFB*)indexer;
	unsigned int CFiltLen;
	float *filter = getPhaseCorrFilterWarpedPFB(pfbPtr, corrFact, &CFiltLen);
	for (int32_t i = 0; i < CFiltLen; i++)
		phaseCorr[i] = filter[i];
	free(filter);
	return CFiltLen;
}
static float NSEEL_CGEN_CALL PolyphaseFilterbankGetDecimationFactor(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param < 2)
		return 0;
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs1 = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int32_t offs2 = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	float *decF = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	WarpedPFB *pfbPtr = (WarpedPFB*)indexer;
	for (int32_t i = 0; i < pfbPtr->N; i++)
		decF[i] = pfbPtr->Sk[i];
	return 0;
}
static float NSEEL_CGEN_CALL PolyphaseFilterbankAnalysisMono(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param < 4)
		return 0;
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs1 = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int32_t offs2 = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	float *subbandDat = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	int32_t offs3 = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *decimationCounter = __NSEEL_RAMAlloc(blocks, (uint64_t)offs3);
	float xn = *parms[3];
	WarpedPFB *pfbPtr = (WarpedPFB*)indexer;
	analysisWarpedPFB(pfbPtr, xn);
	getSubbandDatWarpedPFB(pfbPtr, subbandDat, decimationCounter);
	return 0;
}
static float NSEEL_CGEN_CALL PolyphaseFilterbankSynthesisMono(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param < 2)
		return 0;
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs1 = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int32_t offs2 = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	float *subbandDat = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	WarpedPFB *pfbPtr = (WarpedPFB*)indexer;
	writeSubbandDatWarpedPFB(pfbPtr, subbandDat);
	float y = synthesisWarpedPFB(pfbPtr);
	return y;
}
static float NSEEL_CGEN_CALL PolyphaseFilterbankAnalysisStereo(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param < 7)
		return -1;
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs1 = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer1 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int32_t offs2 = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	float *indexer2 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	int32_t offs3 = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *subbandDat1 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs3);
	int32_t offs4 = (int32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	float *subbandDat2 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs4);
	int32_t offs5 = (int32_t)(*parms[4] + NSEEL_CLOSEFACTOR);
	float *decimationCounter = __NSEEL_RAMAlloc(blocks, (uint64_t)offs5);
	WarpedPFB *pfb1Ptr = (WarpedPFB*)indexer1;
	WarpedPFB *pfb2Ptr = (WarpedPFB*)indexer2;
	analysisWarpedPFBStereo(pfb1Ptr, pfb2Ptr, parms[5], parms[6]);
	getSubbandDatWarpedPFBStereo(pfb1Ptr, pfb2Ptr, subbandDat1, subbandDat2, decimationCounter);
	return 0;
}
static float NSEEL_CGEN_CALL PolyphaseFilterbankSynthesisStereo(void *opaque, INT_PTR num_param, float **parms)
{
	if (num_param < 6)
		return -1;
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t offs1 = (int32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float *indexer1 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	int32_t offs2 = (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	float *indexer2 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	int32_t offs3 = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float *subbandDat1 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs3);
	int32_t offs4 = (int32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	float *subbandDat2 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs4);
	WarpedPFB *pfb1Ptr = (WarpedPFB*)indexer1;
	WarpedPFB *pfb2Ptr = (WarpedPFB*)indexer2;
	writeSubbandDatWarpedPFBStereo(pfb1Ptr, pfb2Ptr, subbandDat1, subbandDat2);
	synthesisWarpedPFBStereo(pfb1Ptr, pfb2Ptr, parms[4], parms[5]);
	return 0;
}
#include "numericSys/FilterDesign/fdesign.h"
static float NSEEL_CGEN_CALL _eel_iirBandSplitterInit(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	uint32_t offs1 = (uint32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	float fs = *parms[1];
	float *tdsbStruct = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
	if (num_param > 9)
		return -1;
	size_t requireMemSize;
	void *ptr;
	if (num_param == 3)
	{
		requireMemSize = sizeof(LinkwitzRileyCrossover);
		LinkwitzRileyCrossover *lr = (LinkwitzRileyCrossover*)malloc(requireMemSize);
		LWZRClearStateVariable(lr);
		LWZRCalculateCoefficients(lr, fs, *parms[2], 0);
		ptr = lr;
	}
	else if (num_param == 4)
	{
		requireMemSize = sizeof(ThreeBandsCrossover);
		ThreeBandsCrossover *bps = (ThreeBandsCrossover*)malloc(requireMemSize);
		memset(bps, 0, sizeof(ThreeBandsCrossover));
		init3BandsCrossover(bps, fs, *parms[2], *parms[3]);
		ptr = bps;
	}
	else if (num_param == 5)
	{
		requireMemSize = sizeof(FourBandsCrossover);
		FourBandsCrossover *bps = (FourBandsCrossover*)malloc(requireMemSize);
		memset(bps, 0, sizeof(FourBandsCrossover));
		init4BandsCrossover(bps, fs, *parms[2], *parms[3], *parms[4]);
		ptr = bps;
	}
	else if (num_param == 6)
	{
		requireMemSize = sizeof(FiveBandsCrossover);
		FiveBandsCrossover *bps = (FiveBandsCrossover*)malloc(requireMemSize);
		memset(bps, 0, sizeof(FiveBandsCrossover));
		init5BandsCrossover(bps, fs, *parms[2], *parms[3], *parms[4], *parms[5]);
		ptr = bps;
	}
	else if (num_param == 7)
	{
		requireMemSize = sizeof(SixBandsCrossover);
		SixBandsCrossover *bps = (SixBandsCrossover*)malloc(requireMemSize);
		memset(bps, 0, sizeof(SixBandsCrossover));
		init6BandsCrossover(bps, fs, *parms[2], *parms[3], *parms[4], *parms[5], *parms[6]);
		ptr = bps;
	}
	else if (num_param == 8)
	{
		requireMemSize = sizeof(SevenBandsCrossover);
		SevenBandsCrossover *bps = (SevenBandsCrossover*)malloc(requireMemSize);
		memset(bps, 0, sizeof(SevenBandsCrossover));
		init7BandsCrossover(bps, fs, *parms[2], *parms[3], *parms[4], *parms[5], *parms[6], *parms[7]);
		ptr = bps;
	}
	else
	{
		requireMemSize = sizeof(EightBandsCrossover);
		EightBandsCrossover *bps = (EightBandsCrossover*)malloc(requireMemSize);
		memset(bps, 0, sizeof(EightBandsCrossover));
		init8BandsCrossover(bps, fs, *parms[2], *parms[3], *parms[4], *parms[5], *parms[6], *parms[7], *parms[8]);
		ptr = bps;
	}
	tdsbStruct[0] = (float)(num_param - 1);
	memcpy(tdsbStruct + 1, ptr, requireMemSize);
	free(ptr);
	return (float)(1 + requireMemSize / sizeof(float));
}
static float NSEEL_CGEN_CALL _eel_iirBandSplitterClearState(float *blocks, float *start)
{
	float *tdsbStruct = __NSEEL_RAMAlloc(blocks, (uint64_t)(uint32_t)(*start + NSEEL_CLOSEFACTOR));
	int32_t bands = (int32_t)(tdsbStruct[0] + NSEEL_CLOSEFACTOR);
	switch (bands)
	{
	case 2:
		LWZRClearStateVariable((LinkwitzRileyCrossover*)(tdsbStruct + 1));
		break;
	case 3:
		clearState3BandsCrossover((ThreeBandsCrossover*)(tdsbStruct + 1));
		break;
	case 4:
		clearState4BandsCrossover((FourBandsCrossover*)(tdsbStruct + 1));
		break;
	case 5:
		clearState5BandsCrossover((FiveBandsCrossover*)(tdsbStruct + 1));
		break;
	case 6:
		clearState6BandsCrossover((SixBandsCrossover*)(tdsbStruct + 1));
		break;
	case 7:
		clearState7BandsCrossover((SevenBandsCrossover*)(tdsbStruct + 1));
		break;
	case 8:
		clearState8BandsCrossover((EightBandsCrossover*)(tdsbStruct + 1));
		break;
	}
	return 1;
}
static float NSEEL_CGEN_CALL _eel_iirBandSplitterProcess(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *tdsbStruct = __NSEEL_RAMAlloc(c->ram_state, (uint32_t)(*parms[0] + NSEEL_CLOSEFACTOR));
	int32_t bands = (int32_t)(tdsbStruct[0] + NSEEL_CLOSEFACTOR);
	double out[8];
	switch (bands)
	{
	case 2:
		LWZRProcessSample((LinkwitzRileyCrossover*)(tdsbStruct + 1), *parms[1], &out[0], &out[1]);
		*parms[2] = (float)out[0];
		*parms[3] = (float)-out[1];
		break;
	case 3:
		process3BandsCrossover((ThreeBandsCrossover*)(tdsbStruct + 1), *parms[1], &out[0], &out[1], &out[2]);
		*parms[2] = (float)out[0];
		*parms[3] = (float)-out[1];
		*parms[4] = (float)out[2];
		break;
	case 4:
		process4BandsCrossover((FourBandsCrossover*)(tdsbStruct + 1), *parms[1], &out[0], &out[1], &out[2], &out[3]);
		*parms[2] = (float)out[0];
		*parms[3] = (float)-out[1];
		*parms[4] = (float)out[2];
		*parms[5] = (float)-out[3];
		break;
	case 5:
		process5BandsCrossover((FiveBandsCrossover*)(tdsbStruct + 1), *parms[1], &out[0], &out[1], &out[2], &out[3], &out[4]);
		*parms[2] = (float)out[0];
		*parms[3] = (float)-out[1];
		*parms[4] = (float)out[2];
		*parms[5] = (float)-out[3];
		*parms[6] = (float)out[4];
		break;
	case 6:
		process6BandsCrossover((SixBandsCrossover*)(tdsbStruct + 1), *parms[1], &out[0], &out[1], &out[2], &out[3], &out[4], &out[5]);
		*parms[2] = (float)out[0];
		*parms[3] = (float)-out[1];
		*parms[4] = (float)out[2];
		*parms[5] = (float)-out[3];
		*parms[6] = (float)out[4];
		*parms[7] = (float)-out[5];
		break;
	case 7:
		process7BandsCrossover((SevenBandsCrossover*)(tdsbStruct + 1), *parms[1], &out[0], &out[1], &out[2], &out[3], &out[4], &out[5], &out[6]);
		*parms[2] = (float)out[0];
		*parms[3] = (float)-out[1];
		*parms[4] = (float)out[2];
		*parms[5] = (float)-out[3];
		*parms[6] = (float)out[4];
		*parms[7] = (float)-out[5];
		*parms[8] = (float)out[6];
		break;
	case 8:
		process8BandsCrossover((EightBandsCrossover*)(tdsbStruct + 1), *parms[1], &out[0], &out[1], &out[2], &out[3], &out[4], &out[5], &out[6], &out[7]);
		*parms[2] = (float)out[0];
		*parms[3] = (float)-out[1];
		*parms[4] = (float)out[2];
		*parms[5] = (float)-out[3];
		*parms[6] = (float)out[4];
		*parms[7] = (float)-out[5];
		*parms[8] = (float)out[6];
		*parms[9] = (float)-out[7];
		break;
	}
	return 1;
}
static float NSEEL_CGEN_CALL _eel_initfftconv1d(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	void *ptr;
	char convType;
	if (num_param == 3)
		convType = 1;
	else if (num_param == 4)
		convType = 2;
	else if (num_param == 6)
		convType = 3;
	else
		return -2;
	uint32_t latency = (uint32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	uint32_t irLen = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	if (convType == 1)
	{
		int32_t offs1 = (uint32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
		float *impulseresponse = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
		FFTConvolver1x1 *conv = (FFTConvolver1x1*)malloc(sizeof(FFTConvolver1x1));
		FFTConvolver1x1Init(conv);
		FFTConvolver1x1LoadImpulseResponse(conv, latency, impulseresponse, irLen);
		ptr = (void*)conv;
	}
	else if (convType == 2)
	{
		uint32_t offs1 = (uint32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
		uint32_t offs2 = (uint32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
		float *leftImp = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
		float *rightImp = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
		FFTConvolver2x2 *conv = (FFTConvolver2x2*)malloc(sizeof(FFTConvolver2x2));
		FFTConvolver2x2Init(conv);
		FFTConvolver2x2LoadImpulseResponse(conv, latency, leftImp, rightImp, irLen);
		ptr = (void*)conv;
	}
	else
	{
		uint32_t offs1 = (uint32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
		uint32_t offs2 = (uint32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
		uint32_t offs3 = (uint32_t)(*parms[4] + NSEEL_CLOSEFACTOR);
		uint32_t offs4 = (uint32_t)(*parms[5] + NSEEL_CLOSEFACTOR);
		float *LL = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
		float *LR = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
		float *RL = __NSEEL_RAMAlloc(blocks, (uint64_t)offs3);
		float *RR = __NSEEL_RAMAlloc(blocks, (uint64_t)offs4);
		FFTConvolver2x4x2 *conv = (FFTConvolver2x4x2*)malloc(sizeof(FFTConvolver2x4x2));
		FFTConvolver2x4x2Init(conv);
		FFTConvolver2x4x2LoadImpulseResponse(conv, latency, LL, LR, RL, RR, irLen);
		ptr = (void*)conv;
	}
	int32_t id = AddData(c->region_context, ptr, convType);
	return (float)id;
}
static float NSEEL_CGEN_CALL _eel_processfftconv1d(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	int32_t idx = (uint32_t)(*parms[0] + NSEEL_CLOSEFACTOR);
	void *ptr = c->region_context->memRegion[idx];
	char convType = c->region_context->type[idx];
	if (convType == 1)
	{
		FFTConvolver1x1 *conv = (FFTConvolver1x1*)ptr;
		uint32_t offs1 = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
		float *x = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
		FFTConvolver1x1Process(conv, x, x, conv->_blockSize);
	}
	if (convType == 2)
	{
		FFTConvolver2x2 *conv = (FFTConvolver2x2*)ptr;
		uint32_t offs1 = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
		uint32_t offs2 = (uint32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
		float *x1 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
		float *x2 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
		FFTConvolver2x2Process(conv, x1, x2, x1, x2, conv->_blockSize);
	}
	if (convType == 3)
	{
		FFTConvolver2x4x2 *conv = (FFTConvolver2x4x2*)ptr;
		uint32_t offs1 = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
		uint32_t offs2 = (uint32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
		float *x1 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs1);
		float *x2 = __NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
		FFTConvolver2x4x2Process(conv, x1, x2, x1, x2, conv->_blockSize);
	}
	return 1;
}
void *task_user_created(void *arg)
{
	abstractThreads *info = (abstractThreads *)arg;
	// cond_wait mutex must be locked before we can wait
	pthread_mutex_lock(&(info->work_mtx));
	// ensure boss is waiting
	pthread_mutex_lock(&(info->boss_mtx));
	// signal to boss that EEL_SETUP is complete
	info->state = EEL_IDLE;
	// wake-up signal
	pthread_cond_signal(&(info->boss_cond));
	pthread_mutex_unlock(&(info->boss_mtx));
	while (1)
	{
		pthread_cond_wait(&(info->work_cond), &(info->work_mtx));
		if (EEL_GET_OFF_FROM_WORK == info->state)
			break; // kill thread
		if (EEL_IDLE == info->state)
			continue; // accidental wake-up
		// do blocking task
		NSEEL_code_execute(info->codePtr);
		// ensure boss is waiting
		pthread_mutex_lock(&(info->boss_mtx));
		// indicate that job is done
		info->state = EEL_IDLE;
		// wake-up signal
		pthread_cond_signal(&(info->boss_cond));
		pthread_mutex_unlock(&(info->boss_mtx));
	}
	pthread_mutex_unlock(&(info->work_mtx));
	pthread_exit(NULL);
	return 0;
}
void thread_init(abstractThreads *info)
{
	info->state = EEL_SETUP;
	pthread_cond_init(&(info->work_cond), NULL);
	pthread_mutex_init(&(info->work_mtx), NULL);
	pthread_cond_init(&(info->boss_cond), NULL);
	pthread_mutex_init(&(info->boss_mtx), NULL);
	pthread_mutex_lock(&(info->boss_mtx));
	pthread_create(&info->threadID, NULL, task_user_created, (void *)info);
	eelThread_wait(info);
}
static float NSEEL_CGEN_CALL _eel_initthread(void *opaque, float *stringID)
{
	compileContext *c = (compileContext*)opaque;
	const char *codePtr = (const char*)GetStringForIndex(c->region_context, *stringID, 0);
	NSEEL_CODEHANDLE compiledCode = NSEEL_code_compile_ex(c, codePtr, 0, 1);
	char *err;
	char badStr[128];
	if (!compiledCode && (err = NSEEL_code_getcodeerror(c)))
	{
		stbsp_snprintf(badStr, 128, "Error when compiling code for thread: %s\n", err);
		EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
		return -2;
	}
	abstractThreads *pth = (abstractThreads*)malloc(sizeof(abstractThreads));
	pth->codePtr = compiledCode;
	thread_init(pth);
	int32_t id = AddData(c->region_context, (void *)pth, 4);
	return (float)id;
}
static float NSEEL_CGEN_CALL _eel_deleteSysVariable(void *opaque, float *v)
{
	compileContext *c = (compileContext*)opaque;
	DeleteSlot(c->region_context, (int32_t)(*v + NSEEL_CLOSEFACTOR));
	return 1;
}
static float NSEEL_CGEN_CALL _eel_createThread(void *opaque, float *v)
{
	compileContext *c = (compileContext*)opaque;
	int32_t idx = (int32_t)(*v + NSEEL_CLOSEFACTOR);
	abstractThreads *ptr = (abstractThreads *)c->region_context->memRegion[idx];
	eelThread_start(ptr);
	return 1;
}
static float NSEEL_CGEN_CALL _eel_joinThread(void *opaque, float *v)
{
	compileContext *c = (compileContext*)opaque;
	int32_t idx = (int32_t)(*v + NSEEL_CLOSEFACTOR);
	abstractThreads *ptr = (abstractThreads *)c->region_context->memRegion[idx];
	eelThread_wait(ptr);
	return 1;
}
static float NSEEL_CGEN_CALL _eel_lockThread(void *opaque, float *v)
{
	compileContext *c = (compileContext*)opaque;
	pthread_mutex_lock(&c->globalLocker);
	return 1;
}
static float NSEEL_CGEN_CALL _eel_unlockThread(void *opaque, float *v)
{
	compileContext *c = (compileContext*)opaque;
	pthread_mutex_unlock(&c->globalLocker);
	return 1;
}
// HPFloat
#define HPFREPEATBLKEND \
void *ptr = malloc(sizeof(xpr)); \
memcpy(ptr, (void *)&br, sizeof(xpr)); \
int32_t id = AddData(c->region_context, ptr, 5); \
return id;
static float NSEEL_CGEN_CALL _eel_createHPFloatFromString(void *opaque, float *v)
{
	compileContext *c = (compileContext *)opaque;
	const char *decimal = (const char *)GetStringForIndex(c->region_context, *v, 0);
	xpr br = atox(decimal);
	HPFREPEATBLKEND
}
static float NSEEL_CGEN_CALL _eel_createHPFloatFromFloat32(void *opaque, float *flt)
{
	compileContext *c = (compileContext *)opaque;
	xpr br = flttox(*flt);
	void *ptr = malloc(sizeof(xpr));
	memcpy(ptr, (void *)&br, sizeof(xpr));
	int32_t id = AddData(c->region_context, ptr, 5);
	return id;
}
static float NSEEL_CGEN_CALL _eel_createHPFloatToFloat32(void *opaque, float *v)
{
	compileContext *c = (compileContext *)opaque;
	int32_t idx = (int32_t)(*v + NSEEL_CLOSEFACTOR);
	xpr *br = (xpr *)c->region_context->memRegion[idx];
	return xtoflt(*br);
}
static float NSEEL_CGEN_CALL _eel_createHPFloatToString(void *opaque, float *v)
{
	compileContext *c = (compileContext *)opaque;
	int32_t idx = (int32_t)(*v + NSEEL_CLOSEFACTOR);
	xpr *br = (xpr *)c->region_context->memRegion[idx];
	char *str = xpr_asprint(*br, 1, 0, (XDIM * 48) / 10 - 2);
	int32_t id = AddData(c->region_context, (void*)str, 0);
	return id;
}
#define HPFREPEATBLKDYADIC \
compileContext *c = (compileContext *)opaque; \
xpr *x1 = (xpr *)c->region_context->memRegion[(int32_t)(*parms[0] + NSEEL_CLOSEFACTOR)]; \
xpr *x2 = (xpr *)c->region_context->memRegion[(int32_t)(*parms[1] + NSEEL_CLOSEFACTOR)];
#define HPFREPEATBLK1O \
compileContext *c = (compileContext *)opaque; \
xpr *x = (xpr *)c->region_context->memRegion[(int32_t)(*parms[0] + NSEEL_CLOSEFACTOR)];
static float NSEEL_CGEN_CALL _eel_HPFAdd(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLKDYADIC
	xpr br = xadd(*x1, *x2, 0);
	HPFREPEATBLKEND
}
static float NSEEL_CGEN_CALL _eel_HPFSub(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLKDYADIC
	xpr br = xadd(*x1, *x2, 1);
	HPFREPEATBLKEND
}
static float NSEEL_CGEN_CALL _eel_HPFMul(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLKDYADIC
	xpr br = xmul(*x1, *x2);
	HPFREPEATBLKEND
}
static float NSEEL_CGEN_CALL _eel_HPFDiv(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLKDYADIC
	xpr br = xdiv(*x1, *x2);
	HPFREPEATBLKEND
}
// Math functions
static float NSEEL_CGEN_CALL _eel_HPFfrexp(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLK1O
	float *blocks = c->ram_state;
	int p;
	xpr br = xfrexp(*x, &p);
	uint32_t offs2 = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	float *out = (float *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	void *ptr = malloc(sizeof(xpr));
	memcpy(ptr, (void *)&br, sizeof(xpr));
	out[0] = AddData(c->region_context, ptr, 5);
	ptr = malloc(sizeof(xpr));
	br = dbltox((double)p);
	memcpy(ptr, (void *)&br, sizeof(xpr));
	out[1] = AddData(c->region_context, ptr, 5);
	return 0;
}
static float NSEEL_CGEN_CALL _eel_HPFqfmod(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext *)opaque;
	xpr *s = (xpr *)c->region_context->memRegion[(int32_t)(*parms[0] + NSEEL_CLOSEFACTOR)];
	xpr *t = (xpr *)c->region_context->memRegion[(int32_t)(*parms[1] + NSEEL_CLOSEFACTOR)];
	xpr *q = (xpr *)c->region_context->memRegion[(int32_t)(*parms[2] + NSEEL_CLOSEFACTOR)];
	xpr br = xfmod(*s, *t, q);
	HPFREPEATBLKEND
}
static float NSEEL_CGEN_CALL _eel_HPFfmod(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLKDYADIC
	xpr q;
	xpr br = xfmod(*x1, *x2, &q);
	HPFREPEATBLKEND
}
static float NSEEL_CGEN_CALL _eel_HPFsfmod(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLK1O
	float *blocks = c->ram_state;
	int p;
	xpr br = xsfmod(*x, &p);
	uint32_t offs2 = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	float *out = (float *)__NSEEL_RAMAlloc(blocks, (uint64_t)offs2);
	void *ptr = malloc(sizeof(xpr));
	memcpy(ptr, (void *)&br, sizeof(xpr));
	out[0] = AddData(c->region_context, ptr, 5);
	ptr = malloc(sizeof(xpr));
	br = dbltox((double)p);
	memcpy(ptr, (void *)&br, sizeof(xpr));
	out[1] = AddData(c->region_context, ptr, 5);
	return 0;
}
static float NSEEL_CGEN_CALL _eel_HPFMulPowOf2(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLK1O
	xpr br = xpr2(*x, (int32_t)(*parms[1] + NSEEL_CLOSEFACTOR));
	HPFREPEATBLKEND
}
#define HPFLogicGen1O(fname, expr) \
static float NSEEL_CGEN_CALL fname(void *opaque, INT_PTR num_param, float **parms) \
{ \
	HPFREPEATBLK1O \
	int lg = expr; \
	return lg; \
}
#define HPFLogicGenDYADIC(fname, expr) \
static float NSEEL_CGEN_CALL fname(void *opaque, INT_PTR num_param, float **parms) \
{ \
	HPFREPEATBLKDYADIC \
	int lg = expr; \
	return lg; \
}
#define HPFGen(fname, expr) \
static float NSEEL_CGEN_CALL fname(void *opaque, INT_PTR num_param, float **parms) \
{ \
	HPFREPEATBLK1O \
	xpr br = expr(*x); \
	HPFREPEATBLKEND \
}
HPFGen(_eel_HPFfrac, xfrac)
HPFGen(_eel_HPFabs, xabs)
HPFGen(_eel_HPFtrunc, xtrunc)
HPFGen(_eel_HPFround, xround)
HPFGen(_eel_HPFceil, xceil)
HPFGen(_eel_HPFfloor, xfloor)
HPFGen(_eel_HPFfix, xfix)
HPFGen(_eel_HPFtan, xtan)
HPFGen(_eel_HPFsin, xsin)
HPFGen(_eel_HPFcos, xcos)
HPFGen(_eel_HPFatan, xatan)
HPFGen(_eel_HPFasin, xasin)
HPFGen(_eel_HPFacos, xacos)
HPFGen(_eel_HPFNegation, xneg)
static float NSEEL_CGEN_CALL _eel_HPFatan2(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLKDYADIC
	xpr br = xatan2(*x1, *x2);
	HPFREPEATBLKEND
}
HPFGen(_eel_HPFsqrt, xsqrt)
HPFGen(_eel_HPFexp, xexp)
HPFGen(_eel_HPFexp2, xexp2)
HPFGen(_eel_HPFexp10, xexp10)
HPFGen(_eel_HPFlog, xlog)
HPFGen(_eel_HPFlog2, xlog2)
HPFGen(_eel_HPFlog10, xlog10)
HPFGen(_eel_HPFtanh, xtanh)
HPFGen(_eel_HPFsinh, xsinh)
HPFGen(_eel_HPFcosh, xcosh)
HPFGen(_eel_HPFatanh, xatanh)
HPFGen(_eel_HPFasinh, xasinh)
HPFGen(_eel_HPFacosh, xacosh)
HPFGen(_eel_HPFclone, )
static float NSEEL_CGEN_CALL _eel_HPFpow(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLKDYADIC
	xpr br = xpow(*x1, *x2);
	HPFREPEATBLKEND
}
static float NSEEL_CGEN_CALL _eel_HPFIntPow(void *opaque, INT_PTR num_param, float **parms)
{
	HPFREPEATBLK1O
	xpr br = xpwr(*x, (int32_t)roundf(*parms[1]));
	HPFREPEATBLKEND
}
HPFLogicGen1O(_eel_HPFiszero, xis0(x))
HPFLogicGen1O(_eel_HPFisneg, x_neg(x))
HPFLogicGen1O(_eel_HPFbinaryexp, x_exp(x))
HPFLogicGenDYADIC(_eel_HPFeq, (xprcmp(x1, x2) == 0))
HPFLogicGenDYADIC(_eel_HPFneq, (xprcmp(x1, x2) != 0))
HPFLogicGenDYADIC(_eel_HPFle, (xprcmp(x1, x2) <= 0))
HPFLogicGenDYADIC(_eel_HPFge, (xprcmp(x1, x2) >= 0))
HPFLogicGenDYADIC(_eel_HPFgt, (xprcmp(x1, x2) > 0))
HPFLogicGenDYADIC(_eel_HPFlt, (xprcmp(x1, x2) < 0))
static void channel_split(float *buffer, uint64_t num_frames, float **chan_buffers, uint32_t num_channels)
{
	uint64_t i, samples = num_frames * num_channels;
	for (i = 0; i < samples; i++)
		chan_buffers[i % num_channels][i / num_channels] = buffer[i];
}
static void channel_join(float **chan_buffers, uint32_t num_channels, float *buffer, uint64_t num_frames)
{
	uint64_t i, samples = num_frames * num_channels;
	for (i = 0; i < samples; i++)
		buffer[i] = (float)(chan_buffers[i % num_channels][i / num_channels]);
}
#include "numericSys/libsamplerate/samplerate.h"
void JamesDSPOfflineResampling(float const *in, float *out, size_t lenIn, size_t lenOut, int channels, double src_ratio)
{
	if (lenOut == lenIn && lenIn == 1)
	{
		memcpy(out, in, channels * sizeof(float));
		return;
	}
	SRC_DATA src_data;
	memset(&src_data, 0, sizeof(src_data));
	src_data.data_in = in;
	src_data.data_out = out;
	src_data.input_frames = (long)lenIn;
	src_data.output_frames = (long)lenOut;
	src_data.src_ratio = src_ratio;
	int error;
	if ((error = src_simple(&src_data, 0, channels)))
	{
		printf("\n%s\n\n", src_strerror(error));
	}
}
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
static float NSEEL_CGEN_CALL _eel_flacDecodeFile(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	const char *filename = (const char*)GetStringForIndex(c->region_context, *parms[0], 0);
	uint32_t channels, fs;
	uint64_t frameCount;
	float *signal = drflac_open_file_and_read_pcm_frames_f32(filename, &channels, &fs, &frameCount, 0);
	float targetFs = *parms[2];
	if (targetFs > FLT_EPSILON)
	{
		double ratio = targetFs / (double)fs;
		if (ratio != 1.0)
		{
			int compressedLen = (int)ceil(frameCount * ratio);
			float *tmpBuf = (float*)malloc(compressedLen * channels * sizeof(float));
			memset(tmpBuf, 0, compressedLen * channels * sizeof(float));
			JamesDSPOfflineResampling(signal, tmpBuf, frameCount, compressedLen, channels, ratio);
			frameCount = compressedLen;
			free(signal);
			signal = tmpBuf;
		}
	}
	else
		*parms[2] = (float)fs;
	uint32_t channel1BasePointer = (uint32_t)(*parms[4] + NSEEL_CLOSEFACTOR);
	frameCount = (frameCount + channel1BasePointer) > NSEEL_RAM_ITEMSPERBLOCK ? (NSEEL_RAM_ITEMSPERBLOCK - channel1BasePointer) : frameCount;
	*parms[1] = (float)channels;
	*parms[3] = (float)frameCount;
	if ((num_param - 4) < (int32_t)channels)
	{
		free(signal);
		return -1;
	}
	float **ptr = (float**)malloc(channels * sizeof(float*));
	ptr[0] = __NSEEL_RAMAlloc(blocks, (uint64_t)channel1BasePointer);
	uint64_t pointer = channel1BasePointer;
	uint64_t count = pointer;
	int resetCnt = 0;
	for (uint32_t i = 1; i < channels; i++)
	{
		if (count + frameCount * 2 < NSEEL_RAM_ITEMSPERBLOCK)
		{
			pointer += frameCount;
			count = pointer;
			resetCnt++;
		}
		else
		{
			pointer = NSEEL_RAM_ITEMSPERBLOCK * resetCnt;
			count = 0;
		}
		*parms[i + 4] = (float)pointer;
		ptr[i] = __NSEEL_RAMAlloc(blocks, (uint64_t)pointer);
	}
	channel_split(signal, frameCount, ptr, channels);
	free(signal);
	free(ptr);
	return 1;
}
static float NSEEL_CGEN_CALL _eel_flacDecodeMemory(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	const char *base64String = (const char*)GetStringForIndex(c->region_context, *parms[0], 0);
	size_t actualSize;
	unsigned char *memoryBlk = base64_decode((const unsigned char*)base64String, strlen(base64String), &actualSize);
	uint32_t channels, fs;
	uint64_t frameCount;
	float *signal = drflac_open_memory_and_read_pcm_frames_f32(memoryBlk, actualSize, &channels, &fs, &frameCount, 0);
	float targetFs = *parms[2];
	if (targetFs > FLT_EPSILON)
	{
		double ratio = targetFs / (double)fs;
		if (ratio != 1.0)
		{
			int compressedLen = (int)ceil(frameCount * ratio);
			float *tmpBuf = (float*)malloc(compressedLen * channels * sizeof(float));
			memset(tmpBuf, 0, compressedLen * channels * sizeof(float));
			JamesDSPOfflineResampling(signal, tmpBuf, frameCount, compressedLen, channels, ratio);
			frameCount = compressedLen;
			free(signal);
			signal = tmpBuf;
		}
	}
	else
		*parms[2] = (float)fs;
	free(memoryBlk);
	uint32_t channel1BasePointer = (uint32_t)(*parms[4] + NSEEL_CLOSEFACTOR);
	frameCount = (frameCount + channel1BasePointer) > NSEEL_RAM_ITEMSPERBLOCK ? (NSEEL_RAM_ITEMSPERBLOCK - channel1BasePointer) : frameCount;
	*parms[1] = (float)channels;
	*parms[3] = (float)frameCount;
	if ((num_param - 4) < (int32_t)channels)
	{
		free(signal);
		return -1;
	}
	float **ptr = (float**)malloc(channels * sizeof(float*));
	ptr[0] = __NSEEL_RAMAlloc(blocks, (uint64_t)channel1BasePointer);
	uint64_t pointer = channel1BasePointer;
	uint64_t count = pointer;
	int resetCnt = 0;
	for (uint32_t i = 1; i < channels; i++)
	{
		if (count + frameCount * 2 < NSEEL_RAM_ITEMSPERBLOCK)
		{
			pointer += frameCount;
			count = pointer;
			resetCnt++;
		}
		else
		{
			pointer = NSEEL_RAM_ITEMSPERBLOCK * resetCnt;
			count = 0;
		}
		*parms[i + 4] = (float)pointer;
		ptr[i] = __NSEEL_RAMAlloc(blocks, (uint64_t)pointer);
	}
	channel_split(signal, frameCount, ptr, channels);
	free(signal);
	free(ptr);
	return 1;
}
static float NSEEL_CGEN_CALL _eel_wavDecodeFile(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	const char *filename = (const char*)GetStringForIndex(c->region_context, *parms[0], 0);
	uint32_t channels, fs;
	uint64_t frameCount;
	float *signal = drwav_open_file_and_read_pcm_frames_f32(filename, &channels, &fs, &frameCount, 0);
	float targetFs = *parms[2];
	if (targetFs > FLT_EPSILON)
	{
		double ratio = targetFs / (double)fs;
		if (ratio != 1.0)
		{
			int compressedLen = (int)ceil(frameCount * ratio);
			float *tmpBuf = (float*)malloc(compressedLen * channels * sizeof(float));
			memset(tmpBuf, 0, compressedLen * channels * sizeof(float));
			JamesDSPOfflineResampling(signal, tmpBuf, frameCount, compressedLen, channels, ratio);
			frameCount = compressedLen;
			free(signal);
			signal = tmpBuf;
		}
	}
	else
		*parms[2] = (float)fs;
	uint32_t channel1BasePointer = (uint32_t)(*parms[4] + NSEEL_CLOSEFACTOR);
	frameCount = (frameCount + channel1BasePointer) > NSEEL_RAM_ITEMSPERBLOCK ? (NSEEL_RAM_ITEMSPERBLOCK - channel1BasePointer) : frameCount;
	*parms[1] = (float)channels;
	*parms[3] = (float)frameCount;
	if ((num_param - 4) < (int32_t)channels)
	{
		free(signal);
		return -1;
	}
	float **ptr = (float**)malloc(channels * sizeof(float*));
	ptr[0] = __NSEEL_RAMAlloc(blocks, (uint64_t)channel1BasePointer);
	uint64_t pointer = channel1BasePointer;
	uint64_t count = pointer;
	int resetCnt = 0;
	for (uint32_t i = 1; i < channels; i++)
	{
		if (count + frameCount * 2 < NSEEL_RAM_ITEMSPERBLOCK)
		{
			pointer += frameCount;
			count = pointer;
			resetCnt++;
		}
		else
		{
			pointer = NSEEL_RAM_ITEMSPERBLOCK * resetCnt;
			count = 0;
		}
		*parms[i + 4] = (float)pointer;
		ptr[i] = __NSEEL_RAMAlloc(blocks, (uint64_t)pointer);
	}
	channel_split(signal, frameCount, ptr, channels);
	free(signal);
	free(ptr);
	return 1;
}
static float NSEEL_CGEN_CALL _eel_wavDecodeMemory(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	const char *base64String = (const char*)GetStringForIndex(c->region_context, *parms[0], 0);
	size_t actualSize;
	unsigned char *memoryBlk = base64_decode((const unsigned char*)base64String, strlen(base64String), &actualSize);
	uint32_t channels, fs;
	uint64_t frameCount;
	float *signal = drwav_open_memory_and_read_pcm_frames_f32(memoryBlk, actualSize, &channels, &fs, &frameCount, 0);
	float targetFs = *parms[2];
	if (targetFs > FLT_EPSILON)
	{
		double ratio = targetFs / (double)fs;
		if (ratio != 1.0)
		{
			int compressedLen = (int)ceil(frameCount * ratio);
			float *tmpBuf = (float*)malloc(compressedLen * channels * sizeof(float));
			memset(tmpBuf, 0, compressedLen * channels * sizeof(float));
			JamesDSPOfflineResampling(signal, tmpBuf, frameCount, compressedLen, channels, ratio);
			frameCount = compressedLen;
			free(signal);
			signal = tmpBuf;
		}
	}
	else
		*parms[2] = (float)fs;
	free(memoryBlk);
	uint32_t channel1BasePointer = (uint32_t)(*parms[4] + NSEEL_CLOSEFACTOR);
	frameCount = (frameCount + channel1BasePointer) > NSEEL_RAM_ITEMSPERBLOCK ? (NSEEL_RAM_ITEMSPERBLOCK - channel1BasePointer) : frameCount;
	*parms[1] = (float)channels;
	*parms[3] = (float)frameCount;
	if ((num_param - 4) < (int32_t)channels)
	{
		free(signal);
		return -1;
	}
	float **ptr = (float**)malloc(channels * sizeof(float*));
	ptr[0] = __NSEEL_RAMAlloc(blocks, (uint64_t)channel1BasePointer);
	uint64_t pointer = channel1BasePointer;
	uint64_t count = pointer;
	int resetCnt = 0;
	for (uint32_t i = 1; i < channels; i++)
	{
		if (count + frameCount * 2 < NSEEL_RAM_ITEMSPERBLOCK)
		{
			pointer += frameCount;
			count = pointer;
			resetCnt++;
		}
		else
		{
			pointer = NSEEL_RAM_ITEMSPERBLOCK * resetCnt;
			count = 0;
		}
		*parms[i + 4] = (float)pointer;
		ptr[i] = __NSEEL_RAMAlloc(blocks, (uint64_t)pointer);
	}
	channel_split(signal, frameCount, ptr, channels);
	free(signal);
	free(ptr);
	return 1;
}
unsigned int peakfinder(unsigned int elements, const float *input, float sel, unsigned int extrema, float *peakInds)
{
	unsigned int i, j, k, foundPeak;
	unsigned int eleMinus1 = elements - 1;
	unsigned int eleMinus2 = elements - 2;
	float work, minMag, leftMin, tempMag;
	unsigned int *ind = (unsigned int*)malloc((eleMinus2 < 4 ? 4 : eleMinus2) * sizeof(unsigned int));
	float *b_x = (float*)malloc((eleMinus1 < 4 ? 4 : eleMinus1) * sizeof(float));
	unsigned int *peakLoc = (unsigned int*)malloc((eleMinus2 < 4 ? 4 : eleMinus2) * sizeof(unsigned int));
	// Adjust threshold according to extrema
	i = 1;
	k = 0;
	if (extrema)
	{
		work = input[0];
		for (j = 0; j < eleMinus1; j++)
		{
			minMag = work;
			work = input[i];
			b_x[k] = input[i] - minMag;
			i++;
			k++;
		}
	}
	else
	{
		work = -input[0];
		for (j = 0; j < eleMinus1; j++)
		{
			minMag = work;
			work = -input[i];
			b_x[k] = -input[i] - minMag;
			i++;
			k++;
		}
	}
	// Find derivative
	for (i = 0; i < eleMinus1; i++)
	{
		if (b_x[i] == 0.0f)
			b_x[i] = -DBL_EPSILON;
	}
	i = 0;
	j = 0;
	while (j < eleMinus2)
	{
		if (b_x[j] * b_x[j + 1] < 0.0f)// This is so we find the first of repeated values
		{
			i++;
			ind[i] = j + 1;
			if (i >= eleMinus2)
				break;
			else
				j++;
		}
		else
			j++;
	}
	ind[0] = 0;
	ind[i + 1] = elements - 1;
	// Find where the derivative changes sign
	// Include endpoints in potential peaks and valleys as desired
	unsigned int indSize = i + 2;
	if (extrema)
	{
		b_x[0] = input[0];
		for (j = 0; j < indSize - 1; j++)
			b_x[j + 1] = input[ind[j + 1]];
		b_x[indSize + 1] = input[eleMinus1];
	}
	else
	{
		b_x[0] = -input[0];
		for (j = 0; j < indSize - 1; j++)
			b_x[j + 1] = -input[ind[j + 1]];
		b_x[indSize + 1] = -input[eleMinus1];
	}
	if (indSize <= 2)
	{
		if (b_x[0] > b_x[1])
			minMag = b_x[1];
		else
			minMag = b_x[0];
	}
	else
	{
		minMag = b_x[0];
		for (k = 2; k <= indSize; k++)
		{
			work = b_x[k - 1];
			if (minMag > work)
				minMag = work;
		}
	}
	leftMin = minMag;
	// x only has the peaks, valleys, and possibly endpoints
	if (indSize > 2)
	{
		// Function with peaks and valleys, set initial parameters for loop
		tempMag = minMag;
		foundPeak = 0;
		// Skip the first point if it is smaller so we always start on a maxima
		if (b_x[0] >= b_x[1])
			j = -1;
		else
			j = 0;
		k = 0;
		i = 1;
		// Loop through extrema which should be peaks and then valleys
		while (j + 1 < indSize)
		{
			j += 2;
			// This is a peak
			// Reset peak finding if we had a peak and the next peak is bigger
			// than the last or the left min was small enough to reset.
			if (foundPeak)
			{
				tempMag = minMag;
				foundPeak = 0;
			}
			// Found new peak that was lager than temp mag and selectivity larger
			// than the minimum to its left.
			work = b_x[j - 1];
			if ((work > tempMag) && (work > leftMin + sel))
			{
				i = j;
				tempMag = work;
			}
			// Make sure we don't iterate past the length of our vector
			if (j == indSize)
				break;
			else
			{
				// Move onto the valley, come down at least sel from peak
				if (tempMag > sel + b_x[j])
				{
					foundPeak = 1;
					// We have found a peak
					leftMin = b_x[j];
					peakLoc[k] = i;
					k++;
				}
				else
				{
					if (b_x[j] < leftMin) // New left minima
						leftMin = b_x[j];
				}
			}
		}
		// Check end point
		if ((b_x[indSize - 1] > tempMag) && (b_x[indSize - 1] > leftMin + sel))
		{
			peakLoc[k] = indSize;
			k++;
		}
		else
		{
			if ((!foundPeak) && (tempMag > minMag))
			{
				// Check if we still need to add the last point
				peakLoc[k] = i;
				k++;
			}
		}
		// Create output
		if (k + 1 > 1)
		{
			for (j = 0; j < k; j++)
				peakInds[j] = (float)ind[peakLoc[j] - 1];
		}
	}
	else
	{
		// This is a monotone function where an endpoint is the only peak
		if (b_x[0] < b_x[1])
		{
			work = b_x[1];
			i = 1;
		}
		else
		{
			work = b_x[0];
			i = 0;
		}
		if (work > minMag + sel)
		{
			peakInds[0] = (float)ind[i];
			k = 1;
		}
	}
	free(ind);
	free(b_x);
	free(peakLoc);
	return k;
}
static float NSEEL_CGEN_CALL _eel_peakFinder(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	float *input = __NSEEL_RAMAlloc(c->ram_state, (uint32_t)(*parms[0] + NSEEL_CLOSEFACTOR));
	float *output = __NSEEL_RAMAlloc(c->ram_state, (uint32_t)(*parms[4] + NSEEL_CLOSEFACTOR));
	uint32_t n = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	float sel = *parms[2];
	uint32_t maximaMinima = (uint32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	return (float)peakfinder(n, input, sel, maximaMinima, output);
}
static float NSEEL_CGEN_CALL _eel_writeWavFile(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	const char *filename = (const char*)GetStringForIndex(c->region_context, *parms[0], 0);
	uint32_t channels = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	uint32_t fs = (uint32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	uint64_t frameCount = (uint64_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	if ((num_param - 4) < (int32_t)channels)
		return -1;
	float *signal = (float*)malloc((size_t)(channels * frameCount * sizeof(float)));
	float **ptr = (float**)malloc(channels * sizeof(float*));
	for (uint32_t i = 0; i < channels; i++)
		ptr[i] = __NSEEL_RAMAlloc(blocks, (uint64_t)(uint32_t)(*parms[i + 4] + NSEEL_CLOSEFACTOR));
	channel_join(ptr, channels, signal, frameCount);
	drwav pWav;
	drwav_data_format format;
	format.container = drwav_container_riff;
	format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
	format.channels = channels;
	format.sampleRate = fs;
	format.bitsPerSample = 32;
	uint32_t fail = drwav_init_file_write(&pWav, filename, &format, 0);
	drwav_uint64 framesWritten = drwav_write_pcm_frames(&pWav, frameCount, signal);
	drwav_uninit(&pWav);
	free(signal);
	free(ptr);
	return 1;
}
static float NSEEL_CGEN_CALL _eel_writeWavMemory(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	s_str *dest = (s_str*)GetStringForIndex(c->region_context, *parms[0], 1);
	uint32_t channels = (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR);
	uint32_t fs = (uint32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	uint64_t frameCount = (uint64_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	if ((num_param - 4) < (int32_t)channels)
		return -1;
	float *signal = (float*)malloc((size_t)(channels * frameCount * sizeof(float)));
	float **ptr = (float**)malloc(channels * sizeof(float*));
	for (uint32_t i = 0; i < channels; i++)
		ptr[i] = __NSEEL_RAMAlloc(blocks, (uint64_t)(uint32_t)(*parms[i + 4] + NSEEL_CLOSEFACTOR));
	channel_join(ptr, channels, signal, frameCount);
	free(ptr);
	drwav pWav;
	drwav_data_format format;
	format.container = drwav_container_riff;
	format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
	format.channels = channels;
	format.sampleRate = fs;
	format.bitsPerSample = 32;
	size_t blkSize;
	void *memoryBlk;
	uint32_t fail = drwav_init_memory_write(&pWav, &memoryBlk, &blkSize, &format, 0);
	drwav_uint64 framesWritten = drwav_write_pcm_frames(&pWav, frameCount, signal);
	drwav_uninit(&pWav);
	free(signal);
	size_t outLen;
	unsigned char *base64String = base64_encode((unsigned char*)memoryBlk, blkSize, &outLen);
	free(memoryBlk);
	if (dest)
		s_str_destroy(dest);
	*dest = s_str_create_from_c_str(base64String);
	free(base64String);
	return 1;
}
static float NSEEL_CGEN_CALL _eel_listSystemVariable(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *blocks = c->ram_state;
	char badStr[128];
	stbsp_snprintf(badStr, 128, "Listing system variables\n");
	EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
	char isString[16];
	for (int i = 0; i < c->varTable_numBlocks; i++)
	{
		for (int j = 0; j < NSEEL_VARS_PER_BLOCK; j++)
		{
			char *valid = GetStringForIndex(c->region_context, c->varTable_Values[i][j], 1);
			if (!valid)
				strncpy(isString, "Is not string", 16);
			else
				strncpy(isString, "Could be string", 16);
			if (c->varTable_Names[i][j])
			{
				stbsp_snprintf(badStr, 128, "%s %1.18lf %s\n", c->varTable_Names[i][j], c->varTable_Values[i][j], isString);
				EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
			}
		}
	}
	stbsp_snprintf(badStr, 128, "All variables has been printed\n");
	EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
	return 0;
}
static float NSEEL_CGEN_CALL _eel_linspace(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *vec = __NSEEL_RAMAlloc(c->ram_state, (uint32_t)(*parms[3] + NSEEL_CLOSEFACTOR));
	int32_t n = (int32_t)(*parms[2] + NSEEL_CLOSEFACTOR);
	float d = (*parms[1] - *parms[0]) / (float)(n - 1);
	for (int32_t i = 0; i < n; i++)
		vec[i] = *parms[0] + i * d;
	return 0;
}
size_t choose(float *a, float *b, size_t src1, size_t src2)
{
	return (*b >= *a) ? src2 : src1;
}
size_t fast_upper_bound4(float *vec, size_t n, float *value)
{
	size_t size = n;
	size_t low = 0;
	while (size >= 8)
	{
		size_t half = size / 2;
		size_t other_half = size - half;
		size_t probe = low + half;
		size_t other_low = low + other_half;
		size = half;
		low = choose(&vec[probe], value, low, other_low);

		half = size / 2;
		other_half = size - half;
		probe = low + half;
		other_low = low + other_half;
		size = half;
		low = choose(&vec[probe], value, low, other_low);

		half = size / 2;
		other_half = size - half;
		probe = low + half;
		other_low = low + other_half;
		size = half;
		low = choose(&vec[probe], value, low, other_low);
	}
	while (size > 0) {
		size_t half = size / 2;
		size_t other_half = size - half;
		size_t probe = low + half;
		size_t other_low = low + other_half;
		size = half;
		low = choose(&vec[probe], value, low, other_low);
	}
	return low;
}
static float _eel_lerp(void *opaque, INT_PTR num_param, float **parms)
{
	compileContext *c = (compileContext*)opaque;
	float *x = __NSEEL_RAMAlloc(c->ram_state, (uint32_t)(*parms[1] + NSEEL_CLOSEFACTOR));
	float *y = __NSEEL_RAMAlloc(c->ram_state, (uint32_t)(*parms[2] + NSEEL_CLOSEFACTOR));
	uint32_t pts = (uint32_t)(*parms[3] + NSEEL_CLOSEFACTOR);
	if (*parms[0] == x[0])
		return y[0];
	if (*parms[0] == x[pts - 1])
		return y[pts - 1];
	size_t j = fast_upper_bound4(x, pts, parms[0]);
	if (j <= 0)
		return (*parms[0] - x[1]) / (x[0] - x[1]) * (y[0] - y[1]) + y[1]; // Extrapolation to leftmost
	else if (j >= pts)
		return (*parms[0] - x[pts - 2]) / (x[pts - 1] - x[pts - 2]) * (y[pts - 1] - y[pts - 2]) + y[pts - 2]; // Extrapolation to rightmost
	else
		return (*parms[0] - x[j - 1]) / (x[j] - x[j - 1]) * (y[j] - y[j - 1]) + y[j - 1]; // Interpolation
}
#define redirect(x) static float redirect_##x(float _input1){return x(_input1); }
#define redirect2(x) static float redirect_##x(float _input1, float _input2){return x(_input1, _input2); }
redirect(sinf);
redirect(cosf);
redirect(tanf);
redirect(asinf);
redirect(acosf);
redirect(atanf);
redirect2(atan2f);
redirect(asinhf);
redirect(acoshf);
redirect(coshf);
redirect(sinhf);
redirect(sqrtf);
redirect(tanhf);
redirect(atanhf)
redirect(logf)
redirect(log10f)
redirect2(hypotf)
redirect2(powf)
redirect(expf)
redirect(roundf)
redirect(floorf)
redirect(ceilf)
static functionType fnTable1[] = {
  {"sin",    nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_WONTMAKEDENORMAL, {(void**)&redirect_sinf} },
  {"cos",    nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_CLEARDENORMAL, {(void**)&redirect_cosf} },
  {"tan",    nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_tanf}  },
  {"sqrt",   nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_WONTMAKEDENORMAL, {(void**)&redirect_sqrtf}, },
  {"asin",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_asinf}, },
  {"acos",   nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_acosf}, },
  {"atan",   nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_atanf}, },
  {"atan2",  nseel_asm_2pdd,nseel_asm_2pdd_end, 2 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK, {(void**)&redirect_atan2f}, },
  {"sinh",    nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_WONTMAKEDENORMAL, {(void**)&redirect_sinhf} },
  {"cosh",    nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_CLEARDENORMAL, {(void**)&redirect_coshf} },
  {"tanh",    nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_tanhf}  },
  {"asinh",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_asinhf}, },
  {"acosh",   nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_acoshf}, },
  {"atanh",   nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_atanhf}, },
  {"log",    nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_logf} },
  {"log10",  nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_log10f} },
  {"hypot",  nseel_asm_2pdd,nseel_asm_2pdd_end, 2 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK, {(void**)&redirect_hypotf}, },
  {"pow",    nseel_asm_2pdd,nseel_asm_2pdd_end, 2 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK, {(void**)&redirect_powf}, },
  {"exp",    nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&redirect_expf}, },
  {"abs",    nseel_asm_abs,nseel_asm_abs_end,   1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(0) | BIF_WONTMAKEDENORMAL },
  {"sqr",    nseel_asm_sqr,nseel_asm_sqr_end,   1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(1) },
  {"min",    nseel_asm_min,nseel_asm_min_end,   2 | NSEEL_NPARAMS_FLAG_CONST | BIF_FPSTACKUSE(3) | BIF_WONTMAKEDENORMAL },
  {"max",    nseel_asm_max,nseel_asm_max_end,   2 | NSEEL_NPARAMS_FLAG_CONST | BIF_FPSTACKUSE(3) | BIF_WONTMAKEDENORMAL },
  {"sign",   nseel_asm_sign,nseel_asm_sign_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL, },
  {"rand",   nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_CLEARDENORMAL, {(void**)&nseel_int_rand}, },
  {"round",  nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_CLEARDENORMAL, {(void**)&redirect_roundf} },
  {"floor",  nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_CLEARDENORMAL, {(void**)&redirect_floorf} },
  {"ceil",   nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_CLEARDENORMAL, {(void**)&redirect_ceilf} },
  {"expint", nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_CLEARDENORMAL, {(void**)&expint} },
  {"expintFast",nseel_asm_1pdd,nseel_asm_1pdd_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_CLEARDENORMAL, {(void**)&expint_interpolation} },
  {"invsqrt",nseel_asm_1pdd,nseel_asm_1pdd_end,1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(3), {(void**)&invsqrt} },
  {"invsqrtFast",nseel_asm_invsqrt,nseel_asm_invsqrt_end,1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(3), },
  {"circshift",_asm_generic3parm,_asm_generic3parm_end,3,{(void**)&__NSEEL_circshift},NSEEL_PProc_RAM},
  {"convolve_c",_asm_generic3parm,_asm_generic3parm_end,3,{(void**)&eel_convolve_c},NSEEL_PProc_RAM},
  {"maxVec",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&eel_max},NSEEL_PProc_RAM},
  {"minVec",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&eel_min},NSEEL_PProc_RAM},
  {"meanVec",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&eel_mean},NSEEL_PProc_RAM},
  {"medianVec",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&eel_median},NSEEL_PProc_RAM},
  {"fft",_asm_generic2parm,_asm_generic2parm_end,2,{(void**)&eel_fft},NSEEL_PProc_RAM},
  {"ifft",_asm_generic2parm,_asm_generic2parm_end,2,{(void**)&eel_ifft},NSEEL_PProc_RAM},
  {"fft_real",_asm_generic2parm,_asm_generic2parm_end,2,{(void**)&eel_fft_real},NSEEL_PProc_RAM},
  {"ifft_real",_asm_generic2parm,_asm_generic2parm_end,2,{(void**)&eel_ifft_real},NSEEL_PProc_RAM},
  {"fft_permute",_asm_generic2parm,_asm_generic2parm_end,2,{(void**)&eel_fft_permute},NSEEL_PProc_RAM},
  {"fft_ipermute",_asm_generic2parm,_asm_generic2parm_end,2,{(void**)&eel_ifft_permute},NSEEL_PProc_RAM},
  {"memcpy",_asm_generic3parm,_asm_generic3parm_end,3,{(void**)&__NSEEL_RAM_MemCpy},NSEEL_PProc_RAM},
  {"memset",_asm_generic3parm,_asm_generic3parm_end,3,{(void**)&__NSEEL_RAM_MemSet},NSEEL_PProc_RAM},
  {"sleep",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK,{(void**)&_eel_sleep}},
  {"time",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK,{(void**)&_eel_time},NSEEL_PProc_THIS},
  {"time_precise",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK,{(void**)&_eel_time_precise},NSEEL_PProc_THIS},
  {"strlen",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_strlen},NSEEL_PProc_THIS},
  {"base64_encode",_asm_generic3parm_retd,_asm_generic3parm_retd_end,3 | BIF_RETURNSONSTACK,{(void**)&_eel_base64_encode},NSEEL_PProc_THIS},
  {"base64_encodeF2F",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&_eel_base64_encodeBinaryToTextFile},NSEEL_PProc_THIS},
  {"base64_decode",_asm_generic3parm_retd,_asm_generic3parm_retd_end,3 | BIF_RETURNSONSTACK,{(void**)&_eel_base64_decode},NSEEL_PProc_THIS},
  {"base64_decodeF2F",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&_eel_base64_decodeBinaryToTextFile},NSEEL_PProc_THIS},
  {"strcmp",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&_eel_strcmp},NSEEL_PProc_THIS},
  {"match",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_match},NSEEL_PProc_THIS},
  {"matchi",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_matchi},NSEEL_PProc_THIS},
  {"stricmp",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&_eel_stricmp},NSEEL_PProc_THIS},
  {"strncmp",_asm_generic3parm_retd,_asm_generic3parm_retd_end,3 | BIF_RETURNSONSTACK,{(void**)&_eel_strncmp},NSEEL_PProc_THIS},
  {"strnicmp",_asm_generic3parm_retd,_asm_generic3parm_retd_end,3 | BIF_RETURNSONSTACK,{(void**)&_eel_strnicmp},NSEEL_PProc_THIS},
  {"printf",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_printf},NSEEL_PProc_THIS},
  {"sprintf",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_sprintf},NSEEL_PProc_THIS},
  {"resetSysMemRegion",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_resetSysMemRegion},NSEEL_PProc_THIS},
  {"importFLTFromStr",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&_eel_importFloatArrayFromString},NSEEL_PProc_THIS},
  {"arburgCheckMemoryRequirement",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&arburgCheckMemoryRequirement},NSEEL_PProc_RAM},
  {"arburgTrainModel",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&arburgTrainModel},NSEEL_PProc_THIS},
  {"arburgPredictBackward",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&arburgPredictBackward},NSEEL_PProc_RAM},
  {"arburgPredictForward",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&arburgPredictForward},NSEEL_PProc_RAM},
  {"getCosineWindows",_asm_generic2parm_retd,_asm_generic2parm_retd_end,3 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&getCosineWindows},NSEEL_PProc_THIS},
  {"getAsymmetricCosine",_asm_generic2parm_retd,_asm_generic2parm_retd_end,5 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&getAsymmetricCosine},NSEEL_PProc_THIS},
  {"stftCheckMemoryRequirement",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&stftCheckMemoryRequirement},NSEEL_PProc_THIS},
  {"stftInit",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&stftInit},NSEEL_PProc_THIS},
  {"iirHilbertProcess",_asm_generic3parm_retd,_asm_generic3parm_retd_end,3 | BIF_RETURNSONSTACK,{(void **)&iirHilbertProcess},NSEEL_PProc_RAM},
  {"iirHilbertInit",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&iirHilbertInit},NSEEL_PProc_THIS},
  {"movingMinMaxProcess",_asm_generic3parm_retd,_asm_generic3parm_retd_end,3 | BIF_RETURNSONSTACK,{(void **)&movingMinMaxProcess},NSEEL_PProc_RAM},
  {"movingMinMaxInit",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&movingMinMaxInit},NSEEL_PProc_RAM},
  {"movingMedianProcess",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void **)&movingMedianProcess},NSEEL_PProc_RAM},
  {"movingMedianInit",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&movingMedianInit},NSEEL_PProc_RAM},
  {"stftGetWindowPower",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&stftGetWindowPower},NSEEL_PProc_THIS},
  {"stftForward",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&stftForward},NSEEL_PProc_THIS},
  {"stftBackward",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&stftBackward},NSEEL_PProc_THIS},
  {"stftSetAsymWnd",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&stftSetAsymWnd},NSEEL_PProc_THIS},
  {"InitPinkNoise",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&pinkNoiseInit},NSEEL_PProc_THIS},
  {"GeneratePinkNoise",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&pinkNoiseGen},NSEEL_PProc_THIS},
  {"InitPolyphaseFilterbank",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&PolyphaseFilterbankInit},NSEEL_PProc_THIS},
  {"PolyphaseFilterbankChangeWarpingFactor",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&PolyphaseFilterbankChangeWarpingFactor},NSEEL_PProc_THIS},
  {"PolyphaseFilterbankGetPhaseCorrector",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&PolyphaseFilterbankGetPhaseCorrector},NSEEL_PProc_THIS},
  {"PolyphaseFilterbankGetDecimationFactor",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&PolyphaseFilterbankGetDecimationFactor},NSEEL_PProc_THIS},
  {"PolyphaseFilterbankAnalysisMono",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&PolyphaseFilterbankAnalysisMono},NSEEL_PProc_THIS},
  {"PolyphaseFilterbankSynthesisMono",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&PolyphaseFilterbankSynthesisMono},NSEEL_PProc_THIS},
  {"PolyphaseFilterbankAnalysisStereo",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&PolyphaseFilterbankAnalysisStereo},NSEEL_PProc_THIS},
  {"PolyphaseFilterbankSynthesisStereo",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&PolyphaseFilterbankSynthesisStereo},NSEEL_PProc_THIS},
  {"FIRInit",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&FIRInit},NSEEL_PProc_RAM},
  {"FIRProcess",_asm_generic3parm_retd,_asm_generic3parm_retd_end,3 | BIF_RETURNSONSTACK,{(void**)&FIRProcess},NSEEL_PProc_RAM},
  {"fractionalDelayLineInit",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&fractionalDelayLineInit},NSEEL_PProc_RAM},
  {"fractionalDelayLineClear",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&fractionalDelayLineClear},NSEEL_PProc_RAM},
  {"fractionalDelayLineSetDelay",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&fractionalDelayLineSetDelay},NSEEL_PProc_RAM},
  {"fractionalDelayLineProcess",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_RETURNSONSTACK,{(void**)&fractionalDelayLineProcess},NSEEL_PProc_RAM},
  {"linspace",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_linspace},NSEEL_PProc_THIS},
  {"rank",_asm_generic3parm_retd,_asm_generic3parm_retd_end,3 | BIF_RETURNSONSTACK,{(void**)&_eel_matRank},NSEEL_PProc_RAM},
  {"det",_asm_generic3parm_retd,_asm_generic3parm_retd_end,3 | BIF_RETURNSONSTACK,{(void**)&_eel_matDeterminant},NSEEL_PProc_RAM},
  {"transpose",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_matTranspose},NSEEL_PProc_THIS},
  {"cholesky",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_cholesky},NSEEL_PProc_THIS},
  {"inv_chol",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_inv_chol},NSEEL_PProc_THIS},
  {"inv",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_matInv},NSEEL_PProc_THIS},
  {"pinv_svd",_asm_generic2parm_retd,_asm_generic2parm_retd_end,5 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_matPinv},NSEEL_PProc_THIS},
  {"pinv_fast",_asm_generic2parm_retd,_asm_generic2parm_retd_end,5 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_matPinvFast},NSEEL_PProc_THIS},
  {"mldivide",_asm_generic2parm_retd,_asm_generic2parm_retd_end,8 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_mldivide},NSEEL_PProc_THIS},
  {"mrdivide",_asm_generic2parm_retd,_asm_generic2parm_retd_end,8 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_mrdivide},NSEEL_PProc_THIS},
  {"quadprog",_asm_generic2parm_retd,_asm_generic2parm_retd_end,12 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_quadprog},NSEEL_PProc_THIS},
  {"lsqlin",_asm_generic2parm_retd,_asm_generic2parm_retd_end,13 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_lsqlin},NSEEL_PProc_THIS},
  {"firls",_asm_generic2parm_retd,_asm_generic2parm_retd_end,7 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_firls},NSEEL_PProc_THIS},
  {"eqnerror",_asm_generic2parm_retd,_asm_generic2parm_retd_end,10 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_eqnerror},NSEEL_PProc_THIS},
  {"unwrap",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_unwrap},NSEEL_PProc_THIS},
  {"zp2sos",_asm_generic2parm_retd,_asm_generic2parm_retd_end,7 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_zp2sos},NSEEL_PProc_THIS},
  {"tf2sos",_asm_generic2parm_retd,_asm_generic2parm_retd_end,6 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_tf2sos},NSEEL_PProc_THIS},
  {"roots",_asm_generic2parm_retd,_asm_generic2parm_retd_end,5 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_roots},NSEEL_PProc_THIS},
  {"cplxpair",_asm_generic2parm_retd,_asm_generic2parm_retd_end,5 | BIF_TAKES_VARPARM_EX | BIF_RETURNSONSTACK,{(void**)&_eel_cplxpair},NSEEL_PProc_THIS},
  {"IIRBandSplitterInit",_asm_generic2parm_retd,_asm_generic2parm_retd_end,3 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_iirBandSplitterInit},NSEEL_PProc_THIS},
  {"IIRBandSplitterClearState",_asm_generic1parm_retd,_asm_generic1parm_retd_end, 1 | BIF_RETURNSONSTACK,{(void**)&_eel_iirBandSplitterClearState},NSEEL_PProc_RAM},
  {"IIRBandSplitterProcess",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_iirBandSplitterProcess},NSEEL_PProc_THIS},
  {"decodeFLACFromFile",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_flacDecodeFile},NSEEL_PProc_THIS},
  {"decodeFLACFromMemory",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_flacDecodeMemory},NSEEL_PProc_THIS},
  {"decodeWavFromFile",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_wavDecodeFile},NSEEL_PProc_THIS},
  {"decodeWavFromMemory",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_wavDecodeMemory},NSEEL_PProc_THIS},
  {"writeWavToFile",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_writeWavFile},NSEEL_PProc_THIS},
  {"writeWavToBase64String",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_writeWavMemory},NSEEL_PProc_THIS},
  {"peakFinder",_asm_generic2parm_retd,_asm_generic2parm_retd_end,5 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_peakFinder},NSEEL_PProc_THIS },
  {"listSystemVariable",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_listSystemVariable},NSEEL_PProc_THIS },
  {"vectorizeAssignScalar",_asm_generic2parm_retd,_asm_generic2parm_retd_end,3 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_vectorizeAssignScalar},NSEEL_PProc_THIS },
  {"vectorizeAdd",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_vectorizeAdd},NSEEL_PProc_THIS },
  {"vectorizeMinus",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_vectorizeMinus},NSEEL_PProc_THIS },
  {"vectorizeMultiply",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_vectorizeMultiply},NSEEL_PProc_THIS },
  {"vectorizeDivide",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void**)&_eel_vectorizeDivide},NSEEL_PProc_THIS },
  { "lerpAt",_asm_generic2parm_retd,_asm_generic2parm_retd_end,4 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_lerp},NSEEL_PProc_THIS },
  {"ls",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_ls},NSEEL_PProc_THIS },
  {"cd",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_cd},NSEEL_PProc_THIS },
  {"eval",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_eval},NSEEL_PProc_THIS },
  {"evalFile",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_evalFile},NSEEL_PProc_THIS },
  {"Conv1DInit",_asm_generic2parm_retd,_asm_generic2parm_retd_end,3 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_initfftconv1d},NSEEL_PProc_THIS },
  {"Conv1DProcess",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_processfftconv1d},NSEEL_PProc_THIS },
  {"ThreadInit",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_initthread},NSEEL_PProc_THIS },
  {"ThreadCreate",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_createThread},NSEEL_PProc_THIS },
  {"ThreadJoin",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_joinThread},NSEEL_PProc_THIS },
  {"ThreadMutexLock",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_lockThread},NSEEL_PProc_THIS },
  {"ThreadMutexUnlock",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void**)&_eel_unlockThread},NSEEL_PProc_THIS },
  {"xFloatS",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void **)&_eel_createHPFloatFromString},NSEEL_PProc_THIS },
  {"xFloatF",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void **)&_eel_createHPFloatFromFloat32},NSEEL_PProc_THIS },
  {"xF2f32",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void **)&_eel_createHPFloatToFloat32},NSEEL_PProc_THIS },
  {"xF2str",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void **)&_eel_createHPFloatToString},NSEEL_PProc_THIS },
  {"xAdd",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFAdd},NSEEL_PProc_THIS },
  {"xSub",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFSub},NSEEL_PProc_THIS },
  {"xMul",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFMul},NSEEL_PProc_THIS },
  {"xDiv",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFDiv},NSEEL_PProc_THIS },
  {"xfrexp",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFfrexp},NSEEL_PProc_THIS },
  {"xqfmod",_asm_generic2parm_retd,_asm_generic2parm_retd_end,3 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFqfmod},NSEEL_PProc_THIS },
  {"xfmod",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFfmod},NSEEL_PProc_THIS },
  {"xsfmod",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFsfmod},NSEEL_PProc_THIS },
  {"xMulPowOf2",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFMulPowOf2},NSEEL_PProc_THIS },
  {"xfrac",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFfrac},NSEEL_PProc_THIS },
  {"xabs",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFabs},NSEEL_PProc_THIS },
  {"xtrunc",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFtrunc},NSEEL_PProc_THIS },
  {"xround",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFround},NSEEL_PProc_THIS },
  {"xceil",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFceil},NSEEL_PProc_THIS },
  {"xfloor",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFfloor},NSEEL_PProc_THIS },
  {"xfix",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFfix},NSEEL_PProc_THIS },
  {"xtan",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFtan},NSEEL_PProc_THIS },
  {"xsin",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFsin},NSEEL_PProc_THIS },
  {"xcos",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFcos},NSEEL_PProc_THIS },
  {"xatan",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFatan},NSEEL_PProc_THIS },
  {"xasin",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFasin},NSEEL_PProc_THIS },
  {"xacos",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFacos},NSEEL_PProc_THIS },
  {"xNegation",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFNegation},NSEEL_PProc_THIS },
  {"xatan2",_asm_generic2parm_retd,_asm_generic2parm_retd_end,2 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFatan2},NSEEL_PProc_THIS },
  {"xsqrt",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFsqrt},NSEEL_PProc_THIS },
  {"xexp",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFexp},NSEEL_PProc_THIS },
  {"xexp2",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFexp2},NSEEL_PProc_THIS },
  {"xexp10",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFexp10},NSEEL_PProc_THIS },
  {"xlog",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFlog},NSEEL_PProc_THIS },
  {"xlog2",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFlog2},NSEEL_PProc_THIS },
  {"xlog10",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFlog10},NSEEL_PProc_THIS },
  {"xtanh",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFtanh},NSEEL_PProc_THIS },
  {"xsinh",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFsinh},NSEEL_PProc_THIS },
  {"xcosh",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFcosh},NSEEL_PProc_THIS },
  {"xatanh",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFatanh},NSEEL_PProc_THIS },
  {"xasinh",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFasinh},NSEEL_PProc_THIS },
  {"xacosh",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFacosh},NSEEL_PProc_THIS },
  {"xclone",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFclone},NSEEL_PProc_THIS },
  {"xpow",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFpow},NSEEL_PProc_THIS },
  {"xintpow",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFIntPow},NSEEL_PProc_THIS },
  {"xbinexp",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFbinaryexp},NSEEL_PProc_THIS },
  {"xequal",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFeq},NSEEL_PProc_THIS },
  {"xnotequal",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFneq},NSEEL_PProc_THIS },
  {"xlessequal",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFle},NSEEL_PProc_THIS },
  {"xgreaterequal",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFge},NSEEL_PProc_THIS },
  {"xgreater",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFgt},NSEEL_PProc_THIS },
  {"xless",_asm_generic2parm_retd,_asm_generic2parm_retd_end,1 | BIF_TAKES_VARPARM | BIF_RETURNSONSTACK,{(void **)&_eel_HPFlt},NSEEL_PProc_THIS },
  {"DeleteSysVariable",_asm_generic1parm_retd,_asm_generic1parm_retd_end,1 | BIF_RETURNSONSTACK,{(void **)&_eel_deleteSysVariable},NSEEL_PProc_THIS },
};
void printFunctions()
{
	char badStr[128];
	for (int i = 0; i < sizeof(fnTable1) / sizeof(fnTable1[0]); i++)
	{
		stbsp_snprintf(badStr, 128, "%s\n", fnTable1[i].name);
		EEL_STRING_STDOUT_WRITE(badStr, strlen(badStr));
	}
}
int XLITTLE_ENDIAN;
void NSEEL_start()
{
	union
	{
		unsigned char cc[4];
		unsigned short s[2];
		unsigned long n;
	} u;
	u.n = 0x12345678;
	XLITTLE_ENDIAN = (u.s[0] == 0x5678 && u.s[1] == 0x1234 && u.cc[0] == 0x78 && u.cc[1] == 0x56 && u.cc[2] == 0x34 && u.cc[3] == 0x12);
	// Global memory
	precompute_lpfcoeff();
	initFFTData();
	srand((uint32_t)time(NULL));
}
void NSEEL_quit()
{
	clean_lpfcoeff();
}
//---------------------------------------------------------------------------------------------------------------
static void freeBlocks(llBlock **start)
{
	llBlock *s = *start;
	*start = 0;
	while (s)
	{
		llBlock *llB = s->next;
		free(s);
		s = llB;
	}
}
//---------------------------------------------------------------------------------------------------------------
static void *__newBlock(llBlock **start, int32_t size)
{
	llBlock *llb;
	int32_t alloc_size;
	if (*start && (LLB_DSIZE - (*start)->sizeused) >= size)
	{
		void *t = (*start)->block + (*start)->sizeused;
		(*start)->sizeused += (size + 7)&~7;
		return t;
	}
	alloc_size = sizeof(llBlock);
	if ((int32_t)size > LLB_DSIZE) alloc_size += size - LLB_DSIZE;
	llb = (llBlock *)malloc(alloc_size); // grab bigger block if absolutely necessary (heh)
	if (!llb) return NULL;
	llb->sizeused = (size + 7)&~7;
	llb->next = *start;
	*start = llb;
	return llb->block;
}
//---------------------------------------------------------------------------------------------------------------
opcodeRec *nseel_createCompiledValue(compileContext *ctx, float value)
{
	opcodeRec *r = newOpCode(ctx, NULL, OPCODETYPE_DIRECTVALUE);
	if (r)
	{
		r->parms.dv.directValue = value;
	}
	return r;
}
opcodeRec *nseel_createCompiledValuePtr(compileContext *ctx, float *addrValue, const char *namestr)
{
	opcodeRec *r = newOpCode(ctx, namestr, OPCODETYPE_VARPTR);
	if (!r) return 0;
	r->parms.dv.valuePtr = addrValue;
	return r;
}
static int32_t validate_varname_for_function(compileContext *ctx, const char *name)
{
	if (!ctx->function_curName || !ctx->function_globalFlag) return 1;
	if (ctx->function_localTable_Size[2] > 0 && ctx->function_localTable_Names[2])
	{
		char * const * const namelist = ctx->function_localTable_Names[2];
		const int32_t namelist_sz = ctx->function_localTable_Size[2];
		int32_t i;
		const size_t name_len = strlen(name);
		for (i = 0; i < namelist_sz; i++)
		{
			const char *nmchk = namelist[i];
			const size_t l = strlen(nmchk);
			if (l > 1 && nmchk[l - 1] == '*')
			{
				if (name_len >= l && !strncmp(nmchk, name, l - 1) && name[l - 1] == '.')  return 1;
			}
			else
			{
				if (name_len == l && !strcmp(nmchk, name)) return 1;
			}
		}
	}
	return 0;
}
opcodeRec *nseel_resolve_named_symbol(compileContext *ctx, opcodeRec *rec, int32_t parmcnt, int32_t *errOut)
{
	const int32_t isFunctionMode = parmcnt >= 0;
	int32_t rel_prefix_len = 0;
	int32_t rel_prefix_idx = -2;
	int32_t i;
	char match_parmcnt[4] = { -1,-1,-1,-1 }; // [3] is guess
	unsigned char match_parmcnt_pos = 0;
	char *sname = (char *)rec->relname;
	const char *prevent_function_calls = NULL;
	if (errOut) *errOut = 0;
	if (rec->opcodeType != OPCODETYPE_VARPTR || !sname || !sname[0]) return NULL;
	if (ctx->function_curName)
	{
		if (!strncmp(sname, "this.", 5))
		{
			rel_prefix_len = 5;
			rel_prefix_idx = -1;
		}
		else if (!strcmp(sname, "this"))
		{
			rel_prefix_len = 4;
			rel_prefix_idx = -1;
		}
		// scan for parameters/local variables before user functions   
		if (rel_prefix_idx < -1 && ctx->function_localTable_Size[0] > 0 && ctx->function_localTable_Names[0] && ctx->function_localTable_ValuePtrs)
		{
			char * const * const namelist = ctx->function_localTable_Names[0];
			const int32_t namelist_sz = ctx->function_localTable_Size[0];
			for (i = 0; i < namelist_sz; i++)
			{
				const char *p = namelist[i];
				if (p)
				{
					if (!isFunctionMode && !strncmp(p, sname, NSEEL_MAX_VARIABLE_NAMELEN))
					{
						rec->opcodeType = OPCODETYPE_VARPTRPTR;
						rec->parms.dv.valuePtr = (float *)(ctx->function_localTable_ValuePtrs + i);
						rec->parms.dv.directValue = 0.0f;
						return rec;
					}
					else
					{
						const size_t plen = strlen(p);
						if (plen > 1 && p[plen - 1] == '*' && !strncmp(p, sname, plen - 1) && ((sname[plen - 1] == '.'&&sname[plen]) || !sname[plen - 1]))
						{
							rel_prefix_len = (int32_t)(sname[plen - 1] ? plen : plen - 1);
							rel_prefix_idx = i;
							break;
						}
					}
				}
			}
		}
		// if instance name set, translate sname or sname.* into "this.sname.*"
		if (rel_prefix_idx < -1 && ctx->function_localTable_Size[1] > 0 && ctx->function_localTable_Names[1])
		{
			char * const * const namelist = ctx->function_localTable_Names[1];
			const int32_t namelist_sz = ctx->function_localTable_Size[1];
			const char *full_sname = rec->relname; // include # in checks
			for (i = 0; i < namelist_sz; i++)
			{
				const char *p = namelist[i];
				if (p && *p)
				{
					const size_t tl = strlen(p);
					if (!strncmp(p, full_sname, tl) && (full_sname[tl] == 0 || full_sname[tl] == '.'))
					{
						rel_prefix_len = 0; // treat as though this. prefixes is present
						rel_prefix_idx = -1;
						break;
					}
				}
			}
		}
		if (rel_prefix_idx >= -1)
		{
			ctx->function_usesNamespaces = 1;
		}
	} // ctx->function_curName
	if (!isFunctionMode)
	{
		// instance variables
		if (rel_prefix_idx >= -1)
		{
			rec->opcodeType = OPCODETYPE_VALUE_FROM_NAMESPACENAME;
			rec->namespaceidx = rel_prefix_idx;
			if (rel_prefix_len > 0)
			{
				memmove(sname, sname + rel_prefix_len, strlen(sname + rel_prefix_len) + 1);
			}
		}
		else
		{
			// no namespace index, so it must be a global
			if (!validate_varname_for_function(ctx, rec->relname))
			{
				if (errOut) *errOut = 1;
				if (ctx->last_error_string[0]) lstrcatn(ctx->last_error_string, ", ", sizeof(ctx->last_error_string));
				snprintf_append(ctx->last_error_string, sizeof(ctx->last_error_string), "global '%s' inaccessible", rec->relname);
				return NULL;
			}
		}
		return rec;
	}
	if (ctx->func_check)
		prevent_function_calls = ctx->func_check(sname, ctx->func_check_user);
	////////// function mode
	// first off, while() and loop() are special and can't be overridden
	//
	if (parmcnt == 1 && !strcmp("while", sname) && !prevent_function_calls)
	{
		rec->opcodeType = OPCODETYPE_FUNC1;
		rec->fntype = FN_WHILE;
		return rec;
	}
	if (parmcnt == 2 && !strcmp("loop", sname) && !prevent_function_calls)
	{
		rec->opcodeType = OPCODETYPE_FUNC2;
		rec->fntype = FN_LOOP;
		return rec;
	}
	//
	// resolve user function names before builtin functions -- this allows the user to override default functions
	{
		_codeHandleFunctionRec *best = NULL;
		size_t bestlen = 0;
		const char * const ourcall = sname + rel_prefix_len;
		const size_t ourcall_len = strlen(ourcall);
		int32_t pass;
		for (pass = 0; pass < 2; pass++)
		{
			_codeHandleFunctionRec *fr = pass ? ctx->functions_common : ctx->functions_local;
			// sname is [namespace.[ns.]]function, find best match of function that matches the right end   
			while (fr)
			{
				int32_t this_np = fr->num_params;
				const char *thisfunc = fr->fname;
				const size_t thisfunc_len = strlen(thisfunc);
				if (this_np < 1) this_np = 1;
				if (thisfunc_len == ourcall_len && !strcmp(thisfunc, ourcall))
				{
					if (this_np == parmcnt)
					{
						bestlen = thisfunc_len;
						best = fr;
						break; // found exact match, finished
					}
					else
					{
						if (match_parmcnt_pos < 3) match_parmcnt[match_parmcnt_pos++] = fr->num_params;
					}
				}
				if (thisfunc_len > bestlen && thisfunc_len < ourcall_len && ourcall[ourcall_len - thisfunc_len - 1] == '.' && !strcmp(thisfunc, ourcall + ourcall_len - thisfunc_len))
				{
					if (this_np == parmcnt)
					{
						bestlen = thisfunc_len;
						best = fr;
					}
					else
						if (match_parmcnt[3] < 0) match_parmcnt[3] = fr->num_params;
				}
				fr = fr->next;
			}
			if (fr) break; // found exact match, finished
		}
		if (best)
		{
			switch (parmcnt)
			{
			case 0:
			case 1: rec->opcodeType = OPCODETYPE_FUNC1; break;
			case 2: rec->opcodeType = OPCODETYPE_FUNC2; break;
			case 3: rec->opcodeType = OPCODETYPE_FUNC3; break;
			default: rec->opcodeType = OPCODETYPE_FUNCX; break;
			}
			if (ourcall != rec->relname) memmove((char *)rec->relname, ourcall, strlen(ourcall) + 1);
			if (ctx->function_curName && rel_prefix_idx < 0)
			{
				// if no namespace specified, and this.commonprefix.func() called, remove common prefixes and set prefixidx to be this
				const char *p = ctx->function_curName;
				if (*p) p++;
				while (*p && *p != '.')  p++;
				if (*p && p[1]) // we have a dot!
				{
					while (p[1]) p++; // go to last char of string, which doesn't allow possible trailing dot to be checked
					while (--p > ctx->function_curName) // do not check possible leading dot
					{
						if (*p == '.')
						{
							const size_t cmplen = p + 1 - ctx->function_curName;
							if (!strncmp(rec->relname, ctx->function_curName, cmplen) && rec->relname[cmplen])
							{
								const char *src = rec->relname + cmplen;
								memmove((char *)rec->relname, src, strlen(src) + 1);
								rel_prefix_idx = -1;
								ctx->function_usesNamespaces = 1;
								break;
							}
						}
					}
				}
			}
			if (ctx->function_curName && rel_prefix_idx < -1 &&
				strchr(rec->relname, '.') && !validate_varname_for_function(ctx, rec->relname))
			{
				if (errOut) *errOut = 1;
				if (ctx->last_error_string[0]) lstrcatn(ctx->last_error_string, ", ", sizeof(ctx->last_error_string));
				snprintf_append(ctx->last_error_string, sizeof(ctx->last_error_string), "namespaced function '%s' inaccessible", rec->relname);
				return NULL;
			}
			rec->namespaceidx = rel_prefix_idx;
			rec->fntype = FUNCTYPE_EELFUNC;
			rec->fn = best;
			return rec;
		}
	}
	if (prevent_function_calls)
	{
		if (ctx->last_error_string[0]) lstrcatn(ctx->last_error_string, ", ", sizeof(ctx->last_error_string));
		snprintf_append(ctx->last_error_string, sizeof(ctx->last_error_string), "'%.30s': %s", sname, prevent_function_calls);
		if (errOut) *errOut = 0;
		return NULL;
	}
	// convert legacy pow() to FN_POW
	if (!strcmp("pow", sname))
	{
		if (parmcnt == 2)
		{
			rec->opcodeType = OPCODETYPE_FUNC2;
			rec->fntype = FN_POW;
			return rec;
		}
		if (match_parmcnt_pos < 3) match_parmcnt[match_parmcnt_pos++] = 2;
	}
	else if (!strcmp("__denormal_likely", sname) || !strcmp("__denormal_unlikely", sname))
	{
		if (parmcnt == 1)
		{
			rec->opcodeType = OPCODETYPE_FUNC1;
			rec->fntype = !strcmp("__denormal_likely", sname) ? FN_DENORMAL_LIKELY : FN_DENORMAL_UNLIKELY;
			return rec;
		}
	}
	for (i = 0; fnTable1 + i; i++)
	{
		if (i >= (int32_t)(sizeof(fnTable1) / sizeof(fnTable1[0])))
			break;
		functionType *f = fnTable1 + i;
		if (!strcmp(f->name, sname))
		{
			const int32_t pc_needed = (f->nParams&FUNCTIONTYPE_PARAMETERCOUNTMASK);
			if ((f->nParams&BIF_TAKES_VARPARM_EX) == BIF_TAKES_VARPARM ? (parmcnt >= pc_needed) : (parmcnt == pc_needed))
			{
				rec->fntype = FUNCTYPE_FUNCTIONTYPEREC;
				rec->fn = (void *)f;
				switch (parmcnt)
				{
				case 0:
				case 1: rec->opcodeType = OPCODETYPE_FUNC1; break;
				case 2: rec->opcodeType = OPCODETYPE_FUNC2; break;
				case 3: rec->opcodeType = OPCODETYPE_FUNC3; break;
				default: rec->opcodeType = OPCODETYPE_FUNCX; break;
				}
				return rec;
			}
			if (match_parmcnt_pos < 3) match_parmcnt[match_parmcnt_pos++] = (f->nParams&FUNCTIONTYPE_PARAMETERCOUNTMASK);
		}
	}
	if (ctx->last_error_string[0]) lstrcatn(ctx->last_error_string, ", ", sizeof(ctx->last_error_string));
	if (match_parmcnt[3] >= 0)
	{
		if (match_parmcnt_pos < 3) match_parmcnt[match_parmcnt_pos] = match_parmcnt[3];
		match_parmcnt_pos++;
	}
	if (!match_parmcnt_pos)
		snprintf_append(ctx->last_error_string, sizeof(ctx->last_error_string), "'%.30s' undefined", sname);
	else
	{
		int32_t x;
		snprintf_append(ctx->last_error_string, sizeof(ctx->last_error_string), "'%.30s' needs ", sname);
		for (x = 0; x < match_parmcnt_pos; x++)
			snprintf_append(ctx->last_error_string, sizeof(ctx->last_error_string), "%s%d", x == 0 ? "" : x == match_parmcnt_pos - 1 ? " or " : ",", match_parmcnt[x]);
		lstrcatn(ctx->last_error_string, " parms", sizeof(ctx->last_error_string));
	}
	if (errOut) *errOut = match_parmcnt_pos > 0 ? parmcnt < match_parmcnt[0] ? 2 : (match_parmcnt[0] < 2 ? 4 : 1) : 0;
	return NULL;
}
opcodeRec *nseel_setCompiledFunctionCallParameters(compileContext *ctx, opcodeRec *fn, opcodeRec *code1, opcodeRec *code2, opcodeRec *code3, opcodeRec *postCode, int32_t *errOut)
{
	opcodeRec *r;
	int32_t np = 0, x;
	if (!fn || fn->opcodeType != OPCODETYPE_VARPTR || !fn->relname || !fn->relname[0])
	{
		return NULL;
	}
	fn->parms.parms[0] = code1;
	fn->parms.parms[1] = code2;
	fn->parms.parms[2] = code3;
	for (x = 0; x < 3; x++)
	{
		opcodeRec *prni = fn->parms.parms[x];
		while (prni && np < NSEEL_MAX_EELFUNC_PARAMETERS)
		{
			const int32_t isMP = prni->opcodeType == OPCODETYPE_MOREPARAMS;
			np++;
			if (!isMP) break;
			prni = prni->parms.parms[1];
		}
	}
	r = nseel_resolve_named_symbol(ctx, fn, np < 1 ? 1 : np, errOut);
	if (postCode && r)
	{
		if (code1 && r->opcodeType == OPCODETYPE_FUNC1 && r->fntype == FN_WHILE)
		{
			// change while(x) (postcode) to be 
			// while ((x) ? (postcode;1) : 0);
			r->parms.parms[0] =
				nseel_createIfElse(ctx, r->parms.parms[0],
					nseel_createSimpleCompiledFunction(ctx, FN_JOIN_STATEMENTS, 2, postCode, nseel_createCompiledValue(ctx, 1.0f)),
					NULL); // NULL defaults to 0.0f
		}
		else
		{
			snprintf_append(ctx->last_error_string, sizeof(ctx->last_error_string), "syntax error following function");
			*errOut = -1;
			return NULL;
		}
	}
	return r;
}
eelStringSegmentRec *nseel_createStringSegmentRec(compileContext *ctx, const char *str, int32_t len)
{
	eelStringSegmentRec *r = newTmpBlock(ctx, sizeof(eelStringSegmentRec));
	if (r)
	{
		r->_next = 0;
		r->str_start = str;
		r->str_len = len;
	}
	return r;
}
opcodeRec *nseel_eelMakeOpcodeFromStringSegments(compileContext *ctx, eelStringSegmentRec *rec)
{
	if (ctx && ctx->onString)
		return nseel_createCompiledValue(ctx, ctx->onString(ctx->caller_this, rec));
	return NULL;
}
opcodeRec *nseel_createMoreParametersOpcode(compileContext *ctx, opcodeRec *code1, opcodeRec *code2)
{
	opcodeRec *r = code1 && code2 ? newOpCode(ctx, NULL, OPCODETYPE_MOREPARAMS) : NULL;
	if (r)
	{
		r->parms.parms[0] = code1;
		r->parms.parms[1] = code2;
	}
	return r;
}
opcodeRec *nseel_createIfElse(compileContext *ctx, opcodeRec *code1, opcodeRec *code2, opcodeRec *code3)
{
	opcodeRec *r = code1 ? newOpCode(ctx, NULL, OPCODETYPE_FUNC3) : NULL;
	if (r)
	{
		if (!code2) code2 = nseel_createCompiledValue(ctx, 0.0f);
		if (!code3) code3 = nseel_createCompiledValue(ctx, 0.0f);
		if (!code2 || !code3) return NULL;
		r->fntype = FN_IF_ELSE;
		r->parms.parms[0] = code1;
		r->parms.parms[1] = code2;
		r->parms.parms[2] = code3;
	}
	return r;
}
opcodeRec *nseel_createMemoryAccess(compileContext *ctx, opcodeRec *code1, opcodeRec *code2)
{
	if (code2 && (code2->opcodeType != OPCODETYPE_DIRECTVALUE || code2->parms.dv.directValue != 0.0f))
	{
		code1 = nseel_createSimpleCompiledFunction(ctx, FN_ADD, 2, code1, code2);
	}
	return nseel_createSimpleCompiledFunction(ctx, FN_MEMORY, 1, code1, 0);
}
opcodeRec *nseel_createSimpleCompiledFunction(compileContext *ctx, int32_t fn, int32_t np, opcodeRec *code1, opcodeRec *code2)
{
	opcodeRec *r = code1 && (np < 2 || code2) ? newOpCode(ctx, NULL, np >= 2 ? OPCODETYPE_FUNC2 : OPCODETYPE_FUNC1) : NULL;
	if (r)
	{
		r->fntype = fn;
		r->parms.parms[0] = code1;
		r->parms.parms[1] = code2;
		if (fn == FN_JOIN_STATEMENTS)
		{
			r->fn = r; // for joins, fn is temporarily used for tail pointers
			if (code1 && code1->opcodeType == OPCODETYPE_FUNC2 && code1->fntype == fn)
			{
				opcodeRec *t = (opcodeRec *)code1->fn;
				// keep joins in the form of dosomething->morestuff. 
				// in this instance, code1 is previous stuff to do, code2 is new stuff to do
				r->parms.parms[0] = t->parms.parms[1];
				code1->fn = (t->parms.parms[1] = r);
				return code1;
			}
		}
	}
	return r;
}
// these are bitmasks; on request you can tell what is supported, and compileOpcodes will return one of them
#define RETURNVALUE_IGNORE 0 // ignore return value
#define RETURNVALUE_NORMAL 1 // pointer
#define RETURNVALUE_FPSTACK 2
#define RETURNVALUE_BOOL 4 // P1 is nonzero if true
#define RETURNVALUE_BOOL_REVERSED 8 // P1 is zero if true
static int32_t compileOpcodes(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int32_t bufOut_len, int32_t *computTable, const namespaceInformation *namespacePathToThis,
	int32_t supportedReturnValues, int32_t *rvType, int32_t *fpStackUsage, int32_t *canHaveDenormalOutput);
static unsigned char *compileCodeBlockWithRet(compileContext *ctx, opcodeRec *rec, int32_t *computTableSize, const namespaceInformation *namespacePathToThis,
	int32_t supportedReturnValues, int32_t *rvType, int32_t *fpStackUse, int32_t *canHaveDenormalOutput);
_codeHandleFunctionRec *eel_createFunctionNamespacedInstance(compileContext *ctx, _codeHandleFunctionRec *fr, const char *nameptr)
{
	size_t n;
	_codeHandleFunctionRec *subfr =
		fr->isCommonFunction ?
		ctx->isSharedFunctions ? newDataBlock(sizeof(_codeHandleFunctionRec), 8) :
		newCtxDataBlock(sizeof(_codeHandleFunctionRec), 8) :  // if common function, but derived version is in non-common context, set ownership to VM rather than us
		newTmpBlock(ctx, sizeof(_codeHandleFunctionRec));
	if (!subfr) return 0;
	// fr points to functionname()'s rec, nameptr to blah.functionname()
	*subfr = *fr;
	n = strlen(nameptr);
	if (n > sizeof(subfr->fname) - 1) n = sizeof(subfr->fname) - 1;
	memcpy(subfr->fname, nameptr, n);
	subfr->fname[n] = 0;
	subfr->next = NULL;
	subfr->startptr = 0; // make sure this code gets recompiled (with correct member ptrs) for this instance!
	// subfr->derivedCopies already points to the right place
	fr->derivedCopies = subfr;
	return subfr;
}
static void combineNamespaceFields(char *nm, const namespaceInformation *namespaceInfo, const char *relname, int32_t thisctx) // nm must be NSEEL_MAX_VARIABLE_NAMELEN+1 bytes
{
	const char *prefix = namespaceInfo ?
		thisctx < 0 ? (thisctx == -1 ? namespaceInfo->namespacePathToThis : NULL) : (thisctx < MAX_SUB_NAMESPACES ? namespaceInfo->subParmInfo[thisctx] : NULL)
		: NULL;
	int32_t lfp = 0, lrn = relname ? (int32_t)strlen(relname) : 0;
	if (prefix) while (prefix[lfp] && prefix[lfp] != ':' && lfp < NSEEL_MAX_VARIABLE_NAMELEN) lfp++;
	if (!relname) relname = "";
	while (*relname == '.') // if relname begins with ., then remove a chunk of context from prefix
	{
		relname++;
		while (lfp > 0 && prefix[lfp - 1] != '.') lfp--;
		if (lfp > 0) lfp--;
	}
	if (lfp > NSEEL_MAX_VARIABLE_NAMELEN - 3) lfp = NSEEL_MAX_VARIABLE_NAMELEN - 3;
	if (lfp > 0) memcpy(nm, prefix, lfp);
	if (lrn > NSEEL_MAX_VARIABLE_NAMELEN - lfp - (lfp > 0)) lrn = NSEEL_MAX_VARIABLE_NAMELEN - lfp - (lfp > 0);
	if (lrn > 0)
	{
		if (lfp > 0) nm[lfp++] = '.';
		memcpy(nm + lfp, relname, lrn);
		lfp += lrn;
	}
	nm[lfp++] = 0;
}
//---------------------------------------------------------------------------------------------------------------
static void *nseel_getBuiltinFunctionAddress(compileContext *ctx,
	int32_t fntype, void *fn,
	NSEEL_PPPROC *pProc, void ***replList,
	void **endP, int32_t *abiInfo, int32_t preferredReturnValues, const float *hasConstParm1, const float *hasConstParm2)
{
	const float *firstConstParm = hasConstParm1 ? hasConstParm1 : hasConstParm2;
	static void *pow_replptrs[4] = { (void**)&redirect_powf, };
	switch (fntype)
	{
#define RF(x) *endP = nseel_asm_##x##_end; return (void*)nseel_asm_##x
	case FN_MUL_OP:
		*abiInfo = BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(mul_op);
	case FN_DIV_OP:
		*abiInfo = BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(div_op);
	case FN_OR_OP:
		*abiInfo = BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(or_op);
	case FN_XOR_OP:
		*abiInfo = BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(xor_op);
	case FN_AND_OP:
		*abiInfo = BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(and_op);
	case FN_MOD_OP:
		*abiInfo = BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(mod_op);
	case FN_ADD_OP:
		*abiInfo = BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(add_op);
	case FN_SUB_OP:
		*abiInfo = BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(sub_op);
	case FN_POW_OP:
		*abiInfo = BIF_LASTPARMONSTACK | BIF_CLEARDENORMAL;
		*replList = pow_replptrs;
		RF(2pdds);
	case FN_POW:
		*abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK;//BIF_FPSTACKUSE(2) might be safe, need to look at pow()'s implementation, but safer bet is to disallow fp stack caching for this expression
		*replList = pow_replptrs;
		RF(2pdd);
	case FN_ADD:
		*abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK_LAZY | BIF_FPSTACKUSE(2) | BIF_WONTMAKEDENORMAL;
		// for x +- non-denormal-constant,  we can set BIF_CLEARDENORMAL
		if (firstConstParm && fabsf(*firstConstParm) > 1.0e-10) *abiInfo |= BIF_CLEARDENORMAL;
		RF(add);
	case FN_SUB:
		*abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK | BIF_FPSTACKUSE(2) | BIF_WONTMAKEDENORMAL;
		// for x +- non-denormal-constant,  we can set BIF_CLEARDENORMAL
		if (firstConstParm && fabsf(*firstConstParm) > 1.0e-10) *abiInfo |= BIF_CLEARDENORMAL;
		RF(sub);
	case FN_MULTIPLY:
		*abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK_LAZY | BIF_FPSTACKUSE(2);
		// for x*constant-greater-than-eq-1, we can set BIF_WONTMAKEDENORMAL
		if (firstConstParm && fabsf(*firstConstParm) >= 1.0f) *abiInfo |= BIF_WONTMAKEDENORMAL;
		RF(mul);
	case FN_DIVIDE:
		*abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK | BIF_FPSTACKUSE(2);
		// for x/constant-less-than-eq-1, we can set BIF_WONTMAKEDENORMAL
		if (firstConstParm && fabsf(*firstConstParm) <= 1.0f) *abiInfo |= BIF_WONTMAKEDENORMAL;
		RF(div);
	case FN_MOD:
		*abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK | BIF_FPSTACKUSE(1) | BIF_CLEARDENORMAL;
		RF(mod);
	case FN_ASSIGN:
		*abiInfo = BIF_FPSTACKUSE(1) | BIF_CLEARDENORMAL;
		RF(assign);
	case FN_AND: *abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK_LAZY | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL; RF(and);
	case FN_OR: *abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK_LAZY | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL; RF(or );
	case FN_XOR:
		*abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK_LAZY | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(xor);
	case FN_SHR:
		*abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(shr);
	case FN_SHL:
		*abiInfo = BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK | BIF_FPSTACKUSE(2) | BIF_CLEARDENORMAL;
		RF(shl);
	case FN_NOTNOT: *abiInfo = BIF_LASTPARM_ASBOOL | BIF_RETURNSBOOL | BIF_FPSTACKUSE(1); RF(bnotnot);
	case FN_UMINUS: *abiInfo = BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_WONTMAKEDENORMAL; RF(uminus);
	case FN_NOT: *abiInfo = BIF_LASTPARM_ASBOOL | BIF_RETURNSBOOL | BIF_FPSTACKUSE(1); RF(bnot);
	case FN_EQ:
		*abiInfo = BIF_TWOPARMSONFPSTACK_LAZY | BIF_RETURNSBOOL | BIF_FPSTACKUSE(2);
		RF(equal);
	case FN_EQ_EXACT:
		*abiInfo = BIF_TWOPARMSONFPSTACK_LAZY | BIF_RETURNSBOOL | BIF_FPSTACKUSE(2);
		RF(equal_exact);
	case FN_NE:
		*abiInfo = BIF_TWOPARMSONFPSTACK_LAZY | BIF_RETURNSBOOL | BIF_FPSTACKUSE(2);
		RF(notequal);
	case FN_NE_EXACT:
		*abiInfo = BIF_TWOPARMSONFPSTACK_LAZY | BIF_RETURNSBOOL | BIF_FPSTACKUSE(2);
		RF(notequal_exact);
	case FN_LOGICAL_AND:
		*abiInfo = BIF_RETURNSBOOL;
		RF(band);
	case FN_LOGICAL_OR:
		*abiInfo = BIF_RETURNSBOOL;
		RF(bor);
#ifdef GLUE_HAS_FXCH
	case FN_GT:
		*abiInfo = BIF_TWOPARMSONFPSTACK | BIF_RETURNSBOOL | BIF_FPSTACKUSE(2);
		RF(above);
	case FN_GTE:
		*abiInfo = BIF_TWOPARMSONFPSTACK | BIF_RETURNSBOOL | BIF_REVERSEFPORDER | BIF_FPSTACKUSE(2);
		RF(beloweq);
	case FN_LT:
		*abiInfo = BIF_TWOPARMSONFPSTACK | BIF_RETURNSBOOL | BIF_REVERSEFPORDER | BIF_FPSTACKUSE(2);
		RF(above);
	case FN_LTE:
		*abiInfo = BIF_TWOPARMSONFPSTACK | BIF_RETURNSBOOL | BIF_FPSTACKUSE(2);
		RF(beloweq);
#else
	case FN_GT:
		*abiInfo = BIF_RETURNSBOOL | BIF_LASTPARMONSTACK;
		RF(above);
	case FN_GTE:
		*abiInfo = BIF_RETURNSBOOL | BIF_LASTPARMONSTACK;
		RF(aboveeq);
	case FN_LT:
		*abiInfo = BIF_RETURNSBOOL | BIF_LASTPARMONSTACK;
		RF(below);
	case FN_LTE:
		*abiInfo = BIF_RETURNSBOOL | BIF_LASTPARMONSTACK;
		RF(beloweq);
#endif
#undef RF
#define RF(x) *endP = _asm_##x##_end; return (void*)_asm_##x
	case FN_MEMORY:
	{
		//static void *replptrs[4] = { (void**)&__NSEEL_RAMAlloc, };
		//*replList = replptrs;
		*abiInfo = BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(1) | BIF_CLEARDENORMAL;
//#ifdef GLUE_MEM_NEEDS_PPROC
//		*pProc = NSEEL_PProc_RAM;
//#endif
		RF(megabuf);
	}
	break;
#undef RF
	case FUNCTYPE_FUNCTIONTYPEREC:
		if (fn)
		{
			functionType *p = (functionType *)fn;
			// if prefers fpstack or int32_t, or ignoring value, then use fp-stack versions
			if ((preferredReturnValues&(RETURNVALUE_BOOL | RETURNVALUE_FPSTACK)) || !preferredReturnValues)
			{
				static functionType min2 = { "min",    nseel_asm_min_fp,nseel_asm_min_fp_end,   2 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK_LAZY | BIF_FPSTACKUSE(2) | BIF_WONTMAKEDENORMAL };
				static functionType max2 = { "max",    nseel_asm_max_fp,nseel_asm_max_fp_end,   2 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_TWOPARMSONFPSTACK_LAZY | BIF_FPSTACKUSE(2) | BIF_WONTMAKEDENORMAL };
				if (p->afunc == (void*)nseel_asm_min) p = &min2;
				else if (p->afunc == (void*)nseel_asm_max) p = &max2;
			}
			*replList = p->replptrs;
			*pProc = p->pProc;
			*endP = p->func_e;
			*abiInfo = p->nParams & BIF_NPARAMS_MASK;
			if (firstConstParm)
			{
				const char *name = p->name;
				if (!strcmp(name, "min") && *firstConstParm < -1.0e-10) *abiInfo |= BIF_CLEARDENORMAL;
				else if (!strcmp(name, "max") && *firstConstParm > 1.0e-10) *abiInfo |= BIF_CLEARDENORMAL;
			}
			return p->afunc;
		}
		break;
	}
	return 0;
}
static void *nseel_getEELFunctionAddress(compileContext *ctx,
	opcodeRec *op,
	int32_t *customFuncParmSize, int32_t *customFuncLocalStorageSize,
	float ***customFuncLocalStorage, int32_t *computTableTop,
	void **endP, int32_t *isRaw, int32_t wantCodeGenerated,
	const namespaceInformation *namespacePathToThis, int32_t *rvMode, int32_t *fpStackUse, int32_t *canHaveDenormalOutput,
	opcodeRec **ordered_parmptrs, int32_t num_ordered_parmptrs
) // if wantCodeGenerated is false, can return bogus pointers in raw mode
{
	_codeHandleFunctionRec *fn = (_codeHandleFunctionRec*)op->fn;
	namespaceInformation local_namespace = { NULL };
	char prefix_buf[NSEEL_MAX_VARIABLE_NAMELEN + 1], nm[NSEEL_MAX_FUNCSIG_NAME + 1];
	if (!fn) return NULL;
	// op->relname ptr is [whatever.]funcname
	if (fn->parameterAsNamespaceMask || fn->usesNamespaces)
	{
		if (wantCodeGenerated)
		{
			char *p = prefix_buf;
			combineNamespaceFields(nm, namespacePathToThis, op->relname, op->namespaceidx);
			lstrcpyn_safe(prefix_buf, nm, sizeof(prefix_buf));
			local_namespace.namespacePathToThis = prefix_buf;
			// nm is full path of function, prefix_buf will be the path not including function name (unless function name only)
			while (*p) p++;
			while (p >= prefix_buf && *p != '.') p--;
			if (p > prefix_buf) *p = 0;
		}
		if (fn->parameterAsNamespaceMask)
		{
			int32_t x;
			for (x = 0; x < MAX_SUB_NAMESPACES && x < fn->num_params; x++)
			{
				if (fn->parameterAsNamespaceMask & (((uint32_t)1) << x))
				{
					if (wantCodeGenerated)
					{
						const char *rn = NULL;
						char tmp[NSEEL_MAX_VARIABLE_NAMELEN + 1];
						if (x < num_ordered_parmptrs && ordered_parmptrs[x])
						{
							if (ordered_parmptrs[x]->opcodeType == OPCODETYPE_VARPTR)
							{
								rn = ordered_parmptrs[x]->relname;
							}
							else if (ordered_parmptrs[x]->opcodeType == OPCODETYPE_VALUE_FROM_NAMESPACENAME)
							{
								const char *p = ordered_parmptrs[x]->relname;
								if (*p == '#') p++;
								combineNamespaceFields(tmp, namespacePathToThis, p, ordered_parmptrs[x]->namespaceidx);
								rn = tmp;
							}
						}
						if (!rn)
						{
							// todo: figure out how to give correct line number/offset (ugh)
							stbsp_snprintf(ctx->last_error_string, sizeof(ctx->last_error_string), "parameter %d to %s() must be namespace", x + 1, fn->fname);
							return NULL;
						}
						lstrcatn(nm, ":", sizeof(nm));
						local_namespace.subParmInfo[x] = nm + strlen(nm);
						lstrcatn(nm, rn, sizeof(nm));
					}
					ordered_parmptrs[x] = NULL; // prevent caller from bothering generating parameters
				}
			}
		}
		if (wantCodeGenerated)
		{
			_codeHandleFunctionRec *fr = fn;
			// find namespace-adjusted function (if generating code, otherwise assume size is the same)
			fn = 0; // if this gets re-set, it will be the new function
			while (fr && !fn)
			{
				if (!strcmp(fr->fname, nm)) fn = fr;
				fr = fr->derivedCopies;
			}
			if (!fn) // generate copy of function
			{
				fn = eel_createFunctionNamespacedInstance(ctx, (_codeHandleFunctionRec*)op->fn, nm);
			}
		}
	}
	if (!fn) return NULL;
	if (!fn->startptr && fn->opcodes && fn->startptr_size > 0)
	{
		int32_t sz;
		fn->tmpspace_req = 0;
		fn->rvMode = RETURNVALUE_IGNORE;
		fn->canHaveDenormalOutput = 0;
		sz = compileOpcodes(ctx, fn->opcodes, NULL, 128 * 1024 * 1024, &fn->tmpspace_req, wantCodeGenerated ? &local_namespace : NULL, RETURNVALUE_NORMAL | RETURNVALUE_FPSTACK, &fn->rvMode, &fn->fpStackUsage, &fn->canHaveDenormalOutput);
		if (!wantCodeGenerated)
		{
			// don't compile anything for now, just give stats
			if (computTableTop) *computTableTop += fn->tmpspace_req;
			*customFuncParmSize = fn->num_params;
			*customFuncLocalStorage = fn->localstorage;
			*customFuncLocalStorageSize = fn->localstorage_size;
			*rvMode = fn->rvMode;
			*fpStackUse = fn->fpStackUsage;
			if (canHaveDenormalOutput) *canHaveDenormalOutput = fn->canHaveDenormalOutput;
			if (sz <= NSEEL_MAX_FUNCTION_SIZE_FOR_INLINE && !(0 & OPTFLAG_NO_INLINEFUNC))
			{
				*isRaw = 1;
				*endP = ((char *)1) + sz;
				return (char *)1;
			}
			*endP = (void*)nseel_asm_fcall_end;
			return (void*)nseel_asm_fcall;
		}
		if (sz <= NSEEL_MAX_FUNCTION_SIZE_FOR_INLINE && !(0 & OPTFLAG_NO_INLINEFUNC))
		{
			void *p = newTmpBlock(ctx, sz);
			fn->tmpspace_req = 0;
			if (p)
			{
				fn->canHaveDenormalOutput = 0;
				if (fn->isCommonFunction) ctx->isGeneratingCommonFunction++;
				sz = compileOpcodes(ctx, fn->opcodes, (unsigned char*)p, sz, &fn->tmpspace_req, &local_namespace, RETURNVALUE_NORMAL | RETURNVALUE_FPSTACK, &fn->rvMode, &fn->fpStackUsage, &fn->canHaveDenormalOutput);
				if (fn->isCommonFunction) ctx->isGeneratingCommonFunction--;
				// recompile function with native context pointers
				if (sz > 0)
				{
					fn->startptr_size = sz;
					fn->startptr = p;
				}
			}
		}
		else
		{
			unsigned char *codeCall;
			fn->tmpspace_req = 0;
			fn->fpStackUsage = 0;
			fn->canHaveDenormalOutput = 0;
			if (fn->isCommonFunction) ctx->isGeneratingCommonFunction++;
			codeCall = compileCodeBlockWithRet(ctx, fn->opcodes, &fn->tmpspace_req, &local_namespace, RETURNVALUE_NORMAL | RETURNVALUE_FPSTACK, &fn->rvMode, &fn->fpStackUsage, &fn->canHaveDenormalOutput);
			if (fn->isCommonFunction) ctx->isGeneratingCommonFunction--;
			if (codeCall)
			{
				void *f = GLUE_realAddress(nseel_asm_fcall, nseel_asm_fcall_end, &sz);
				fn->startptr = newTmpBlock(ctx, sz);
				if (fn->startptr)
				{
					memcpy(fn->startptr, f, sz);
					EEL_GLUE_set_immediate(fn->startptr, (INT_PTR)codeCall);
					fn->startptr_size = sz;
				}
			}
		}
	}
	if (fn->startptr)
	{
		if (computTableTop) *computTableTop += fn->tmpspace_req;
		*customFuncParmSize = fn->num_params;
		*customFuncLocalStorage = fn->localstorage;
		*customFuncLocalStorageSize = fn->localstorage_size;
		*rvMode = fn->rvMode;
		*fpStackUse = fn->fpStackUsage;
		if (canHaveDenormalOutput) *canHaveDenormalOutput = fn->canHaveDenormalOutput;
		*endP = (char*)fn->startptr + fn->startptr_size;
		*isRaw = 1;
		return fn->startptr;
	}
	return 0;
}
// returns true if does something (other than calculating and throwing away a value)
static char optimizeOpcodes(compileContext *ctx, opcodeRec *op, int32_t needsResult)
{
	opcodeRec *lastJoinOp = NULL;
	char retv, retv_parm[3], joined_retv = 0;
	while (op && op->opcodeType == OPCODETYPE_FUNC2 && op->fntype == FN_JOIN_STATEMENTS)
	{
		if (!optimizeOpcodes(ctx, op->parms.parms[0], 0) || OPCODE_IS_TRIVIAL(op->parms.parms[0]))
		{
			// direct value, can skip ourselves
			memcpy(op, op->parms.parms[1], sizeof(*op));
		}
		else
		{
			joined_retv |= 1;
			lastJoinOp = op;
			op = op->parms.parms[1];
		}
	}
	goto start_over;
#define RESTART_DIRECTVALUE(X) { op->parms.dv.directValue = (X); goto start_over_directvalue; }
start_over_directvalue:
	op->opcodeType = OPCODETYPE_DIRECTVALUE;
	op->parms.dv.valuePtr = NULL;
start_over: // when an opcode changed substantially in optimization, goto here to reprocess it
	retv = retv_parm[0] = retv_parm[1] = retv_parm[2] = 0;
	if (!op || // should never really happen
		OPCODE_IS_TRIVIAL(op) || // should happen often (vars)
		op->opcodeType < 0 || op->opcodeType >= OPCODETYPE_INVALID // should never happen (assert would be appropriate heh)
		) return joined_retv;
	if (!needsResult)
	{
		if (op->fntype == FUNCTYPE_EELFUNC)
		{
			needsResult = 1; // assume eel functions are non-const for now
		}
		else if (op->fntype == FUNCTYPE_FUNCTIONTYPEREC)
		{
			functionType  *pfn = (functionType *)op->fn;
			if (!pfn || !(pfn->nParams&NSEEL_NPARAMS_FLAG_CONST)) needsResult = 1;
		}
		else if (op->fntype >= FN_NONCONST_BEGIN && op->fntype < FUNCTYPE_SIMPLEMAX)
		{
			needsResult = 1;
		}
	}
	if (op->opcodeType >= OPCODETYPE_FUNC2) retv_parm[1] = optimizeOpcodes(ctx, op->parms.parms[1], needsResult);
	if (op->opcodeType >= OPCODETYPE_FUNC3) retv_parm[2] = optimizeOpcodes(ctx, op->parms.parms[2], needsResult);
	retv_parm[0] = optimizeOpcodes(ctx, op->parms.parms[0], needsResult ||
		(FNPTR_HAS_CONDITIONAL_EXEC(op) && (retv_parm[1] || retv_parm[2] || op->opcodeType <= OPCODETYPE_FUNC1)));
	if (op->opcodeType != OPCODETYPE_MOREPARAMS)
	{
		if (op->fntype >= 0 && op->fntype < FUNCTYPE_SIMPLEMAX)
		{
			if (op->opcodeType == OPCODETYPE_FUNC1) // within FUNCTYPE_SIMPLE
			{
				if (op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE)
				{
					switch (op->fntype)
					{
					case FN_NOTNOT: RESTART_DIRECTVALUE(fabsf(op->parms.parms[0]->parms.dv.directValue) >= NSEEL_CLOSEFACTOR ? 1.0f : 0.0f);
					case FN_NOT:    RESTART_DIRECTVALUE(fabsf(op->parms.parms[0]->parms.dv.directValue) >= NSEEL_CLOSEFACTOR ? 0.0f : 1.0f);
					case FN_UMINUS: RESTART_DIRECTVALUE(-op->parms.parms[0]->parms.dv.directValue);
					}
				}
				else if (op->fntype == FN_NOT || op->fntype == FN_NOTNOT)
				{
					if (op->parms.parms[0]->opcodeType == OPCODETYPE_FUNC1)
					{
						switch (op->parms.parms[0]->fntype)
						{
						case FN_UMINUS:
						case FN_NOTNOT: // ignore any NOTNOTs UMINUS or UPLUS, they would have no effect anyway
							op->parms.parms[0] = op->parms.parms[0]->parms.parms[0];
							goto start_over;
						case FN_NOT:
							op->fntype = op->fntype == FN_NOT ? FN_NOTNOT : FN_NOT; // switch between FN_NOT and FN_NOTNOT
							op->parms.parms[0] = op->parms.parms[0]->parms.parms[0];
							goto start_over;
						}
					}
					else if (op->parms.parms[0]->opcodeType == OPCODETYPE_FUNC2)
					{
						int32_t repl_type = -1;
						switch (op->parms.parms[0]->fntype)
						{
						case FN_EQ: repl_type = FN_NE; break;
						case FN_NE: repl_type = FN_EQ; break;
						case FN_EQ_EXACT: repl_type = FN_NE_EXACT; break;
						case FN_NE_EXACT: repl_type = FN_EQ_EXACT; break;
						case FN_LT:  repl_type = FN_GTE; break;
						case FN_LTE: repl_type = FN_GT; break;
						case FN_GT:  repl_type = FN_LTE; break;
						case FN_GTE: repl_type = FN_LT; break;
						}
						if (repl_type != -1)
						{
							const int32_t oldtype = op->fntype;
							memcpy(op, op->parms.parms[0], sizeof(*op));
							if (oldtype == FN_NOT) op->fntype = repl_type;
							goto start_over;
						}
					}
				}
			}
			else if (op->opcodeType == OPCODETYPE_FUNC2)  // within FUNCTYPE_SIMPLE
			{
				const int32_t dv0 = op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE;
				const int32_t dv1 = op->parms.parms[1]->opcodeType == OPCODETYPE_DIRECTVALUE;
				if (dv0 && dv1)
				{
					int32_t reval = -1;
					switch (op->fntype)
					{
					case FN_MOD:
					{
						int32_t a = (int32_t)op->parms.parms[1]->parms.dv.directValue;
						if (a)
						{
							a = (int32_t)op->parms.parms[0]->parms.dv.directValue % a;
							if (a < 0) a = -a;
						}
						RESTART_DIRECTVALUE((float)a);
					}
					break;
					case FN_SHL:      RESTART_DIRECTVALUE(((int32_t)op->parms.parms[0]->parms.dv.directValue) << ((int32_t)op->parms.parms[1]->parms.dv.directValue));
					case FN_SHR:      RESTART_DIRECTVALUE(((int32_t)op->parms.parms[0]->parms.dv.directValue) >> ((int32_t)op->parms.parms[1]->parms.dv.directValue));
					case FN_POW:      RESTART_DIRECTVALUE(powf(op->parms.parms[0]->parms.dv.directValue, op->parms.parms[1]->parms.dv.directValue));
					case FN_DIVIDE:   RESTART_DIRECTVALUE(op->parms.parms[0]->parms.dv.directValue / op->parms.parms[1]->parms.dv.directValue);
					case FN_MULTIPLY: RESTART_DIRECTVALUE(op->parms.parms[0]->parms.dv.directValue * op->parms.parms[1]->parms.dv.directValue);
					case FN_ADD:      RESTART_DIRECTVALUE(op->parms.parms[0]->parms.dv.directValue + op->parms.parms[1]->parms.dv.directValue);
					case FN_SUB:      RESTART_DIRECTVALUE(op->parms.parms[0]->parms.dv.directValue - op->parms.parms[1]->parms.dv.directValue);
					case FN_AND:      RESTART_DIRECTVALUE((float)(((WDL_INT64)op->parms.parms[0]->parms.dv.directValue) & ((WDL_INT64)op->parms.parms[1]->parms.dv.directValue)));
					case FN_OR:       RESTART_DIRECTVALUE((float)(((WDL_INT64)op->parms.parms[0]->parms.dv.directValue) | ((WDL_INT64)op->parms.parms[1]->parms.dv.directValue)));
					case FN_XOR:      RESTART_DIRECTVALUE((float)(((WDL_INT64)op->parms.parms[0]->parms.dv.directValue) ^ ((WDL_INT64)op->parms.parms[1]->parms.dv.directValue)));
					case FN_EQ:       reval = fabsf(op->parms.parms[0]->parms.dv.directValue - op->parms.parms[1]->parms.dv.directValue) < NSEEL_CLOSEFACTOR; break;
					case FN_NE:       reval = fabsf(op->parms.parms[0]->parms.dv.directValue - op->parms.parms[1]->parms.dv.directValue) >= NSEEL_CLOSEFACTOR; break;
					case FN_EQ_EXACT: reval = op->parms.parms[0]->parms.dv.directValue == op->parms.parms[1]->parms.dv.directValue; break;
					case FN_NE_EXACT: reval = op->parms.parms[0]->parms.dv.directValue != op->parms.parms[1]->parms.dv.directValue; break;
					case FN_LT:       reval = op->parms.parms[0]->parms.dv.directValue < op->parms.parms[1]->parms.dv.directValue; break;
					case FN_LTE:      reval = op->parms.parms[0]->parms.dv.directValue <= op->parms.parms[1]->parms.dv.directValue; break;
					case FN_GT:       reval = op->parms.parms[0]->parms.dv.directValue > op->parms.parms[1]->parms.dv.directValue; break;
					case FN_GTE:      reval = op->parms.parms[0]->parms.dv.directValue >= op->parms.parms[1]->parms.dv.directValue; break;
					case FN_LOGICAL_AND: reval = fabsf(op->parms.parms[0]->parms.dv.directValue) >= NSEEL_CLOSEFACTOR && fabsf(op->parms.parms[1]->parms.dv.directValue) >= NSEEL_CLOSEFACTOR; break;
					case FN_LOGICAL_OR:  reval = fabsf(op->parms.parms[0]->parms.dv.directValue) >= NSEEL_CLOSEFACTOR || fabsf(op->parms.parms[1]->parms.dv.directValue) >= NSEEL_CLOSEFACTOR; break;
					}
					if (reval >= 0) RESTART_DIRECTVALUE((float)reval);
				}
				else if (dv0 || dv1)
				{
					float dvalue = op->parms.parms[!dv0]->parms.dv.directValue;
					switch (op->fntype)
					{
					case FN_OR:
					case FN_XOR:
						if (!(WDL_INT64)dvalue)
						{
							// replace with or0
							static functionType fr = { "or0",nseel_asm_or0, nseel_asm_or0_end, 1 | NSEEL_NPARAMS_FLAG_CONST | BIF_LASTPARMONSTACK | BIF_RETURNSONSTACK | BIF_CLEARDENORMAL, {0}, NULL };
							op->opcodeType = OPCODETYPE_FUNC1;
							op->fntype = FUNCTYPE_FUNCTIONTYPEREC;
							op->fn = &fr;
							if (dv0) op->parms.parms[0] = op->parms.parms[1];
							goto start_over;
						}
						break;
					case FN_SUB:
						if (dv0)
						{
							if (dvalue == 0.0f)
							{
								op->opcodeType = OPCODETYPE_FUNC1;
								op->fntype = FN_UMINUS;
								op->parms.parms[0] = op->parms.parms[1];
								goto start_over;
							}
							break;
						}
						// fall through, if dv1 we can remove +0.0f
					case FN_ADD:
						if (dvalue == 0.0f)
						{
							memcpy(op, op->parms.parms[!!dv0], sizeof(*op));
							goto start_over;
						}
						break;
					case FN_AND:
						if ((WDL_INT64)dvalue) break;
						dvalue = 0.0f; // treat x&0 as x*0, which optimizes to 0
						// fall through
					case FN_MULTIPLY:
						if (dvalue == 0.0f) // remove multiply by 0.0f (using 0.0f direct value as replacement), unless the nonzero side did something
						{
							if (!retv_parm[!!dv0])
							{
								memcpy(op, op->parms.parms[!dv0], sizeof(*op)); // set to 0 if other action wouldn't do anything
								goto start_over;
							}
							else
							{
								// this is 0.0f * oldexpressionthatmustbeprocessed or oldexpressionthatmustbeprocessed*0.0f
								op->fntype = FN_JOIN_STATEMENTS;
								if (dv0) // 0.0f*oldexpression, reverse the order so that 0 is returned
								{
									// set to (oldexpression;0)
									opcodeRec *tmp = op->parms.parms[1];
									op->parms.parms[1] = op->parms.parms[0];
									op->parms.parms[0] = tmp;
								}
								goto start_over;
							}
						}
						else if (dvalue == 1.0f) // remove multiply by 1.0f (using non-1.0f value as replacement)
						{
							memcpy(op, op->parms.parms[!!dv0], sizeof(*op));
							goto start_over;
						}
						break;
					case FN_POW:
						if (dv1)
						{
							// x^0 = 1
							if (fabsf(dvalue) < 1e-15f)
							{
								RESTART_DIRECTVALUE(1.0f);
							}
							// x^1 = x
							if (fabsf(dvalue - 1.0f) < 1e-15f)
							{
								memcpy(op, op->parms.parms[0], sizeof(*op));
								goto start_over;
							}
						}
						else if (dv0)
						{
							// pow(constant, x) = exp((x) * ln(constant)), if constant>0
							// opcodeRec *parm0 = op->parms.parms[0];
							if (dvalue > 0.0f)
							{
								static functionType expcpy = { "exp",    nseel_asm_1pdd,nseel_asm_1pdd_end,   1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK, {(void**)&exp}, };
								// 1^x = 1
								if (fabsf(dvalue - 1.0f) < 1e-15f)
								{
									RESTART_DIRECTVALUE(1.0f);
								}
								dvalue = logf(dvalue);
								if (fabsf(dvalue - 1.0f) < FLT_EPSILON)
								{
									// caller wanted e^x
									op->parms.parms[0] = op->parms.parms[1];
								}
								else
								{
									// it would be nice to replace 10^x with exp(log(10)*x) or 2^x with exp(log(2),x), but 
									// doing so breaks rounding. we could maybe only allow 10^x, which is used for dB conversion,
									// but for now we should just force the programmer do it exp(log(10)*x) themselves.
									break;
									/*
									parm0->opcodeType = OPCODETYPE_FUNC2;
									parm0->fntype = FN_MULTIPLY;
									parm0->parms.parms[0] = nseel_createCompiledValue(ctx,dvalue);
									parm0->parms.parms[1] = op->parms.parms[1];
									*/
								}
								op->opcodeType = OPCODETYPE_FUNC1;
								op->fntype = FUNCTYPE_FUNCTIONTYPEREC;
								op->fn = &expcpy;
								goto start_over;
							}
						}
						break;
					case FN_MOD:
						if (dv1)
						{
							const int32_t a = (int32_t)dvalue;
							if (!a)
							{
								RESTART_DIRECTVALUE(0.0f);
							}
						}
						break;
					case FN_DIVIDE:
						if (dv1)
						{
							if (dvalue == 1.0f)  // remove divide by 1.0f  (using non-1.0f value as replacement)
							{
								memcpy(op, op->parms.parms[!!dv0], sizeof(*op));
								goto start_over;
							}
							else
							{
								// change to a multiply
								if (dvalue == 0.0f)
								{
									op->fntype = FN_MULTIPLY;
									goto start_over;
								}
								else
								{
									op->fntype = FN_MULTIPLY;
									op->parms.parms[1]->parms.dv.directValue = 1.0f / dvalue;
									op->parms.parms[1]->parms.dv.valuePtr = NULL;
									goto start_over;
								}
							}
						}
						else if (dvalue == 0.0f)
						{
							if (!retv_parm[!!dv0])
							{
								// if 0/x set to always 0.
								// this is 0.0f / (oldexpression that can be eliminated)
								memcpy(op, op->parms.parms[!dv0], sizeof(*op)); // set to 0 if other action wouldn't do anything
							}
							else
							{
								opcodeRec *tmp;
								// this is 0.0f / oldexpressionthatmustbeprocessed
								op->fntype = FN_JOIN_STATEMENTS;
								tmp = op->parms.parms[1];
								op->parms.parms[1] = op->parms.parms[0];
								op->parms.parms[0] = tmp;
								// set to (oldexpression;0)
							}
							goto start_over;
						}
						break;
					case FN_EQ:
						if (dvalue == 0.0f)
						{
							// convert x == 0.0f to !x
							op->opcodeType = OPCODETYPE_FUNC1;
							op->fntype = FN_NOT;
							if (dv0) op->parms.parms[0] = op->parms.parms[1];
							goto start_over;
						}
						break;
					case FN_NE:
						if (dvalue == 0.0f)
						{
							// convert x != 0.0f to !!
							op->opcodeType = OPCODETYPE_FUNC1;
							op->fntype = FN_NOTNOT;
							if (dv0) op->parms.parms[0] = op->parms.parms[1];
							goto start_over;
						}
						break;
					case FN_LOGICAL_AND:
						if (dv0)
						{
							// dvalue && expr
							if (fabsf(dvalue) < NSEEL_CLOSEFACTOR)
							{
								// 0 && expr, replace with 0
								RESTART_DIRECTVALUE(0.0f);
							}
							else
							{
								// 1 && expr, replace with 0 != expr
								op->fntype = FN_NE;
								op->parms.parms[0]->parms.dv.valuePtr = NULL;
								op->parms.parms[0]->parms.dv.directValue = 0.0f;
							}
						}
						else
						{
							// expr && dvalue
							if (fabsf(dvalue) < NSEEL_CLOSEFACTOR)
							{
								// expr && 0
								if (!retv_parm[0])
								{
									// expr has no consequence, drop it
									RESTART_DIRECTVALUE(0.0f);
								}
								else
								{
									// replace with (expr; 0)
									op->fntype = FN_JOIN_STATEMENTS;
									op->parms.parms[1]->parms.dv.valuePtr = NULL;
									op->parms.parms[1]->parms.dv.directValue = 0.0f;
								}
							}
							else
							{
								// expr && 1, replace with expr != 0
								op->fntype = FN_NE;
								op->parms.parms[1]->parms.dv.valuePtr = NULL;
								op->parms.parms[1]->parms.dv.directValue = 0.0f;
							}
						}
						goto start_over;
					case FN_LOGICAL_OR:
						if (dv0)
						{
							// dvalue || expr
							if (fabsf(dvalue) >= NSEEL_CLOSEFACTOR)
							{
								// 1 || expr, replace with 1
								RESTART_DIRECTVALUE(1.0f);
							}
							else
							{
								// 0 || expr, replace with 0 != expr
								op->fntype = FN_NE;
								op->parms.parms[0]->parms.dv.valuePtr = NULL;
								op->parms.parms[0]->parms.dv.directValue = 0.0f;
							}
						}
						else
						{
							// expr || dvalue
							if (fabsf(dvalue) >= NSEEL_CLOSEFACTOR)
							{
								// expr || 1
								if (!retv_parm[0])
								{
									// expr has no consequence, drop it and return 1
									RESTART_DIRECTVALUE(1.0f);
								}
								else
								{
									// replace with (expr; 1)
									op->fntype = FN_JOIN_STATEMENTS;
									op->parms.parms[1]->parms.dv.valuePtr = NULL;
									op->parms.parms[1]->parms.dv.directValue = 1.0f;
								}
							}
							else
							{
								// expr || 0, replace with expr != 0
								op->fntype = FN_NE;
								op->parms.parms[1]->parms.dv.valuePtr = NULL;
								op->parms.parms[1]->parms.dv.directValue = 0.0f;
							}
						}
						goto start_over;
					}
				} // dv0 || dv1
				// general optimization of two parameters
				switch (op->fntype)
				{
				case FN_MULTIPLY:
				{
					opcodeRec *first_parm = op->parms.parms[0], *second_parm = op->parms.parms[1];
					if (second_parm->opcodeType == first_parm->opcodeType)
					{
						switch (first_parm->opcodeType)
						{
						case OPCODETYPE_VALUE_FROM_NAMESPACENAME:
							if (first_parm->namespaceidx != second_parm->namespaceidx) break;
							// fall through
						case OPCODETYPE_VARPTR:
							if (first_parm->relname && second_parm->relname && !strcmp(second_parm->relname, first_parm->relname)) second_parm = NULL;
							break;
						case OPCODETYPE_VARPTRPTR:
							if (first_parm->parms.dv.valuePtr && first_parm->parms.dv.valuePtr == second_parm->parms.dv.valuePtr) second_parm = NULL;
							break;
						}
						if (!second_parm) // switch from x*x to sqr(x)
						{
							static functionType sqrcpy = { "sqr",    nseel_asm_sqr,nseel_asm_sqr_end,   1 | NSEEL_NPARAMS_FLAG_CONST | BIF_RETURNSONSTACK | BIF_LASTPARMONSTACK | BIF_FPSTACKUSE(1) };
							op->opcodeType = OPCODETYPE_FUNC1;
							op->fntype = FUNCTYPE_FUNCTIONTYPEREC;
							op->fn = &sqrcpy;
							goto start_over;
						}
					}
				}
				break;
				case FN_POW:
				{
					opcodeRec *first_parm = op->parms.parms[0];
					if (first_parm->opcodeType == op->opcodeType && first_parm->fntype == FN_POW)
					{
						// since first_parm is a pow too, we can multiply the exponents.
						// set our base to be the base of the inner pow
						op->parms.parms[0] = first_parm->parms.parms[0];
						// make the old extra pow be a multiply of the exponents
						first_parm->fntype = FN_MULTIPLY;
						first_parm->parms.parms[0] = op->parms.parms[1];
						// put that as the exponent
						op->parms.parms[1] = first_parm;
						goto start_over;
					}
				}
				break;
				case FN_LOGICAL_AND:
				case FN_LOGICAL_OR:
					if (op->parms.parms[0]->fntype == FN_NOTNOT)
					{
						// remove notnot, unnecessary for input to &&/|| operators
						op->parms.parms[0] = op->parms.parms[0]->parms.parms[0];
						goto start_over;
					}
					if (op->parms.parms[1]->fntype == FN_NOTNOT)
					{
						// remove notnot, unnecessary for input to &&/|| operators
						op->parms.parms[1] = op->parms.parms[1]->parms.parms[0];
						goto start_over;
					}
					break;
				}
			}
			else if (op->opcodeType == OPCODETYPE_FUNC3)  // within FUNCTYPE_SIMPLE
			{
				if (op->fntype == FN_IF_ELSE)
				{
					if (op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE)
					{
						int32_t s = fabsf(op->parms.parms[0]->parms.dv.directValue) >= NSEEL_CLOSEFACTOR;
						memcpy(op, op->parms.parms[s ? 1 : 2], sizeof(opcodeRec));
						goto start_over;
					}
					if (op->parms.parms[0]->opcodeType == OPCODETYPE_FUNC1)
					{
						if (op->parms.parms[0]->fntype == FN_NOTNOT)
						{
							// remove notnot, unnecessary for input to ? operator
							op->parms.parms[0] = op->parms.parms[0]->parms.parms[0];
							goto start_over;
						}
					}
				}
			}
			if (op->fntype >= FN_NONCONST_BEGIN && op->fntype < FUNCTYPE_SIMPLEMAX) retv |= 1;
			// FUNCTYPE_SIMPLE
		}
		else if (op->fntype == FUNCTYPE_FUNCTIONTYPEREC && op->fn)
		{
			/*
			probably worth doing reduction on:
			_divop (constant change to multiply)
			_and
			_or
			abs
			maybe:
			min
			max
			also, optimize should (recursively or maybe iteratively?) search transitive functions (mul/div) for more constant reduction possibilities
			*/
			functionType  *pfn = (functionType *)op->fn;
			if (!(pfn->nParams&NSEEL_NPARAMS_FLAG_CONST)) retv |= 1;
			if (op->opcodeType == OPCODETYPE_FUNC1) // within FUNCTYPE_FUNCTIONTYPEREC
			{
				if (op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE)
				{
					int32_t suc = 1;
					float v = op->parms.parms[0]->parms.dv.directValue;
#define DOF(x) if (!strcmp(pfn->name,#x)) v = x(v); else
#define DOF2(x,y) if (!strcmp(pfn->name,#x)) v = x(y); else
					DOF(sinf)
						DOF(cosf)
						DOF(tanf)
						DOF(asinf)
						DOF(acosf)
						DOF(atanf)
						DOF2(sqrtf, fabsf(v))
						DOF(expf)
						DOF(logf)
						DOF(log10f)
						/* else */ suc = 0;
#undef DOF
#undef DOF2
					if (suc)
					{
						RESTART_DIRECTVALUE(v);
					}
				}
			}
			else if (op->opcodeType == OPCODETYPE_FUNC2)  // within FUNCTYPE_FUNCTIONTYPEREC
			{
				const int32_t dv0 = op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE;
				const int32_t dv1 = op->parms.parms[1]->opcodeType == OPCODETYPE_DIRECTVALUE;
				if (dv0 && dv1)
				{
					if (!strcmp(pfn->name, "atan2"))
					{
						RESTART_DIRECTVALUE(atan2f(op->parms.parms[0]->parms.dv.directValue, op->parms.parms[1]->parms.dv.directValue));
					}
				}
			}
			// FUNCTYPE_FUNCTIONTYPEREC
		}
		else
		{
			// unknown or eel func, assume non-const
			retv |= 1;
		}
	}
	// if we need results, or our function has effects itself, then finish
	if (retv || needsResult)
	{
		return retv || joined_retv || retv_parm[0] || retv_parm[1] || retv_parm[2];
	}
	// we don't need results here, and our function is const, which means we can remove it
	{
		int32_t cnt = 0, idx1 = 0, idx2 = 0, x;
		for (x = 0; x < 3; x++) if (retv_parm[x]) { if (!cnt++) idx1 = x; else idx2 = x; }
		if (!cnt) // none of the parameters do anything, remove this opcode
		{
			if (lastJoinOp)
			{
				// replace previous join with its first linked opcode, removing this opcode completely
				memcpy(lastJoinOp, lastJoinOp->parms.parms[0], sizeof(*lastJoinOp));
			}
			else if (op->opcodeType != OPCODETYPE_DIRECTVALUE)
			{
				// allow caller to easily detect this as trivial and remove it
				op->opcodeType = OPCODETYPE_DIRECTVALUE;
				op->parms.dv.valuePtr = NULL;
				op->parms.dv.directValue = 0.0f;
			}
			// return joined_retv below
		}
		else
		{
			// if parameters are non-const, and we're a conditional, preserve our function
			if (FNPTR_HAS_CONDITIONAL_EXEC(op)) return 1;
			// otherwise, condense into either the non-const statement, or a join
			if (cnt == 1)
			{
				memcpy(op, op->parms.parms[idx1], sizeof(*op));
			}
			else if (cnt == 2)
			{
				op->opcodeType = OPCODETYPE_FUNC2;
				op->fntype = FN_JOIN_STATEMENTS;
				op->fn = op;
				op->parms.parms[0] = op->parms.parms[idx1];
				op->parms.parms[1] = op->parms.parms[idx2];
				op->parms.parms[2] = NULL;
			}
			else
			{
				// todo need to create a new opcodeRec here, for now just leave as is 
				// (non-conditional const 3 parameter functions are rare anyway)
			}
			return 1;
		}
	}
	return joined_retv;
}
static int32_t generateValueToReg(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int32_t whichReg, const namespaceInformation *functionPrefix, int32_t allowCache)
{
	float *b = NULL;
	if (op->opcodeType == OPCODETYPE_VALUE_FROM_NAMESPACENAME)
	{
		char nm[NSEEL_MAX_VARIABLE_NAMELEN + 1];
		const char *p = op->relname;
		combineNamespaceFields(nm, functionPrefix, p + (*p == '#'), op->namespaceidx);
		if (!nm[0]) return -1;
		b = nseel_int_register_var(ctx, nm, 0, NULL);
		if (!b) RET_MINUS1_FAIL("error registering var")
	}
	else
	{
		if (op->opcodeType != OPCODETYPE_DIRECTVALUE) allowCache = 0;
		b = op->parms.dv.valuePtr;
		if (!b && op->opcodeType == OPCODETYPE_VARPTR && op->relname && op->relname[0])
		{
			op->parms.dv.valuePtr = b = nseel_int_register_var(ctx, op->relname, 0, NULL);
		}
		if (b && op->opcodeType == OPCODETYPE_VARPTRPTR) b = *(float **)b;
		if (!b && allowCache)
		{
			int32_t n = 50; // only scan last X items
			opcodeRec *r = ctx->directValueCache;
			while (r && n--)
			{
				if (r->parms.dv.directValue == op->parms.dv.directValue && (b = r->parms.dv.valuePtr)) break;
				r = (opcodeRec*)r->fn;
			}
		}
		if (!b)
		{
			ctx->l_stats[3]++;
			if (ctx->isGeneratingCommonFunction)
				b = newCtxDataBlock(sizeof(float), sizeof(float));
			else
				b = newDataBlock(sizeof(float), sizeof(float));
			if (!b) RET_MINUS1_FAIL("error allocating data block")
				if (op->opcodeType != OPCODETYPE_VARPTRPTR) op->parms.dv.valuePtr = b;
			*b = op->parms.dv.directValue;
			if (allowCache)
			{
				op->fn = ctx->directValueCache;
				ctx->directValueCache = op;
			}
		}
	}
	GLUE_MOV_PX_DIRECTVALUE_GEN(bufOut, (INT_PTR)b, whichReg);
	return GLUE_MOV_PX_DIRECTVALUE_SIZE;
}
unsigned char *compileCodeBlockWithRet(compileContext *ctx, opcodeRec *rec, int32_t *computTableSize, const namespaceInformation *namespacePathToThis,
	int32_t supportedReturnValues, int32_t *rvType, int32_t *fpStackUsage, int32_t *canHaveDenormalOutput)
{
	unsigned char *p, *newblock2;
	// generate code call
	int32_t funcsz = compileOpcodes(ctx, rec, NULL, 1024 * 1024 * 128, NULL, namespacePathToThis, supportedReturnValues, rvType, fpStackUsage, NULL);
	if (funcsz < 0) return NULL;
	p = newblock2 = newCodeBlock(funcsz + sizeof(GLUE_RET), 32);
	if (!newblock2) return NULL;
	*fpStackUsage = 0;
	funcsz = compileOpcodes(ctx, rec, p, funcsz, computTableSize, namespacePathToThis, supportedReturnValues, rvType, fpStackUsage, canHaveDenormalOutput);
	if (funcsz < 0) return NULL;
	p += funcsz;
	memcpy(p, &GLUE_RET, sizeof(GLUE_RET)); p += sizeof(GLUE_RET);
#ifdef __arm__
	__clear_cache(newblock2, p);
#endif
	ctx->l_stats[2] += funcsz + 2;
	return newblock2;
}
static int32_t compileNativeFunctionCall(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int32_t bufOut_len, int32_t *computTableSize, const namespaceInformation *namespacePathToThis,
	int32_t *rvMode, int32_t *fpStackUsage, int32_t preferredReturnValues, int32_t *canHaveDenormalOutput)
{
	// builtin function generation
	int32_t func_size = 0;
	int32_t cfunc_abiinfo = 0;
	int32_t local_fpstack_use = 0; // how many items we have pushed onto the fp stack
	int32_t parm_size = 0;
	int32_t restore_stack_amt = 0;
	void *func_e = NULL;
	NSEEL_PPPROC preProc = 0;
	void **repl = NULL;
	int32_t n_params = 1 + op->opcodeType - OPCODETYPE_FUNC1;
	const int32_t parm0_dv = op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE;
	const int32_t parm1_dv = n_params > 1 && op->parms.parms[1]->opcodeType == OPCODETYPE_DIRECTVALUE;
	void *func = nseel_getBuiltinFunctionAddress(ctx, op->fntype, op->fn, &preProc, &repl, &func_e, &cfunc_abiinfo, preferredReturnValues,
		parm0_dv ? &op->parms.parms[0]->parms.dv.directValue : NULL,
		parm1_dv ? &op->parms.parms[1]->parms.dv.directValue : NULL
	);
	if (!func) RET_MINUS1_FAIL("error getting funcaddr")
		*fpStackUsage = BIF_GETFPSTACKUSE(cfunc_abiinfo);
	*rvMode = RETURNVALUE_NORMAL;
	if (cfunc_abiinfo & BIF_TAKES_VARPARM)
	{
#if defined(__arm__) || defined(__ppc__) || (defined (_M_ARM) && _M_ARM == 7)
		const int32_t max_params = 4096; // 32kb max offset addressing for stack, so 4096*4 = 16384, should be safe
#else
		const int32_t max_params = 32768; // sanity check, the stack is free to grow on x86/x86-64
#endif
		int32_t x;
		// this mode is less efficient in that it creates a list of pointers on the stack to pass to the function
		// but it is more flexible and works for >3 parameters.
		if (op->opcodeType == OPCODETYPE_FUNCX)
		{
			n_params = 0;
			for (x = 0; x < 3; x++)
			{
				opcodeRec *prni = op->parms.parms[x];
				while (prni)
				{
					const int32_t isMP = prni->opcodeType == OPCODETYPE_MOREPARAMS;
					n_params++;
					if (!isMP || n_params >= max_params) break;
					prni = prni->parms.parms[1];
				}
			}
		}
		restore_stack_amt = (sizeof(void *) * n_params + 15)&~15;
		if (restore_stack_amt)
		{
			int32_t offs = restore_stack_amt;
			while (offs > 0)
			{
				int32_t amt = offs;
				if (amt > 4096) amt = 4096;
				if ((size_t)bufOut_len < ((size_t)(parm_size)+GLUE_MOVE_STACK_SIZE)) RET_MINUS1_FAIL("insufficient size for varparm")
					if (bufOut) GLUE_MOVE_STACK(bufOut + parm_size, -amt);
				parm_size += GLUE_MOVE_STACK_SIZE;
				offs -= amt;
				if (offs > 0) // make sure this page is in memory
				{
					if ((size_t)bufOut_len < ((size_t)(parm_size)+GLUE_STORE_P1_TO_STACK_AT_OFFS_SIZE))
						RET_MINUS1_FAIL("insufficient size for varparm stackchk")
						if (bufOut) GLUE_STORE_P1_TO_STACK_AT_OFFS(bufOut + parm_size, 0);
					parm_size += GLUE_STORE_P1_TO_STACK_AT_OFFS_SIZE;
				}
			}
		}
		if (op->opcodeType == OPCODETYPE_FUNCX)
		{
			n_params = 0;
			for (x = 0; x < 3; x++)
			{
				opcodeRec *prni = op->parms.parms[x];
				while (prni)
				{
					const int32_t isMP = prni->opcodeType == OPCODETYPE_MOREPARAMS;
					opcodeRec *r = isMP ? prni->parms.parms[0] : prni;
					if (r)
					{
						int32_t canHaveDenorm = 0;
						int32_t rvt = RETURNVALUE_NORMAL;
						int32_t subfpstackuse = 0;
						int32_t lsz = compileOpcodes(ctx, r, bufOut ? bufOut + parm_size : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis, rvt, &rvt, &subfpstackuse, &canHaveDenorm);
						if (canHaveDenorm && canHaveDenormalOutput) *canHaveDenormalOutput = 1;
						if (lsz < 0) RET_MINUS1_FAIL("call coc for varparmX failed")
							if (rvt != RETURNVALUE_NORMAL) RET_MINUS1_FAIL("call coc for varparmX gave bad type back");
						parm_size += lsz;
						if ((size_t)bufOut_len < ((size_t)(parm_size)+GLUE_STORE_P1_TO_STACK_AT_OFFS_SIZE)) RET_MINUS1_FAIL("call coc for varparmX size");
						if (bufOut) GLUE_STORE_P1_TO_STACK_AT_OFFS(bufOut + parm_size, n_params * sizeof(void *));
						parm_size += GLUE_STORE_P1_TO_STACK_AT_OFFS_SIZE;
						if (subfpstackuse + local_fpstack_use > *fpStackUsage) *fpStackUsage = subfpstackuse + local_fpstack_use;
					}
					else RET_MINUS1_FAIL("zero parameter varparmX")
						n_params++;
					if (!isMP || n_params >= max_params) break;
					prni = prni->parms.parms[1];
				}
			}
		}
		else for (x = 0; x < n_params; x++)
		{
			opcodeRec *r = op->parms.parms[x];
			if (r)
			{
				int32_t canHaveDenorm = 0;
				int32_t subfpstackuse = 0;
				int32_t rvt = RETURNVALUE_NORMAL;
				int32_t lsz = compileOpcodes(ctx, r, bufOut ? bufOut + parm_size : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis, rvt, &rvt, &subfpstackuse, &canHaveDenorm);
				if (canHaveDenorm && canHaveDenormalOutput) *canHaveDenormalOutput = 1;
				if (lsz < 0) RET_MINUS1_FAIL("call coc for varparm123 failed")
					if (rvt != RETURNVALUE_NORMAL) RET_MINUS1_FAIL("call coc for varparm123 gave bad type back");
				parm_size += lsz;
				if ((size_t)bufOut_len < ((size_t)(parm_size)+GLUE_STORE_P1_TO_STACK_AT_OFFS_SIZE)) RET_MINUS1_FAIL("call coc for varparm123 size");
				if (bufOut) GLUE_STORE_P1_TO_STACK_AT_OFFS(bufOut + parm_size, x * sizeof(void *));
				parm_size += GLUE_STORE_P1_TO_STACK_AT_OFFS_SIZE;
				if (subfpstackuse + local_fpstack_use > *fpStackUsage) *fpStackUsage = subfpstackuse + local_fpstack_use;
			}
			else RET_MINUS1_FAIL("zero parameter for varparm123");
		}
		if ((size_t)bufOut_len < ((size_t)(parm_size)+GLUE_MOV_PX_DIRECTVALUE_SIZE + GLUE_MOVE_PX_STACKPTR_SIZE)) RET_MINUS1_FAIL("insufficient size for varparm p1")
			if (bufOut) GLUE_MOV_PX_DIRECTVALUE_GEN(bufOut + parm_size, (INT_PTR)n_params, 1);
		parm_size += GLUE_MOV_PX_DIRECTVALUE_SIZE;
		if (bufOut) GLUE_MOVE_PX_STACKPTR_GEN(bufOut + parm_size, 0);
		parm_size += GLUE_MOVE_PX_STACKPTR_SIZE;
	}
	else // not varparm
	{
		int32_t pn;
#ifdef GLUE_HAS_FXCH
		int32_t need_fxch = 0;
#endif
		int32_t last_nt_parm = -1, last_nt_parm_type;
		if (op->opcodeType == OPCODETYPE_FUNCX)
		{
			// this is not yet supported (calling conventions will need to be sorted, among other things)
			RET_MINUS1_FAIL("funcx for native functions requires BIF_TAKES_VARPARM or BIF_TAKES_VARPARM_EX")
		}
		// end of built-in function specific special casing
		// first pass, calculate any non-trivial parameters
		for (pn = 0; pn < n_params; pn++)
		{
			if (!OPCODE_IS_TRIVIAL(op->parms.parms[pn]))
			{
				int32_t canHaveDenorm = 0;
				int32_t subfpstackuse = 0;
				int32_t lsz = 0;
				int32_t rvt = RETURNVALUE_NORMAL;
				int32_t may_need_fppush = -1;
				if (last_nt_parm >= 0)
				{
					if (last_nt_parm_type == RETURNVALUE_FPSTACK)
					{
						may_need_fppush = parm_size;
					}
					else
					{
						// push last result
						if (bufOut_len < parm_size + (int32_t)sizeof(GLUE_PUSH_P1)) RET_MINUS1_FAIL("failed on size, pushp1")
							if (bufOut) memcpy(bufOut + parm_size, &GLUE_PUSH_P1, sizeof(GLUE_PUSH_P1));
						parm_size += sizeof(GLUE_PUSH_P1);
					}
				}
				if (func == nseel_asm_bnot) rvt = RETURNVALUE_BOOL_REVERSED | RETURNVALUE_BOOL;
				else if (pn == n_params - 1)
				{
					if (cfunc_abiinfo&BIF_LASTPARMONSTACK) rvt = RETURNVALUE_FPSTACK;
					else if (cfunc_abiinfo&BIF_LASTPARM_ASBOOL) rvt = RETURNVALUE_BOOL;
					else if (func == nseel_asm_assign) rvt = RETURNVALUE_FPSTACK | RETURNVALUE_NORMAL;
				}
				else if (pn == n_params - 2 && (cfunc_abiinfo&BIF_SECONDLASTPARMST))
				{
					rvt = RETURNVALUE_FPSTACK;
				}
				lsz = compileOpcodes(ctx, op->parms.parms[pn], bufOut ? bufOut + parm_size : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis, rvt, &rvt, &subfpstackuse, &canHaveDenorm);
				if (lsz < 0) RET_MINUS1_FAIL("call coc failed")
					if (func == nseel_asm_bnot && rvt == RETURNVALUE_BOOL_REVERSED)
					{
						// remove bnot, compileOpcodes() used fptobool_rev
						func = nseel_asm_bnotnot;
						func_e = nseel_asm_bnotnot_end;
						rvt = RETURNVALUE_BOOL;
					}
				if (canHaveDenorm && canHaveDenormalOutput) *canHaveDenormalOutput = 1;
				parm_size += lsz;
				if (may_need_fppush >= 0)
				{
					if (local_fpstack_use + subfpstackuse >= (GLUE_MAX_FPSTACK_SIZE - 1) || (0 & OPTFLAG_NO_FPSTACK))
					{
						if (bufOut_len < parm_size + (int32_t)sizeof(GLUE_POP_FPSTACK_TOSTACK))
							RET_MINUS1_FAIL("failed on size, popfpstacktostack")
							if (bufOut)
							{
								memmove(bufOut + may_need_fppush + sizeof(GLUE_POP_FPSTACK_TOSTACK), bufOut + may_need_fppush, parm_size - may_need_fppush);
								memcpy(bufOut + may_need_fppush, &GLUE_POP_FPSTACK_TOSTACK, sizeof(GLUE_POP_FPSTACK_TOSTACK));
							}
						parm_size += sizeof(GLUE_POP_FPSTACK_TOSTACK);
					}
					else
					{
						local_fpstack_use++;
					}
				}
				if (subfpstackuse + local_fpstack_use > *fpStackUsage)
					*fpStackUsage = subfpstackuse + local_fpstack_use;
				last_nt_parm = pn;
				last_nt_parm_type = rvt;
				if (pn == n_params - 1 && func == nseel_asm_assign)
				{
					if (!canHaveDenorm)
					{
						if (rvt == RETURNVALUE_FPSTACK)
						{
							cfunc_abiinfo |= BIF_LASTPARMONSTACK;
							func = nseel_asm_assign_fast_fromfp;
							func_e = nseel_asm_assign_fast_fromfp_end;
						}
						else
						{
							func = nseel_asm_assign_fast;
							func_e = nseel_asm_assign_fast_end;
						}
					}
					else
					{
						if (rvt == RETURNVALUE_FPSTACK)
						{
							cfunc_abiinfo |= BIF_LASTPARMONSTACK;
							func = nseel_asm_assign_fromfp;
							func_e = nseel_asm_assign_fromfp_end;
						}
					}
				}
			}
		}
		pn = last_nt_parm;
		if (pn >= 0) // if the last thing executed doesn't go to the last parameter, move it there
		{
			if ((cfunc_abiinfo&BIF_SECONDLASTPARMST) && pn == n_params - 2)
			{
				// do nothing, things are in the right place
			}
			else if (pn != n_params - 1)
			{
				// generate mov p1->pX
				if ((size_t)bufOut_len < ((size_t)(parm_size)+GLUE_SET_PX_FROM_P1_SIZE)) RET_MINUS1_FAIL("size, pxfromp1")
					if (bufOut) GLUE_SET_PX_FROM_P1(bufOut + parm_size, n_params - 1 - pn);
				parm_size += GLUE_SET_PX_FROM_P1_SIZE;
			}
		}
		// pop any pushed parameters
		while (--pn >= 0)
		{
			if (!OPCODE_IS_TRIVIAL(op->parms.parms[pn]))
			{
				if ((cfunc_abiinfo&BIF_SECONDLASTPARMST) && pn == n_params - 2)
				{
					if (!local_fpstack_use)
					{
						if (bufOut_len < parm_size + (int32_t)sizeof(GLUE_POP_STACK_TO_FPSTACK)) RET_MINUS1_FAIL("size, popstacktofpstack 2")
							if (bufOut) memcpy(bufOut + parm_size, GLUE_POP_STACK_TO_FPSTACK, sizeof(GLUE_POP_STACK_TO_FPSTACK));
						parm_size += sizeof(GLUE_POP_STACK_TO_FPSTACK);
#ifdef GLUE_HAS_FXCH
						need_fxch = 1;
#endif
					}
					else
					{
						local_fpstack_use--;
					}
				}
				else
				{
					if ((size_t)bufOut_len < (size_t)(parm_size)+GLUE_POP_PX_SIZE) RET_MINUS1_FAIL("size, poppx")
						if (bufOut) GLUE_POP_PX(bufOut + parm_size, n_params - 1 - pn);
					parm_size += GLUE_POP_PX_SIZE;
				}
			}
		}
		// finally, set trivial pointers
		for (pn = 0; pn < n_params; pn++)
		{
			if (OPCODE_IS_TRIVIAL(op->parms.parms[pn]))
			{
				if (pn == n_params - 2 && (cfunc_abiinfo&(BIF_SECONDLASTPARMST)))  // second to last parameter
				{
					int32_t a = compileOpcodes(ctx, op->parms.parms[pn], bufOut ? bufOut + parm_size : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis,
						RETURNVALUE_FPSTACK, NULL, NULL, canHaveDenormalOutput);
					if (a < 0) RET_MINUS1_FAIL("coc call here 2")
						parm_size += a;
#ifdef GLUE_HAS_FXCH
					need_fxch = 1;
#endif
				}
				else if (pn == n_params - 1)  // last parameter, but we should call compileOpcodes to get it in the right format (compileOpcodes can optimize that process if it needs to)
				{
					int32_t rvt = 0, a;
					int32_t wantFpStack = func == nseel_asm_assign;
#ifdef GLUE_PREFER_NONFP_DV_ASSIGNS // x86-64, and maybe others, prefer to avoid the fp stack for a simple copy
					if (wantFpStack &&
						(op->parms.parms[pn]->opcodeType != OPCODETYPE_DIRECTVALUE ||
						(op->parms.parms[pn]->parms.dv.directValue != 1.0f && op->parms.parms[pn]->parms.dv.directValue != 0.0f)))
					{
						wantFpStack = 0;
					}
#endif
					a = compileOpcodes(ctx, op->parms.parms[pn], bufOut ? bufOut + parm_size : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis,
						func == nseel_asm_bnot ? (RETURNVALUE_BOOL_REVERSED | RETURNVALUE_BOOL) :
						(cfunc_abiinfo & BIF_LASTPARMONSTACK) ? RETURNVALUE_FPSTACK :
						(cfunc_abiinfo & BIF_LASTPARM_ASBOOL) ? RETURNVALUE_BOOL :
						wantFpStack ? (RETURNVALUE_FPSTACK | RETURNVALUE_NORMAL) :
						RETURNVALUE_NORMAL,
						&rvt, NULL, canHaveDenormalOutput);
					if (a < 0) RET_MINUS1_FAIL("coc call here 3")
						if (func == nseel_asm_bnot && rvt == RETURNVALUE_BOOL_REVERSED)
						{
							// remove bnot, compileOpcodes() used fptobool_rev
							func = nseel_asm_bnotnot;
							func_e = nseel_asm_bnotnot_end;
							rvt = RETURNVALUE_BOOL;
						}
					parm_size += a;
#ifdef GLUE_HAS_FXCH
					need_fxch = 0;
#endif
					if (func == nseel_asm_assign)
					{
						if (rvt == RETURNVALUE_FPSTACK)
						{
							func = nseel_asm_assign_fast_fromfp;
							func_e = nseel_asm_assign_fast_fromfp_end;
						}
						else
						{
							// assigning a value (from a variable or other non-computer), can use a fast assign (no denormal/result checking)
							func = nseel_asm_assign_fast;
							func_e = nseel_asm_assign_fast_end;
						}
					}
				}
				else
				{
					if ((size_t)bufOut_len < ((size_t)(parm_size)+GLUE_MOV_PX_DIRECTVALUE_SIZE)) RET_MINUS1_FAIL("size, pxdvsz")
						if (bufOut)
						{
							if (generateValueToReg(ctx, op->parms.parms[pn], bufOut + parm_size, n_params - 1 - pn, namespacePathToThis, 0/*nocaching, function gets pointer*/) < 0) RET_MINUS1_FAIL("gvtr")
						}
					parm_size += GLUE_MOV_PX_DIRECTVALUE_SIZE;
				}
			}
		}
#ifdef GLUE_HAS_FXCH
		if ((cfunc_abiinfo&(BIF_SECONDLASTPARMST)) && !(cfunc_abiinfo&(BIF_LAZYPARMORDERING)) &&
			((!!need_fxch) ^ !!(cfunc_abiinfo&BIF_REVERSEFPORDER))
			)
		{
			// emit fxch
			if ((size_t)bufOut_len < sizeof(GLUE_FXCH)) RET_MINUS1_FAIL("len,fxch")
				if (bufOut)
				{
					memcpy(bufOut + parm_size, GLUE_FXCH, sizeof(GLUE_FXCH));
				}
			parm_size += sizeof(GLUE_FXCH);
		}
#endif
		if (!*canHaveDenormalOutput)
		{
			// if add_op or sub_op, and non-denormal input, safe to omit denormal checks
			if (func == (void*)nseel_asm_add_op)
			{
				func = nseel_asm_add_op_fast;
				func_e = nseel_asm_add_op_fast_end;
			}
			else if (func == (void*)nseel_asm_sub_op)
			{
				func = nseel_asm_sub_op_fast;
				func_e = nseel_asm_sub_op_fast_end;
			}
			// or if mul/div by a fixed value of >= or <= 1.0f
			else if (func == (void *)nseel_asm_mul_op && parm1_dv && fabsf(op->parms.parms[1]->parms.dv.directValue) >= 1.0f)
			{
				func = nseel_asm_mul_op_fast;
				func_e = nseel_asm_mul_op_fast_end;
			}
			else if (func == (void *)nseel_asm_div_op && parm1_dv && fabsf(op->parms.parms[1]->parms.dv.directValue) <= 1.0f)
			{
				func = nseel_asm_div_op_fast;
				func_e = nseel_asm_div_op_fast_end;
			}
		}
	} // not varparm
	if (cfunc_abiinfo & (BIF_CLEARDENORMAL | BIF_RETURNSBOOL)) *canHaveDenormalOutput = 0;
	else if (!(cfunc_abiinfo & BIF_WONTMAKEDENORMAL)) *canHaveDenormalOutput = 1;
	func = GLUE_realAddress(func, func_e, &func_size);
	if (!func) RET_MINUS1_FAIL("failrealladdrfunc")
		if (bufOut_len < parm_size + func_size) RET_MINUS1_FAIL("funcsz")
			if (bufOut)
			{
				unsigned char *p = bufOut + parm_size;
				memcpy(p, func, func_size);
				if (preProc) p = preProc(p, func_size, ctx);
				if (repl)
				{
					if (repl[0]) p = EEL_GLUE_set_immediate(p, (INT_PTR)repl[0]);
					if (repl[1]) p = EEL_GLUE_set_immediate(p, (INT_PTR)repl[1]);
					if (repl[2]) p = EEL_GLUE_set_immediate(p, (INT_PTR)repl[2]);
					if (repl[3]) p = EEL_GLUE_set_immediate(p, (INT_PTR)repl[3]);
				}
			}
	if (restore_stack_amt)
	{
		if ((size_t)bufOut_len < ((size_t)(parm_size + func_size) + GLUE_MOVE_STACK_SIZE)) RET_MINUS1_FAIL("insufficient size for varparm")
			if (bufOut) GLUE_MOVE_STACK(bufOut + parm_size + func_size, restore_stack_amt);
		parm_size += GLUE_MOVE_STACK_SIZE;
	}
	if (cfunc_abiinfo&BIF_RETURNSONSTACK) *rvMode = RETURNVALUE_FPSTACK;
	else if (cfunc_abiinfo&BIF_RETURNSBOOL) *rvMode = RETURNVALUE_BOOL;
	return parm_size + func_size;
}
static int32_t compileEelFunctionCall(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int32_t bufOut_len, int32_t *computTableSize, const namespaceInformation *namespacePathToThis,
	int32_t *rvMode, int32_t *fpStackUse, int32_t *canHaveDenormalOutput)
{
	int32_t func_size = 0, parm_size = 0;
	int32_t pn;
	int32_t last_nt_parm = -1, last_nt_parm_mode = 0;
	void *func_e = NULL;
	int32_t n_params;
	opcodeRec *parmptrs[NSEEL_MAX_EELFUNC_PARAMETERS];
	int32_t cfp_numparams = -1;
	int32_t cfp_statesize = 0;
	float **cfp_ptrs = NULL;
	int32_t func_raw = 0;
	int32_t do_parms;
	int32_t x;
	void *func;
	for (x = 0; x < 3; x++) parmptrs[x] = op->parms.parms[x];
	if (op->opcodeType == OPCODETYPE_FUNCX)
	{
		n_params = 0;
		for (x = 0; x < 3; x++)
		{
			opcodeRec *prni = op->parms.parms[x];
			while (prni && n_params < NSEEL_MAX_EELFUNC_PARAMETERS)
			{
				const int32_t isMP = prni->opcodeType == OPCODETYPE_MOREPARAMS;
				parmptrs[n_params++] = isMP ? prni->parms.parms[0] : prni;
				if (!isMP) break;
				prni = prni->parms.parms[1];
			}
		}
	}
	else
	{
		n_params = 1 + op->opcodeType - OPCODETYPE_FUNC1;
	}
	*fpStackUse = 0;
	func = nseel_getEELFunctionAddress(ctx, op,
		&cfp_numparams, &cfp_statesize, &cfp_ptrs,
		computTableSize,
		&func_e, &func_raw,
		!!bufOut, namespacePathToThis, rvMode, fpStackUse, canHaveDenormalOutput, parmptrs, n_params);
	if (func_raw) func_size = (int32_t)((char*)func_e - (char*)func);
	else if (func) func = GLUE_realAddress(func, func_e, &func_size);
	if (!func) RET_MINUS1_FAIL("eelfuncaddr")
		*fpStackUse += 1;
	if (cfp_numparams > 0 && n_params != cfp_numparams)
	{
		RET_MINUS1_FAIL("eelfuncnp")
	}
	// user defined function
	do_parms = cfp_numparams > 0 && cfp_ptrs && cfp_statesize > 0;
	// if function local/parameter state is zero, we need to allocate storage for it
	if (cfp_statesize > 0 && cfp_ptrs && !cfp_ptrs[0])
	{
		float *pstate = newDataBlock(sizeof(float)*cfp_statesize, 8);
		if (!pstate) RET_MINUS1_FAIL("eelfuncdb")
			for (pn = 0; pn < cfp_statesize; pn++)
			{
				pstate[pn] = 0;
				cfp_ptrs[pn] = pstate + pn;
			}
	}
	// first process parameters that are non-trivial
	for (pn = 0; pn < n_params; pn++)
	{
		int32_t needDenorm = 0;
		int32_t lsz, sUse = 0;
		if (!parmptrs[pn] || OPCODE_IS_TRIVIAL(parmptrs[pn])) continue; // skip and process after
		if (last_nt_parm >= 0 && do_parms)
		{
			if (last_nt_parm_mode == RETURNVALUE_FPSTACK)
			{
				if (bufOut_len < parm_size + (int32_t)sizeof(GLUE_POP_FPSTACK_TOSTACK)) RET_MINUS1_FAIL("eelfunc_size popfpstacktostack")
					if (bufOut) memcpy(bufOut + parm_size, GLUE_POP_FPSTACK_TOSTACK, sizeof(GLUE_POP_FPSTACK_TOSTACK));
				parm_size += sizeof(GLUE_POP_FPSTACK_TOSTACK);
			}
			else
			{
				if (bufOut_len < parm_size + (int32_t)sizeof(GLUE_PUSH_P1PTR_AS_VALUE)) RET_MINUS1_FAIL("eelfunc_size pushp1ptrasval")
					// push
					if (bufOut) memcpy(bufOut + parm_size, &GLUE_PUSH_P1PTR_AS_VALUE, sizeof(GLUE_PUSH_P1PTR_AS_VALUE));
				parm_size += sizeof(GLUE_PUSH_P1PTR_AS_VALUE);
			}
		}
		last_nt_parm_mode = 0;
		lsz = compileOpcodes(ctx, parmptrs[pn], bufOut ? bufOut + parm_size : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis,
			do_parms ? (RETURNVALUE_FPSTACK | RETURNVALUE_NORMAL) : RETURNVALUE_IGNORE, &last_nt_parm_mode, &sUse, &needDenorm);
		// todo: if needDenorm, denorm convert when copying parameter
		if (lsz < 0) RET_MINUS1_FAIL("eelfunc, coc fail")
			if (last_nt_parm_mode == RETURNVALUE_FPSTACK) sUse++;
		if (sUse > *fpStackUse) *fpStackUse = sUse;
		parm_size += lsz;
		last_nt_parm = pn;
	}
	// pop non-trivial results into place
	if (last_nt_parm >= 0 && do_parms)
	{
		while (--pn >= 0)
		{
			if (!parmptrs[pn] || OPCODE_IS_TRIVIAL(parmptrs[pn])) continue; // skip and process after
			if (pn == last_nt_parm)
			{
				if (last_nt_parm_mode == RETURNVALUE_FPSTACK)
				{
					// pop to memory directly
					const int32_t cpsize = GLUE_POP_FPSTACK_TO_PTR(NULL, NULL);
					if (bufOut_len < parm_size + cpsize) RET_MINUS1_FAIL("eelfunc size popfpstacktoptr")
						if (bufOut) GLUE_POP_FPSTACK_TO_PTR((unsigned char *)bufOut + parm_size, cfp_ptrs[pn]);
					parm_size += cpsize;
				}
				else
				{
					// copy direct p1ptr to mem
					const int32_t cpsize = GLUE_COPY_VALUE_AT_P1_TO_PTR(NULL, NULL);
					if (bufOut_len < parm_size + cpsize) RET_MINUS1_FAIL("eelfunc size copyvalueatp1toptr")
						if (bufOut) GLUE_COPY_VALUE_AT_P1_TO_PTR((unsigned char *)bufOut + parm_size, cfp_ptrs[pn]);
					parm_size += cpsize;
				}
			}
			else
			{
				const int32_t popsize = GLUE_POP_VALUE_TO_ADDR(NULL, NULL);
				if (bufOut_len < parm_size + popsize) RET_MINUS1_FAIL("eelfunc size pop value to addr")
					if (bufOut) GLUE_POP_VALUE_TO_ADDR((unsigned char *)bufOut + parm_size, cfp_ptrs[pn]);
				parm_size += popsize;
			}
		}
	}
	// finally, set any trivial parameters
	if (do_parms)
	{
		const int32_t cpsize = GLUE_MOV_PX_DIRECTVALUE_SIZE + GLUE_COPY_VALUE_AT_P1_TO_PTR(NULL, NULL);
		for (pn = 0; pn < n_params; pn++)
		{
			if (!parmptrs[pn] || !OPCODE_IS_TRIVIAL(parmptrs[pn])) continue; // set trivial values, we already set nontrivials
			if (bufOut_len < parm_size + cpsize) RET_MINUS1_FAIL("eelfunc size trivial set")
				if (bufOut)
				{
					if (generateValueToReg(ctx, parmptrs[pn], bufOut + parm_size, 0, namespacePathToThis, 1) < 0) RET_MINUS1_FAIL("eelfunc gvr fail")
						GLUE_COPY_VALUE_AT_P1_TO_PTR(bufOut + parm_size + GLUE_MOV_PX_DIRECTVALUE_SIZE, cfp_ptrs[pn]);
				}
			parm_size += cpsize;
		}
	}
	if (bufOut_len < parm_size + func_size) RET_MINUS1_FAIL("eelfunc size combined")
		if (bufOut) memcpy(bufOut + parm_size, func, func_size);
	return parm_size + func_size;
	// end of EEL function generation
}
#define CHECK_SIZE_FORJMP(x,y)
#define RET_MINUS1_FAIL_FALLBACK(err,j) RET_MINUS1_FAIL(err)
static int32_t compileOpcodesInternal(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int32_t bufOut_len, int32_t *computTableSize, const namespaceInformation *namespacePathToThis, int32_t *calledRvType, int32_t preferredReturnValues, int32_t *fpStackUse, int32_t *canHaveDenormalOutput)
{
	int32_t rv_offset = 0, denormal_force = -1;
	if (!op) RET_MINUS1_FAIL("coi !op")
		*fpStackUse = 0;
	for (;;)
	{
		// special case: statement delimiting means we can process the left side into place, and iteratively do the second parameter without recursing
		// also we don't need to save/restore anything to the stack (which the normal 2 parameter function processing does)
		if (op->opcodeType == OPCODETYPE_FUNC2 && op->fntype == FN_JOIN_STATEMENTS)
		{
			int32_t fUse1;
			int32_t parm_size = compileOpcodes(ctx, op->parms.parms[0], bufOut, bufOut_len, computTableSize, namespacePathToThis, RETURNVALUE_IGNORE, NULL, &fUse1, NULL);
			if (parm_size < 0) RET_MINUS1_FAIL("coc join fail")
				op = op->parms.parms[1];
			if (!op) RET_MINUS1_FAIL("join got to null")
				if (fUse1 > *fpStackUse) *fpStackUse = fUse1;
			if (bufOut) bufOut += parm_size;
			bufOut_len -= parm_size;
			rv_offset += parm_size;
			denormal_force = -1;
		}
		// special case: __denormal_likely(), __denormal_unlikely()
		else if (op->opcodeType == OPCODETYPE_FUNC1 && (op->fntype == FN_DENORMAL_LIKELY || op->fntype == FN_DENORMAL_UNLIKELY))
		{
			denormal_force = op->fntype == FN_DENORMAL_LIKELY;
			op = op->parms.parms[0];
		}
		else
		{
			break;
		}
	}
	if (denormal_force >= 0 && canHaveDenormalOutput)
	{
		*canHaveDenormalOutput = denormal_force;
		canHaveDenormalOutput = &denormal_force; // prevent it from being changed by functions below
	}
	// special case: BAND/BOR
	if (op->opcodeType == OPCODETYPE_FUNC2 && (op->fntype == FN_LOGICAL_AND || op->fntype == FN_LOGICAL_OR))
	{
		int32_t fUse1 = 0;
		int32_t parm_size;
		int32_t retType = RETURNVALUE_IGNORE;
		if (preferredReturnValues != RETURNVALUE_IGNORE) retType = RETURNVALUE_BOOL;
		*calledRvType = retType;
		parm_size = compileOpcodes(ctx, op->parms.parms[0], bufOut, bufOut_len, computTableSize, namespacePathToThis, RETURNVALUE_BOOL, NULL, &fUse1, NULL);
		if (parm_size < 0) RET_MINUS1_FAIL("loop band/bor coc fail")
			if (fUse1 > *fpStackUse) *fpStackUse = fUse1;
		{
			int32_t sz2, fUse2 = 0;
			unsigned char *destbuf;
			const int32_t testsz = op->fntype == FN_LOGICAL_OR ? sizeof(GLUE_JMP_IF_P1_NZ) : sizeof(GLUE_JMP_IF_P1_Z);
			if (bufOut_len < parm_size + testsz) RET_MINUS1_FAIL_FALLBACK("band/bor size fail", doNonInlinedAndOr_)
				if (bufOut)  memcpy(bufOut + parm_size, op->fntype == FN_LOGICAL_OR ? GLUE_JMP_IF_P1_NZ : GLUE_JMP_IF_P1_Z, testsz);
			parm_size += testsz;
			destbuf = bufOut + parm_size;
			sz2 = compileOpcodes(ctx, op->parms.parms[1], bufOut ? bufOut + parm_size : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis, retType, NULL, &fUse2, NULL);
			CHECK_SIZE_FORJMP(sz2, doNonInlinedAndOr_)
				if (sz2 < 0) RET_MINUS1_FAIL("band/bor coc fail")
					parm_size += sz2;
			if (bufOut) GLUE_JMP_SET_OFFSET(destbuf, (bufOut + parm_size) - destbuf);
			if (fUse2 > *fpStackUse) *fpStackUse = fUse2;
			return rv_offset + parm_size;
		}
	}
	if (op->opcodeType == OPCODETYPE_FUNC3 && op->fntype == FN_IF_ELSE) // special case: IF
	{
		int32_t fUse1 = 0;
		int32_t use_rv = RETURNVALUE_IGNORE;
		int32_t rvMode = 0;
		int32_t parm_size = compileOpcodes(ctx, op->parms.parms[0], bufOut, bufOut_len, computTableSize, namespacePathToThis, RETURNVALUE_BOOL | RETURNVALUE_BOOL_REVERSED, &rvMode, &fUse1, NULL);
		if (parm_size < 0) RET_MINUS1_FAIL("if coc fail")
			if (fUse1 > *fpStackUse) *fpStackUse = fUse1;
		if (preferredReturnValues & RETURNVALUE_NORMAL) use_rv = RETURNVALUE_NORMAL;
		else if (preferredReturnValues & RETURNVALUE_FPSTACK) use_rv = RETURNVALUE_FPSTACK;
		else if (preferredReturnValues & RETURNVALUE_BOOL) use_rv = RETURNVALUE_BOOL;
		*calledRvType = use_rv;
		{
			int32_t csz, hasSecondHalf;
			if (rvMode & RETURNVALUE_BOOL_REVERSED)
			{
				if (bufOut_len < parm_size + (int32_t)sizeof(GLUE_JMP_IF_P1_NZ)) RET_MINUS1_FAIL_FALLBACK("if size fail", doNonInlineIf_)
					if (bufOut) memcpy(bufOut + parm_size, GLUE_JMP_IF_P1_NZ, sizeof(GLUE_JMP_IF_P1_NZ));
				parm_size += sizeof(GLUE_JMP_IF_P1_NZ);
			}
			else
			{
				if (bufOut_len < parm_size + (int32_t)sizeof(GLUE_JMP_IF_P1_Z)) RET_MINUS1_FAIL_FALLBACK("if size fail", doNonInlineIf_)
					if (bufOut) memcpy(bufOut + parm_size, GLUE_JMP_IF_P1_Z, sizeof(GLUE_JMP_IF_P1_Z));
				parm_size += sizeof(GLUE_JMP_IF_P1_Z);
			}
			csz = compileOpcodes(ctx, op->parms.parms[1], bufOut ? bufOut + parm_size : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis, use_rv, NULL, &fUse1, canHaveDenormalOutput);
			if (fUse1 > *fpStackUse) *fpStackUse = fUse1;
			hasSecondHalf = preferredReturnValues || !OPCODE_IS_TRIVIAL(op->parms.parms[2]);
			CHECK_SIZE_FORJMP(csz, doNonInlineIf_)
				if (csz < 0) RET_MINUS1_FAIL("if coc fial")
					if (bufOut) GLUE_JMP_SET_OFFSET(bufOut + parm_size, csz + (hasSecondHalf ? sizeof(GLUE_JMP_NC) : 0));
			parm_size += csz;
			if (hasSecondHalf)
			{
				if (bufOut_len < parm_size + (int32_t)sizeof(GLUE_JMP_NC)) RET_MINUS1_FAIL_FALLBACK("if len fail", doNonInlineIf_)
					if (bufOut) memcpy(bufOut + parm_size, GLUE_JMP_NC, sizeof(GLUE_JMP_NC));
				parm_size += sizeof(GLUE_JMP_NC);
				csz = compileOpcodes(ctx, op->parms.parms[2], bufOut ? bufOut + parm_size : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis, use_rv, NULL, &fUse1, canHaveDenormalOutput);
				CHECK_SIZE_FORJMP(csz, doNonInlineIf_)
					if (csz < 0) RET_MINUS1_FAIL("if coc 2 fail")
						// update jump address
						if (bufOut) GLUE_JMP_SET_OFFSET(bufOut + parm_size, csz);
				parm_size += csz;
				if (fUse1 > *fpStackUse) *fpStackUse = fUse1;
			}
			return rv_offset + parm_size;
		}
	}
	{
		// special case: while
		if (op->opcodeType == OPCODETYPE_FUNC1 && op->fntype == FN_WHILE)
		{
			*calledRvType = RETURNVALUE_BOOL;
			{
				unsigned char *jzoutpt;
				unsigned char *looppt;
				int32_t parm_size = 0, subsz;
				if (bufOut_len < parm_size + (int32_t)(GLUE_WHILE_SETUP_SIZE + sizeof(GLUE_WHILE_BEGIN))) RET_MINUS1_FAIL("while size fail 1")
					if (bufOut) memcpy(bufOut + parm_size, GLUE_WHILE_SETUP, GLUE_WHILE_SETUP_SIZE);
				parm_size += GLUE_WHILE_SETUP_SIZE;
				looppt = bufOut + parm_size;
				if (bufOut) memcpy(bufOut + parm_size, GLUE_WHILE_BEGIN, sizeof(GLUE_WHILE_BEGIN));
				parm_size += sizeof(GLUE_WHILE_BEGIN);
				subsz = compileOpcodes(ctx, op->parms.parms[0], bufOut ? (bufOut + parm_size) : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis, RETURNVALUE_BOOL, NULL, fpStackUse, NULL);
				if (subsz < 0) RET_MINUS1_FAIL("while coc fail")
					if (bufOut_len < parm_size + (int32_t)(sizeof(GLUE_WHILE_END) + sizeof(GLUE_WHILE_CHECK_RV))) RET_MINUS1_FAIL("which size fial 2")
						parm_size += subsz;
				if (bufOut) memcpy(bufOut + parm_size, GLUE_WHILE_END, sizeof(GLUE_WHILE_END));
				parm_size += sizeof(GLUE_WHILE_END);
				jzoutpt = bufOut + parm_size;
				if (bufOut) memcpy(bufOut + parm_size, GLUE_WHILE_CHECK_RV, sizeof(GLUE_WHILE_CHECK_RV));
				parm_size += sizeof(GLUE_WHILE_CHECK_RV);
				if (bufOut)
				{
					GLUE_JMP_SET_OFFSET(bufOut + parm_size, (looppt - (bufOut + parm_size)));
					GLUE_JMP_SET_OFFSET(jzoutpt, (bufOut + parm_size) - jzoutpt);
				}
				return rv_offset + parm_size;
			}
		}
		// special case: loop
		if (op->opcodeType == OPCODETYPE_FUNC2 && op->fntype == FN_LOOP)
		{
			int32_t fUse1;
			int32_t parm_size = compileOpcodes(ctx, op->parms.parms[0], bufOut, bufOut_len, computTableSize, namespacePathToThis, RETURNVALUE_FPSTACK, NULL, &fUse1, NULL);
			if (parm_size < 0) RET_MINUS1_FAIL("loop coc fail")
				*calledRvType = RETURNVALUE_BOOL;
			if (fUse1 > *fpStackUse) *fpStackUse = fUse1;
			{
				int32_t subsz;
				int32_t fUse2 = 0;
				unsigned char *skipptr1, *loopdest;
				if (bufOut_len < parm_size + (int32_t)(sizeof(GLUE_LOOP_LOADCNT) + GLUE_LOOP_CLAMPCNT_SIZE + GLUE_LOOP_BEGIN_SIZE)) RET_MINUS1_FAIL("loop size fail")
					// store, convert to int32_t, compare against 1, if less than, skip to end
					if (bufOut) memcpy(bufOut + parm_size, GLUE_LOOP_LOADCNT, sizeof(GLUE_LOOP_LOADCNT));
				parm_size += sizeof(GLUE_LOOP_LOADCNT);
				skipptr1 = bufOut + parm_size;
				// compare aginst max loop length, jump to loop start if not above it
				if (bufOut) memcpy(bufOut + parm_size, GLUE_LOOP_CLAMPCNT, GLUE_LOOP_CLAMPCNT_SIZE);
				parm_size += GLUE_LOOP_CLAMPCNT_SIZE;
				// loop code:
				loopdest = bufOut + parm_size;
				if (bufOut) memcpy(bufOut + parm_size, GLUE_LOOP_BEGIN, GLUE_LOOP_BEGIN_SIZE);
				parm_size += GLUE_LOOP_BEGIN_SIZE;
				subsz = compileOpcodes(ctx, op->parms.parms[1], bufOut ? (bufOut + parm_size) : NULL, bufOut_len - parm_size, computTableSize, namespacePathToThis, RETURNVALUE_IGNORE, NULL, &fUse2, NULL);
				if (subsz < 0) RET_MINUS1_FAIL("loop coc fail")
					if (fUse2 > *fpStackUse) *fpStackUse = fUse2;
				parm_size += subsz;
				if (bufOut_len < parm_size + (int32_t)sizeof(GLUE_LOOP_END)) RET_MINUS1_FAIL("loop size fail 2")
					if (bufOut) memcpy(bufOut + parm_size, GLUE_LOOP_END, sizeof(GLUE_LOOP_END));
				parm_size += sizeof(GLUE_LOOP_END);
				if (bufOut)
				{
					GLUE_JMP_SET_OFFSET(bufOut + parm_size, loopdest - (bufOut + parm_size));
					GLUE_JMP_SET_OFFSET(skipptr1, (bufOut + parm_size) - skipptr1);
				}
				return rv_offset + parm_size;
			}
		}
	}
	switch (op->opcodeType)
	{
	case OPCODETYPE_DIRECTVALUE:
		if (preferredReturnValues == RETURNVALUE_BOOL)
		{
			int32_t w = fabsf(op->parms.dv.directValue) >= NSEEL_CLOSEFACTOR;
			int32_t wsz = (w ? sizeof(GLUE_SET_P1_NZ) : sizeof(GLUE_SET_P1_Z));
			*calledRvType = RETURNVALUE_BOOL;
			if (bufOut_len < wsz) RET_MINUS1_FAIL("direct int32_t size fail3")
				if (bufOut) memcpy(bufOut, w ? GLUE_SET_P1_NZ : GLUE_SET_P1_Z, wsz);
			return rv_offset + wsz;
		}
		else if (preferredReturnValues & RETURNVALUE_FPSTACK)
		{
#ifdef GLUE_HAS_FLDZ
			if (op->parms.dv.directValue == 0.0f)
			{
				*fpStackUse = 1;
				*calledRvType = RETURNVALUE_FPSTACK;
				if (bufOut_len < sizeof(GLUE_FLDZ)) RET_MINUS1_FAIL("direct fp fail 1")
					if (bufOut) memcpy(bufOut, GLUE_FLDZ, sizeof(GLUE_FLDZ));
				return rv_offset + sizeof(GLUE_FLDZ);
			}
#endif
#ifdef GLUE_HAS_FLD1
			if (op->parms.dv.directValue == 1.0f)
			{
				*fpStackUse = 1;
				*calledRvType = RETURNVALUE_FPSTACK;
				if (bufOut_len < sizeof(GLUE_FLD1)) RET_MINUS1_FAIL("direct fp fail 1")
					if (bufOut) memcpy(bufOut, GLUE_FLD1, sizeof(GLUE_FLD1));
				return rv_offset + sizeof(GLUE_FLD1);
			}
#endif
		}
		// fall through
	case OPCODETYPE_DIRECTVALUE_TEMPSTRING:
	case OPCODETYPE_VALUE_FROM_NAMESPACENAME:
	case OPCODETYPE_VARPTR:
	case OPCODETYPE_VARPTRPTR:
#ifdef GLUE_MOV_PX_DIRECTVALUE_TOSTACK_SIZE
		if (OPCODE_IS_TRIVIAL(op))
		{
			if (preferredReturnValues & RETURNVALUE_FPSTACK)
			{
				*fpStackUse = 1;
				if (bufOut_len < GLUE_MOV_PX_DIRECTVALUE_TOSTACK_SIZE) RET_MINUS1_FAIL("direct fp fail 2")
					if (bufOut)
					{
						if (generateValueToReg(ctx, op, bufOut, -1, namespacePathToThis, 1 /*allow caching*/) < 0) RET_MINUS1_FAIL("direct fp fail gvr")
					}
				*calledRvType = RETURNVALUE_FPSTACK;
				return rv_offset + GLUE_MOV_PX_DIRECTVALUE_TOSTACK_SIZE;
			}
		}
#endif
		if (bufOut_len < GLUE_MOV_PX_DIRECTVALUE_SIZE)
		{
			RET_MINUS1_FAIL("direct value fail 1")
		}
		if (bufOut)
		{
			if (generateValueToReg(ctx, op, bufOut, 0, namespacePathToThis, !!(preferredReturnValues&RETURNVALUE_FPSTACK)/*cache if going to the fp stack*/) < 0) RET_MINUS1_FAIL("direct value gvr fail3")
		}
		return rv_offset + GLUE_MOV_PX_DIRECTVALUE_SIZE;
	case OPCODETYPE_FUNCX:
	case OPCODETYPE_FUNC1:
	case OPCODETYPE_FUNC2:
	case OPCODETYPE_FUNC3:
		if (op->fntype == FUNCTYPE_EELFUNC)
		{
			int32_t a;
			a = compileEelFunctionCall(ctx, op, bufOut, bufOut_len, computTableSize, namespacePathToThis, calledRvType, fpStackUse, canHaveDenormalOutput);
			if (a < 0) return a;
			rv_offset += a;
		}
		else
		{
			int32_t a;
			a = compileNativeFunctionCall(ctx, op, bufOut, bufOut_len, computTableSize, namespacePathToThis, calledRvType, fpStackUse, preferredReturnValues, canHaveDenormalOutput);
			if (a < 0)return a;
			rv_offset += a;
		}
		return rv_offset;
	}
	RET_MINUS1_FAIL("default opcode fail")
}
int32_t compileOpcodes(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int32_t bufOut_len, int32_t *computTableSize, const namespaceInformation *namespacePathToThis,
	int32_t supportedReturnValues, int32_t *rvType, int32_t *fpStackUse, int32_t *canHaveDenormalOutput)
{
	int32_t code_returns = RETURNVALUE_NORMAL;
	int32_t fpsu = 0;
	int32_t codesz;
	int32_t denorm = 0;
	codesz = compileOpcodesInternal(ctx, op, bufOut, bufOut_len, computTableSize, namespacePathToThis, &code_returns, supportedReturnValues, &fpsu, &denorm);
	if (denorm && canHaveDenormalOutput) *canHaveDenormalOutput = 1;
	if (codesz < 0) return codesz;
	if (fpStackUse) *fpStackUse = fpsu;
	if (bufOut) bufOut += codesz;
	bufOut_len -= codesz;
	if (code_returns == RETURNVALUE_BOOL && !(supportedReturnValues & RETURNVALUE_BOOL) && supportedReturnValues)
	{
		int32_t stubsize;
		void *stub = GLUE_realAddress(nseel_asm_booltofp, nseel_asm_booltofp_end, &stubsize);
		if (!stub || bufOut_len < stubsize) RET_MINUS1_FAIL(stub ? "booltofp size" : "booltfp addr")
			if (bufOut)
			{
				memcpy(bufOut, stub, stubsize);
				bufOut += stubsize;
			}
		codesz += stubsize;
		bufOut_len -= stubsize;
		code_returns = RETURNVALUE_FPSTACK;
	}
	// default processing of code_returns to meet return value requirements
	if (supportedReturnValues & code_returns)
	{
		if (rvType) *rvType = code_returns;
		return codesz;
	}
	if (rvType) *rvType = RETURNVALUE_IGNORE;
	if (code_returns == RETURNVALUE_NORMAL)
	{
		if (supportedReturnValues & (RETURNVALUE_FPSTACK | RETURNVALUE_BOOL))
		{
			if (bufOut_len < GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE) RET_MINUS1_FAIL("pushvalatpxtofpstack,size")
				if (bufOut)
				{
					GLUE_PUSH_VAL_AT_PX_TO_FPSTACK(bufOut, 0); // always fld qword [eax] but we might change that later
					bufOut += GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE;
				}
			codesz += GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE;
			bufOut_len -= GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE;
			if (supportedReturnValues & RETURNVALUE_BOOL)
			{
				code_returns = RETURNVALUE_FPSTACK;
			}
			else
			{
				if (rvType) *rvType = RETURNVALUE_FPSTACK;
			}
		}
	}
	if (code_returns == RETURNVALUE_FPSTACK)
	{
		if (supportedReturnValues & (RETURNVALUE_BOOL | RETURNVALUE_BOOL_REVERSED))
		{
			int32_t stubsize;
			void *stub;
			if (supportedReturnValues & RETURNVALUE_BOOL_REVERSED)
			{
				if (rvType) *rvType = RETURNVALUE_BOOL_REVERSED;
				stub = GLUE_realAddress(nseel_asm_fptobool_rev, nseel_asm_fptobool_rev_end, &stubsize);
			}
			else
			{
				if (rvType) *rvType = RETURNVALUE_BOOL;
				stub = GLUE_realAddress(nseel_asm_fptobool, nseel_asm_fptobool_end, &stubsize);
			}
			if (!stub || bufOut_len < stubsize) RET_MINUS1_FAIL(stub ? "fptobool size" : "fptobool addr")
				if (bufOut)
				{
					memcpy(bufOut, stub, stubsize);
					bufOut += stubsize;
				}
			codesz += stubsize;
			bufOut_len -= stubsize;
		}
		else if (supportedReturnValues & RETURNVALUE_NORMAL)
		{
			if (computTableSize) (*computTableSize)++;
			if (bufOut_len < GLUE_POP_FPSTACK_TO_WTP_TO_PX_SIZE) RET_MINUS1_FAIL("popfpstacktowtptopxsize")
				// generate fp-pop to temp space
				if (bufOut) GLUE_POP_FPSTACK_TO_WTP_TO_PX(bufOut, 0);
			codesz += GLUE_POP_FPSTACK_TO_WTP_TO_PX_SIZE;
			if (rvType) *rvType = RETURNVALUE_NORMAL;
		}
		else
		{
			// toss return value that will be ignored
			if (bufOut_len < GLUE_POP_FPSTACK_SIZE) RET_MINUS1_FAIL("popfpstack size")
				if (bufOut) memcpy(bufOut, GLUE_POP_FPSTACK, GLUE_POP_FPSTACK_SIZE);
			codesz += GLUE_POP_FPSTACK_SIZE;
		}
	}
	return codesz;
}
//------------------------------------------------------------------------------
NSEEL_CODEHANDLE NSEEL_code_compile(NSEEL_VMCTX _ctx, const char *_expression, int32_t lineoffs)
{
	return NSEEL_code_compile_ex(_ctx, _expression, lineoffs, 0);
}
typedef struct topLevelCodeSegmentRec {
	struct topLevelCodeSegmentRec *_next;
	void *code;
	int32_t codesz;
	int32_t tmptable_use;
} topLevelCodeSegmentRec;
NSEEL_CODEHANDLE NSEEL_code_compile_ex(NSEEL_VMCTX _ctx, const char *_expression, int32_t lineoffs, int32_t compile_flags)
{
	compileContext *ctx = (compileContext*)_ctx;
	const char *endptr;
	const char *_expression_end;
	codeHandleType *handle;
	topLevelCodeSegmentRec *startpts_tail = NULL;
	topLevelCodeSegmentRec *startpts = NULL;
	_codeHandleFunctionRec *oldCommonFunctionList;
	int32_t curtabptr_sz = 0;
	void *curtabptr = NULL;
	int32_t had_err = 0;
	if (!ctx) return 0;
	ctx->directValueCache = 0;
	ctx->gotEndOfInput = 0;
	if (compile_flags & NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS_RESET)
	{
		ctx->functions_common = NULL; // reset common function list
	}
	else
	{
		// reset common compiled function code, forcing a recompile if shared
		_codeHandleFunctionRec *a = ctx->functions_common;
		while (a)
		{
			_codeHandleFunctionRec *b = a->derivedCopies;
			if (a->localstorage)
			{
				// force local storage actual values to be reallocated if used again
				memset(a->localstorage, 0, sizeof(float *) * a->localstorage_size);
			}
			a->startptr = NULL; // force this copy to be recompiled
			while (b)
			{
				b->startptr = NULL; // force derived copies to get recompiled
				// no need to reset b->localstorage, since it points to a->localstorage
				b = b->derivedCopies;
			}
			a = a->next;
		}
	}
	ctx->last_error_string[0] = 0;
	if (!_expression || !*_expression) return 0;
	_expression_end = _expression + strlen(_expression);
	oldCommonFunctionList = ctx->functions_common;
	ctx->isGeneratingCommonFunction = 0;
	ctx->isSharedFunctions = !!(compile_flags & NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
	ctx->functions_local = NULL;
	freeBlocks(&ctx->tmpblocks_head);  // free blocks
	freeBlocks(&ctx->blocks_head);  // free blocks
	freeBlocks(&ctx->blocks_head_data);  // free blocks
	memset(ctx->l_stats, 0, sizeof(ctx->l_stats));
	handle = (codeHandleType*)newDataBlock(sizeof(codeHandleType), 8);
	if (!handle)
	{
		return 0;
	}
	memset(handle, 0, sizeof(codeHandleType));
	ctx->l_stats[0] += (int32_t)(_expression_end - _expression);
	ctx->tmpCodeHandle = handle;
	endptr = _expression;
	while (*endptr)
	{
		int32_t computTableTop = 0;
		int32_t startptr_size = 0;
		void *startptr = NULL;
		opcodeRec *start_opcode = NULL;
		const char *expr = endptr;
		int32_t function_numparms = 0;
		char is_fname[NSEEL_MAX_VARIABLE_NAMELEN + 1];
		is_fname[0] = 0;
		memset(ctx->function_localTable_Size, 0, sizeof(ctx->function_localTable_Size));
		memset(ctx->function_localTable_Names, 0, sizeof(ctx->function_localTable_Names));
		ctx->function_localTable_ValuePtrs = 0;
		ctx->function_usesNamespaces = 0;
		ctx->function_curName = NULL;
		ctx->function_globalFlag = 0;
		ctx->errVar = 0;
		// single out top level segment
		{
			int32_t had_something = 0, pcnt = 0, pcnt2 = 0;
			int32_t state = 0;
			for (;;)
			{
				int32_t l;
				const char *p = nseel_simple_tokenizer(&endptr, _expression_end, &l, &state);
				if (!p)
				{
					if (pcnt || pcnt2) ctx->gotEndOfInput |= 4;
					break;
				}
				if (*p == ';')
				{
					if (had_something && !pcnt && !pcnt2) break;
				}
				else if (*p == '/' && l > 1 && (p[1] == '/' || p[1] == '*'))
				{
					if (l > 19 && !strncmp(p, "//#eel-no-optimize:", 19))
					{
						//0 = atoi(p + 19);
					}
				}
				else
				{
					if (!had_something)
					{
						expr = p;
						had_something = 1;
					}
					if (*p == '(') pcnt++;
					else if (*p == ')') { if (--pcnt < 0) pcnt = 0; }
					else if (*p == '[') pcnt2++;
					else if (*p == ']') { if (--pcnt2 < 0) pcnt2 = 0; }
				}
			}
			if (!*expr || !had_something) break;
		}
		// parse   
		{
			int32_t tmplen, funcname_len;
			const char *p = expr;
			const char *tok1 = nseel_simple_tokenizer(&p, endptr, &tmplen, NULL);
			const char *funcname = nseel_simple_tokenizer(&p, endptr, &funcname_len, NULL);
			if (tok1 && funcname && tmplen == 8 && !strncmp(tok1, "function", 8) && (isalpha(funcname[0]) || funcname[0] == '_'))
			{
				int32_t had_parms_locals = 0;
				if (funcname_len > (int32_t)(sizeof(is_fname) - 1))
					funcname_len = (int32_t)(sizeof(is_fname) - 1);
				memcpy(is_fname, funcname, funcname_len);
				is_fname[funcname_len] = 0;
				ctx->function_curName = is_fname; // only assigned for the duration of the loop, cleared later //-V507
				while (NULL != (tok1 = nseel_simple_tokenizer(&p, endptr, &tmplen, NULL)))
				{
					int32_t is_parms = 0, localTableContext = 0;
					int32_t maxcnt = 0;
					const char *sp_save;
					if (tok1[0] == '(')
					{
						if (had_parms_locals)
						{
							expr = p - 1; // begin compilation at this code!
							break;
						}
						is_parms = 1;
					}
					else
					{
						if (tmplen == 5 && !strncmp(tok1, "local", tmplen)) localTableContext = 0;
						else if (tmplen == 6 && !strncmp(tok1, "static", tmplen)) localTableContext = 0;
						else if (tmplen == 8 && !strncmp(tok1, "instance", tmplen)) localTableContext = 1;
						else if ((tmplen == 7 && !strncmp(tok1, "globals", tmplen)) ||
							(tmplen == 6 && !strncmp(tok1, "global", tmplen)))
						{
							ctx->function_globalFlag = 1;
							localTableContext = 2;
						}
						else break; // unknown token!
						tok1 = nseel_simple_tokenizer(&p, endptr, &tmplen, NULL);
						if (!tok1 || tok1[0] != '(') break;
					}
					had_parms_locals = 1;
					sp_save = p;
					while (NULL != (tok1 = nseel_simple_tokenizer(&p, endptr, &tmplen, NULL)))
					{
						if (tok1[0] == ')') break;
						if (*tok1 == '#' && localTableContext != 1 && localTableContext != 2)
						{
							ctx->errVar = (int32_t)(tok1 - _expression);
							lstrcpyn_safe(ctx->last_error_string, "#string can only be in instance() or globals()", sizeof(ctx->last_error_string));
							goto had_error;
						}
						if (isalpha(*tok1) || *tok1 == '_' || *tok1 == '#')
						{
							maxcnt++;
							if (p < endptr && *p == '*')
							{
								if (!is_parms && localTableContext != 2)
								{
									ctx->errVar = (int32_t)(p - _expression);
									lstrcpyn_safe(ctx->last_error_string, "namespace* can only be used in parameters or globals()", sizeof(ctx->last_error_string));
									goto had_error;
								}
								p++;
							}
						}
						else if (*tok1 != ',')
						{
							ctx->errVar = (int32_t)(tok1 - _expression);
							lstrcpyn_safe(ctx->last_error_string, "unknown character in function parameters", sizeof(ctx->last_error_string));
							goto had_error;
						}
					}
					if (tok1 && maxcnt > 0)
					{
						char **ot = ctx->function_localTable_Names[localTableContext];
						const int32_t osz = ctx->function_localTable_Size[localTableContext];
						maxcnt += osz;
						ctx->function_localTable_Names[localTableContext] = (char **)newTmpBlock(ctx, sizeof(char *) * maxcnt);
						if (ctx->function_localTable_Names[localTableContext])
						{
							int32_t i = osz;
							if (osz && ot) memcpy(ctx->function_localTable_Names[localTableContext], ot, sizeof(char *) * osz);
							p = sp_save;
							while (NULL != (tok1 = nseel_simple_tokenizer(&p, endptr, &tmplen, NULL)))
							{
								if (tok1[0] == ')') break;
								if (isalpha(*tok1) || *tok1 == '_' || *tok1 == '#')
								{
									char *newstr;
									int32_t l = tmplen;
									if (*p == '*')  // xyz* for namespace
									{
										p++;
										l++;
									}
									if (l > NSEEL_MAX_VARIABLE_NAMELEN) l = NSEEL_MAX_VARIABLE_NAMELEN;
									newstr = newTmpBlock(ctx, l + 1);
									if (newstr)
									{
										memcpy(newstr, tok1, l);
										newstr[l] = 0;
										ctx->function_localTable_Names[localTableContext][i++] = newstr;
									}
								}
							}
							ctx->function_localTable_Size[localTableContext] = i;
							if (is_parms) function_numparms = i;
						}
					}
				}
			}
		}
		if (ctx->function_localTable_Size[0] > 0)
		{
			ctx->function_localTable_ValuePtrs =
				ctx->isSharedFunctions ? newDataBlock(ctx->function_localTable_Size[0] * sizeof(float *), 8) :
				newTmpBlock(ctx, ctx->function_localTable_Size[0] * sizeof(float *));
			if (!ctx->function_localTable_ValuePtrs)
			{
				ctx->function_localTable_Size[0] = 0;
				function_numparms = 0;
			}
			else
			{
				memset(ctx->function_localTable_ValuePtrs, 0, sizeof(float *) * ctx->function_localTable_Size[0]); // force values to be allocated
			}
		}
		{
			int32_t nseelparse(compileContext* context);
			void nseelrestart(void *input_file, void *yyscanner);
			ctx->rdbuf_start = _expression;
			ctx->rdbuf = expr;
			ctx->rdbuf_end = endptr;
			if (!nseelparse(ctx) && !ctx->errVar)
				start_opcode = ctx->result;
			ctx->rdbuf = NULL;
		}
		if (start_opcode)
		{
			int32_t rvMode = 0, fUse = 0;
			optimizeOpcodes(ctx, start_opcode, is_fname[0] ? 1 : 0);
			startptr_size = compileOpcodes(ctx, start_opcode, NULL, 1024 * 1024 * 256, NULL, NULL,
				is_fname[0] ? (RETURNVALUE_NORMAL | RETURNVALUE_FPSTACK) : RETURNVALUE_IGNORE, &rvMode, &fUse, NULL); // if not a function, force return value as address (avoid having to pop it ourselves
			  // if a function, allow the code to decide how return values are generated
			if (is_fname[0])
			{
				_codeHandleFunctionRec *fr = ctx->isSharedFunctions ? newDataBlock(sizeof(_codeHandleFunctionRec), 8) :
					newTmpBlock(ctx, sizeof(_codeHandleFunctionRec));
				if (fr)
				{
					memset(fr, 0, sizeof(_codeHandleFunctionRec));
					fr->startptr_size = startptr_size;
					fr->opcodes = start_opcode;
					fr->rvMode = rvMode;
					fr->fpStackUsage = fUse;
					fr->tmpspace_req = computTableTop;
					if (ctx->function_localTable_Size[0] > 0 && ctx->function_localTable_ValuePtrs)
					{
						if (ctx->function_localTable_Names[0])
						{
							int32_t i;
							for (i = 0; i < function_numparms; i++)
							{
								const char *nptr = ctx->function_localTable_Names[0][i];
								if (nptr && *nptr && nptr[strlen(nptr) - 1] == '*')
								{
									fr->parameterAsNamespaceMask |= ((uint32_t)1) << i;
								}
							}
						}
						fr->num_params = function_numparms;
						fr->localstorage = ctx->function_localTable_ValuePtrs;
						fr->localstorage_size = ctx->function_localTable_Size[0];
					}
					fr->usesNamespaces = ctx->function_usesNamespaces;
					fr->isCommonFunction = ctx->isSharedFunctions;
					lstrcpyn_safe(fr->fname, is_fname, sizeof(fr->fname));
					if (ctx->isSharedFunctions)
					{
						fr->next = ctx->functions_common;
						ctx->functions_common = fr;
					}
					else
					{
						fr->next = ctx->functions_local;
						ctx->functions_local = fr;
					}
				}
				continue;
			}
			if (!startptr_size) continue; // optimized away
			if (startptr_size > 0)
			{
				startptr = newTmpBlock(ctx, startptr_size);
				if (startptr)
				{
					startptr_size = compileOpcodes(ctx, start_opcode, (unsigned char*)startptr, startptr_size, &computTableTop, NULL, RETURNVALUE_IGNORE, NULL, NULL, NULL);
					if (startptr_size <= 0) startptr = NULL;
				}
			}
		}
		if (!startptr)
		{
		had_error:
			//if (!ctx->last_error_string[0])
			{
				int32_t byteoffs = ctx->errVar;
				int32_t linenumber;
				char cur_err[sizeof(ctx->last_error_string)];
				lstrcpyn_safe(cur_err, ctx->last_error_string, sizeof(cur_err));
				if (cur_err[0]) lstrcatn(cur_err, ": ", sizeof(cur_err));
				else lstrcpyn_safe(cur_err, "syntax error: ", sizeof(cur_err));
				if (_expression + byteoffs >= _expression_end)
				{
					if (ctx->gotEndOfInput & 4) byteoffs = (int32_t)(expr - _expression);
					else byteoffs = (int32_t)(_expression_end - _expression);
				}
				if (byteoffs < 0) byteoffs = 0;
				linenumber = findLineNumber(_expression, byteoffs) + 1;
				if (ctx->gotEndOfInput & 4)
				{
					stbsp_snprintf(ctx->last_error_string, sizeof(ctx->last_error_string), "%d: %smissing ) or ]", linenumber + lineoffs, cur_err);
				}
				else
				{
					const char *p = _expression + byteoffs;
					int32_t x = 0, right_amt_nospace = 0, left_amt_nospace = 0;
					while (x < 32 && p - x > _expression && p[-x] != '\r' && p[-x] != '\n')
					{
						if (!isspace(p[-x])) left_amt_nospace = x;
						x++;
					}
					x = 0;
					while (x < 60 && p[x] && p[x] != '\r' && p[x] != '\n')
					{
						if (!isspace(p[x])) right_amt_nospace = x;
						x++;
					}
					if (right_amt_nospace < 1) right_amt_nospace = 1;
					// display left_amt >>>> right_amt_nospace
					if (left_amt_nospace > 0)
						stbsp_snprintf(ctx->last_error_string, sizeof(ctx->last_error_string), "%d: %s'%.*s <!> %.*s'", linenumber + lineoffs, cur_err,
							left_amt_nospace, p - left_amt_nospace,
							right_amt_nospace, p);
					else
						stbsp_snprintf(ctx->last_error_string, sizeof(ctx->last_error_string), "%d: %s'%.*s'", linenumber + lineoffs, cur_err, right_amt_nospace, p);
				}
			}
			startpts = NULL;
			startpts_tail = NULL;
			had_err = 1;
			break;
		}
		if (!is_fname[0]) // redundant check (if is_fname[0] is set and we succeeded, it should continue)
						  // but we'll be on the safe side
		{
			topLevelCodeSegmentRec *p = newTmpBlock(ctx, sizeof(topLevelCodeSegmentRec));
			p->_next = 0;
			p->code = startptr;
			p->codesz = startptr_size;
			p->tmptable_use = computTableTop;
			if (!startpts_tail) startpts_tail = startpts = p;
			else
			{
				startpts_tail->_next = p;
				startpts_tail = p;
			}
			if (curtabptr_sz < computTableTop)
			{
				curtabptr_sz = computTableTop;
			}
		}
	}
	memset(ctx->function_localTable_Size, 0, sizeof(ctx->function_localTable_Size));
	memset(ctx->function_localTable_Names, 0, sizeof(ctx->function_localTable_Names));
	ctx->function_localTable_ValuePtrs = 0;
	ctx->function_usesNamespaces = 0;
	ctx->function_curName = NULL;
	ctx->function_globalFlag = 0;
	ctx->tmpCodeHandle = NULL;
	if (startpts)
	{
		curtabptr_sz += 2; // many functions use the worktable for temporary storage of up to 2 float's
		handle->workTable_size = curtabptr_sz;
		handle->workTable = curtabptr = newDataBlock((curtabptr_sz + MIN_COMPUTABLE_SIZE + COMPUTABLE_EXTRA_SPACE) * sizeof(float), 32);
		if (!curtabptr) startpts = NULL;
	}
	if (startpts || (!had_err && (compile_flags & NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS)))
	{
		unsigned char *writeptr;
		topLevelCodeSegmentRec *p = startpts;
		int32_t size = sizeof(GLUE_RET); // for ret at end :)
		int32_t wtpos = 0;
		// now we build one big code segment out of our list of them, inserting a mov esi, computable before each item as necessary
		while (p)
		{
			if (wtpos <= 0)
			{
				wtpos = MIN_COMPUTABLE_SIZE;
				size += GLUE_RESET_WTP(NULL, 0);
			}
			size += p->codesz;
			wtpos -= p->tmptable_use;
			p = p->_next;
		}
		handle->code = newCodeBlock(size, 32);
		if (handle->code)
		{
			writeptr = (unsigned char *)handle->code;
			p = startpts;
			wtpos = 0;
			while (p)
			{
				if (wtpos <= 0)
				{
					wtpos = MIN_COMPUTABLE_SIZE;
					writeptr += GLUE_RESET_WTP(writeptr, curtabptr);
				}
				memcpy(writeptr, (char*)p->code, p->codesz);
				writeptr += p->codesz;
				wtpos -= p->tmptable_use;
				p = p->_next;
			}
			memcpy(writeptr, &GLUE_RET, sizeof(GLUE_RET)); writeptr += sizeof(GLUE_RET);
			ctx->l_stats[1] = size;
			handle->code_size = (int32_t)(writeptr - (unsigned char *)handle->code);
#ifdef __arm__
			__clear_cache(handle->code, writeptr);
#endif
		}
		handle->blocks = ctx->blocks_head;
		handle->blocks_data = ctx->blocks_head_data;
		ctx->blocks_head = 0;
		ctx->blocks_head_data = 0;
	}
	else
	{
		// failed compiling, or failed calloc()
		handle = NULL;              // return NULL (after resetting blocks_head)
	}
	ctx->directValueCache = 0;
	ctx->functions_local = NULL;
	ctx->isGeneratingCommonFunction = 0;
	ctx->isSharedFunctions = 0;
	freeBlocks(&ctx->tmpblocks_head);  // free blocks
	freeBlocks(&ctx->blocks_head);  // free blocks of code (will be nonzero only on error)
	freeBlocks(&ctx->blocks_head_data);  // free blocks of data (will be nonzero only on error)
	if (handle)
	{
		handle->ramPtr = ctx->ram_state;
		memcpy(handle->code_stats, ctx->l_stats, sizeof(ctx->l_stats));
		nseel_evallib_stats[0] += ctx->l_stats[0];
		nseel_evallib_stats[1] += ctx->l_stats[1];
		nseel_evallib_stats[2] += ctx->l_stats[2];
		nseel_evallib_stats[3] += ctx->l_stats[3];
		nseel_evallib_stats[4]++;
	}
	else
	{
		ctx->functions_common = oldCommonFunctionList; // failed compiling, remove any added common functions from the list
		// remove any derived copies of functions due to error, since we may have added some that have been freed
		while (oldCommonFunctionList)
		{
			oldCommonFunctionList->derivedCopies = NULL;
			oldCommonFunctionList = oldCommonFunctionList->next;
		}
	}
	memset(ctx->l_stats, 0, sizeof(ctx->l_stats));
	return (NSEEL_CODEHANDLE)handle;
}
//------------------------------------------------------------------------------
void NSEEL_code_execute(NSEEL_CODEHANDLE code)
{
	codeHandleType *h = (codeHandleType *)code;
	INT_PTR codeptr = (INT_PTR)h->code;
	INT_PTR tabptr = (INT_PTR)h->workTable;
	GLUE_CALL_CODE(tabptr, codeptr, (INT_PTR)h->ramPtr);
}
int32_t NSEEL_code_geterror_flag(NSEEL_VMCTX ctx)
{
	compileContext *c = (compileContext*)ctx;
	if (c) return (c->gotEndOfInput ? 1 : 0);
	return 0;
}
char *NSEEL_code_getcodeerror(NSEEL_VMCTX ctx)
{
	compileContext *c = (compileContext*)ctx;
	if (ctx && c->last_error_string[0]) return c->last_error_string;
	return 0;
}
//------------------------------------------------------------------------------
void NSEEL_code_free(NSEEL_CODEHANDLE code)
{
	codeHandleType *h = (codeHandleType *)code;
	if (h != NULL)
	{
		nseel_evallib_stats[0] -= h->code_stats[0];
		nseel_evallib_stats[1] -= h->code_stats[1];
		nseel_evallib_stats[2] -= h->code_stats[2];
		nseel_evallib_stats[3] -= h->code_stats[3];
		nseel_evallib_stats[4]--;
		freeBlocks(&h->blocks);
		freeBlocks(&h->blocks_data);
	}
}
//------------------------------------------------------------------------------
void NSEEL_VM_freevars(NSEEL_VMCTX _ctx)
{
	if (_ctx)
	{
		compileContext *ctx = (compileContext*)_ctx;
		free(ctx->varTable_Values);
		free(ctx->varTable_Names);
		ctx->varTable_Values = 0;
		ctx->varTable_Names = 0;
		ctx->varTable_numBlocks = 0;
		if (ctx->region_context)
		{
			FreeRegion_context(ctx->region_context);
			free(ctx->region_context);
			ctx->region_context = 0;
		}
	}
}
void NSEEL_init_memRegion(NSEEL_VMCTX _ctx)
{
	if (_ctx)
	{
		compileContext *ctx = (compileContext*)_ctx;
		ctx->region_context = (eel_builtin_memRegion*)malloc(sizeof(eel_builtin_memRegion));
		InitRegion_context(ctx->region_context);
	}
}
NSEEL_VMCTX NSEEL_VM_alloc() // return a handle
{
	// Context
	compileContext *ctx = (compileContext*)calloc(1, sizeof(compileContext));
	if (ctx)
	{
		ctx->caller_this = ctx;
		ctx->scanner = ctx;
		ctx->onString = addStringCallback;
		NSEEL_init_memRegion((NSEEL_VMCTX)ctx);
	}
	return ctx;
}
void NSEEL_VM_SetFunctionValidator(NSEEL_VMCTX _ctx, const char * (*validateFunc)(const char *fn_name, void *user), void *user)
{
	if (_ctx)
	{
		compileContext *ctx = (compileContext*)_ctx;
		ctx->func_check = validateFunc;
		ctx->func_check_user = user;
	}
}
void NSEEL_VM_free(NSEEL_VMCTX _ctx) // free when done with a VM and ALL of its code have been freed, as well
{
	if (_ctx)
	{
		compileContext *ctx = (compileContext*)_ctx;
		NSEEL_VM_freevars(_ctx);
		freeBlocks(&ctx->pblocks);
		// these should be 0 normally but just in case
		freeBlocks(&ctx->tmpblocks_head);  // free blocks
		freeBlocks(&ctx->blocks_head);  // free blocks
		freeBlocks(&ctx->blocks_head_data);  // free blocks
		ctx->scanner = 0;
		free(ctx);
	}
}
int32_t *NSEEL_code_getstats(NSEEL_CODEHANDLE code)
{
	codeHandleType *h = (codeHandleType *)code;
	if (h)
	{
		return h->code_stats;
	}
	return 0;
}
void NSEEL_VM_SetStringFunc(NSEEL_VMCTX ctx, float(*onString)(void *caller_this, eelStringSegmentRec *list))
{
	if (ctx)
	{
		compileContext *c = (compileContext*)ctx;
		c->onString = onString;
	}
}
void *NSEEL_PProc_RAM(void *data, int32_t data_size, compileContext *ctx)
{
	if (data_size > 0) data = EEL_GLUE_set_immediate(data, (INT_PTR)ctx->ram_state);
	return data;
}
void *NSEEL_PProc_THIS(void *data, int32_t data_size, compileContext *ctx)
{
	if (data_size > 0) data = EEL_GLUE_set_immediate(data, (INT_PTR)ctx->caller_this);
	return data;
}
void NSEEL_VM_remove_unused_vars(NSEEL_VMCTX _ctx)
{
	compileContext *ctx = (compileContext*)_ctx;
	int32_t wb;
	if (ctx) for (wb = 0; wb < ctx->varTable_numBlocks; wb++)
	{
		int32_t ti;
		char **plist = ctx->varTable_Names[wb];
		if (!plist) break;
		for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti++)
		{
			if (plist[ti])
			{
				varNameHdr *v = ((varNameHdr*)plist[ti]) - 1;
				if (!v->refcnt && !v->isreg)
				{
					plist[ti] = NULL;
				}
			}
		}
	}
}
void NSEEL_VM_remove_all_nonreg_vars(NSEEL_VMCTX _ctx)
{
	compileContext *ctx = (compileContext*)_ctx;
	int32_t wb;
	if (ctx) for (wb = 0; wb < ctx->varTable_numBlocks; wb++)
	{
		int32_t ti;
		char **plist = ctx->varTable_Names[wb];
		if (!plist) break;
		for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti++)
		{
			if (plist[ti])
			{
				varNameHdr *v = ((varNameHdr*)plist[ti]) - 1;
				if (!v->isreg)
				{
					plist[ti] = NULL;
				}
			}
		}
	}
}
void NSEEL_VM_clear_var_refcnts(NSEEL_VMCTX _ctx)
{
	compileContext *ctx = (compileContext*)_ctx;
	int32_t wb;
	if (ctx) for (wb = 0; wb < ctx->varTable_numBlocks; wb++)
	{
		int32_t ti;
		char **plist = ctx->varTable_Names[wb];
		if (!plist) break;
		for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti++)
		{
			if (plist[ti])
			{
				varNameHdr *v = ((varNameHdr*)plist[ti]) - 1;
				v->refcnt = 0;
			}
		}
	}
}
float *nseel_int_register_var(compileContext *ctx, const char *name, int32_t isReg, const char **namePtrOut)
{
	int32_t match_wb = -1, match_ti = -1;
	int32_t wb;
	int32_t ti = 0;
	for (wb = 0; wb < ctx->varTable_numBlocks; wb++)
	{
		char **plist = ctx->varTable_Names[wb];
		if (!plist) return NULL; // error!
		for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti++)
		{
			if (!plist[ti])
			{
				if (match_wb < 0)
				{
					match_wb = wb;
					match_ti = ti;
				}
			}
			else if (!strncmp(plist[ti], name, NSEEL_MAX_VARIABLE_NAMELEN))
			{
				varNameHdr *v = ((varNameHdr*)plist[ti]) - 1;
				if (isReg < 0)
				{
					float *p;
					return (ctx->varTable_Values && NULL != (p = ctx->varTable_Values[wb])) ? p + ti : NULL;
				}
				v->refcnt++;
				if (isReg) v->isreg = isReg;
				if (namePtrOut) *namePtrOut = plist[ti];
				break;
			}
		}
		if (ti < NSEEL_VARS_PER_BLOCK) break;
	}
	if (isReg < 0) return NULL;
	if (wb == ctx->varTable_numBlocks && match_wb >= 0 && match_ti >= 0)
	{
		wb = match_wb;
		ti = match_ti;
	}
	if (wb == ctx->varTable_numBlocks)
	{
		ti = 0;
		// add new block
		if (!(ctx->varTable_numBlocks&(NSEEL_VARS_MALLOC_CHUNKSIZE - 1)) || !ctx->varTable_Values || !ctx->varTable_Names)
		{
			void *nv = realloc(ctx->varTable_Values, (ctx->varTable_numBlocks + NSEEL_VARS_MALLOC_CHUNKSIZE) * sizeof(float *));
			if (!nv) return NULL;
			ctx->varTable_Values = (float **)nv;
			nv = realloc(ctx->varTable_Names, (ctx->varTable_numBlocks + NSEEL_VARS_MALLOC_CHUNKSIZE) * sizeof(char **));
			if (!nv) return NULL;
			ctx->varTable_Names = (char ***)nv;
		}
		ctx->varTable_numBlocks++;
		ctx->varTable_Values[wb] = (float *)newCtxDataBlock(sizeof(float)*NSEEL_VARS_PER_BLOCK, 8);
		ctx->varTable_Names[wb] = (char **)newCtxDataBlock(sizeof(char *)*NSEEL_VARS_PER_BLOCK, 1);
		if (ctx->varTable_Values[wb])
		{
			memset(ctx->varTable_Values[wb], 0, sizeof(float)*NSEEL_VARS_PER_BLOCK);
		}
		if (ctx->varTable_Names[wb])
		{
			memset(ctx->varTable_Names[wb], 0, sizeof(char *)*NSEEL_VARS_PER_BLOCK);
		}
	}
	if (!ctx->varTable_Names[wb] || !ctx->varTable_Values[wb]) return NULL;
	if (!ctx->varTable_Names[wb][ti])
	{
		size_t l = strlen(name);
		char *b;
		varNameHdr *vh;
		if (l > NSEEL_MAX_VARIABLE_NAMELEN) l = NSEEL_MAX_VARIABLE_NAMELEN;
		b = newCtxDataBlock((int32_t)(sizeof(varNameHdr) + l + 1), 1);
		if (!b) return NULL; // malloc fail
		vh = (varNameHdr *)b;
		vh->refcnt = 1;
		vh->isreg = isReg;
		b += sizeof(varNameHdr);
		memcpy(b, name, l);
		b[l] = 0;
		ctx->varTable_Names[wb][ti] = b;
		ctx->varTable_Values[wb][ti] = 0.0f;
		if (namePtrOut) *namePtrOut = b;
	}
	return ctx->varTable_Values[wb] + ti;
}
//------------------------------------------------------------------------------
void NSEEL_VM_enumallvars(NSEEL_VMCTX ctx, int32_t(*func)(const char *name, float *val, void *ctx), void *userctx)
{
	compileContext *tctx = (compileContext*)ctx;
	int32_t wb;
	if (!tctx) return;
	for (wb = 0; wb < tctx->varTable_numBlocks; wb++)
	{
		int32_t ti;
		char **plist = tctx->varTable_Names[wb];
		if (!plist) break;
		for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti++)
		{
			if (plist[ti] && !func(plist[ti], tctx->varTable_Values[wb] + ti, userctx)) break;
		}
		if (ti < NSEEL_VARS_PER_BLOCK)
			break;
	}
}
//------------------------------------------------------------------------------
float *NSEEL_VM_regvar(NSEEL_VMCTX _ctx, const char *var)
{
	compileContext *ctx = (compileContext*)_ctx;
	if (!ctx) return 0;
	return nseel_int_register_var(ctx, var, 1, NULL);
}
float *NSEEL_VM_getvar(NSEEL_VMCTX _ctx, const char *var)
{
	compileContext *ctx = (compileContext*)_ctx;
	if (!ctx) return 0;
	return nseel_int_register_var(ctx, var, -1, NULL);
}
int32_t  NSEEL_VM_get_var_refcnt(NSEEL_VMCTX _ctx, const char *name)
{
	compileContext *ctx = (compileContext*)_ctx;
	int32_t wb;
	if (!ctx) return -1;
	for (wb = 0; wb < ctx->varTable_numBlocks; wb++)
	{
		int32_t ti;
		if (!ctx->varTable_Values[wb] || !ctx->varTable_Names[wb]) break;
		for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti++)
		{
			if (ctx->varTable_Names[wb][ti] && !stricmp(ctx->varTable_Names[wb][ti], name))
			{
				varNameHdr *h = ((varNameHdr *)ctx->varTable_Names[wb][ti]) - 1;
				return h->refcnt;
			}
		}
	}
	return -1;
}
opcodeRec *nseel_createFunctionByName(compileContext *ctx, const char *name, int32_t np, opcodeRec *code1, opcodeRec *code2, opcodeRec *code3)
{
	int32_t i;
	for (i = 0; fnTable1 + i; i++)
	{
		if (i >= (int32_t)(sizeof(fnTable1) / sizeof(fnTable1[0])))
			break;
		functionType *f = fnTable1 + i;
		if ((f->nParams&FUNCTIONTYPE_PARAMETERCOUNTMASK) == np && !strcmp(f->name, name))
		{
			opcodeRec *o = newOpCode(ctx, NULL, np == 3 ? OPCODETYPE_FUNC3 : np == 2 ? OPCODETYPE_FUNC2 : OPCODETYPE_FUNC1);
			if (o)
			{
				o->fntype = FUNCTYPE_FUNCTIONTYPEREC;
				o->fn = f;
				o->parms.parms[0] = code1;
				o->parms.parms[1] = code2;
				o->parms.parms[2] = code3;
			}
			return o;
		}
	}
	return NULL;
}
#include <float.h>
//------------------------------------------------------------------------------
#define GENCONSTANTIF_ELSE(idx) \
else if (!tmplen ? !stricmp(tmp, xVarName[idx]) : (tmplen == strlen(xVarName[idx]) && !strnicmp(tmp, xVarName[idx], strlen(xVarName[idx])))) \
return nseel_createCompiledValue(ctx, idx);
opcodeRec *nseel_translate(compileContext *ctx, const char *tmp, size_t tmplen) // tmplen 0 = null term
{
	// this depends on the string being nul terminated eventually, tmplen is used more as a hint than anything else
	if ((tmp[0] == '0' || tmp[0] == '$') && toupper(tmp[1]) == 'X')
	{
		char *p;
		return nseel_createCompiledValue(ctx, (float)strtoul(tmp + 2, &p, 16));
	}
	else if (tmp[0] == '$')
	{
		if (tmp[1] == '~')
		{
			char *p = (char*)tmp + 2;
			uint32_t v = strtoul(tmp + 2, &p, 10);
			if (v > 53) v = 53;
			return nseel_createCompiledValue(ctx, (float)((((WDL_INT64)1) << v) - 1));
		}
		else if (!tmplen ? !stricmp(tmp, "$E") : (tmplen == 2 && !strnicmp(tmp, "$E", 2)))
			return nseel_createCompiledValue(ctx, (float)2.71828182845904523536);
		else if (!tmplen ? !stricmp(tmp, "$PI") : (tmplen == 3 && !strnicmp(tmp, "$PI", 3)))
			return nseel_createCompiledValue(ctx, (float)M_PIDouble);
		else if (!tmplen ? !stricmp(tmp, "$PHI") : (tmplen == 4 && !strnicmp(tmp, "$PHI", 4)))
			return nseel_createCompiledValue(ctx, (float)1.618033988749894848205);
		else if (!tmplen ? !stricmp(tmp, "$EPS") : (tmplen == 4 && !strnicmp(tmp, "$EPS", 4)))
			return nseel_createCompiledValue(ctx, (float)FLT_EPSILON);
		GENCONSTANTIF_ELSE(0)
		GENCONSTANTIF_ELSE(1)
		GENCONSTANTIF_ELSE(2)
		GENCONSTANTIF_ELSE(3)
		GENCONSTANTIF_ELSE(4)
		GENCONSTANTIF_ELSE(5)
		GENCONSTANTIF_ELSE(6)
		GENCONSTANTIF_ELSE(7)
		GENCONSTANTIF_ELSE(8)
		GENCONSTANTIF_ELSE(9)
		GENCONSTANTIF_ELSE(10)
		GENCONSTANTIF_ELSE(11)
		GENCONSTANTIF_ELSE(12)
		GENCONSTANTIF_ELSE(13)
		GENCONSTANTIF_ELSE(14)
		GENCONSTANTIF_ELSE(15)
		GENCONSTANTIF_ELSE(16)
		GENCONSTANTIF_ELSE(17)
		GENCONSTANTIF_ELSE(18)
		else if (!tmplen ? !stricmp(tmp, "$MEMBLKLIMIT") : (tmplen == 12 && !strnicmp(tmp, "$MEMBLKLIMIT", 12)))
			return nseel_createCompiledValue(ctx, (float)NSEEL_RAM_ITEMSPERBLOCK);
		else if ((!tmplen || tmplen == 4) && tmp[1] == '\'' && tmp[2] && tmp[3] == '\'')
			return nseel_createCompiledValue(ctx, (float)tmp[2]);
		else return NULL;
	}
	else if (tmp[0] == '\'')
	{
		char b[64];
		int32_t x, sz;
		uint32_t rv = 0;
		if (!tmplen) // nul terminated tmplen, calculate a workable length
		{
			// faster than strlen(tmp) if tmp is large, we'll never need more than ~18 chars anyway
			while (tmplen < 32 && tmp[tmplen]) tmplen++;
		}
		sz = tmplen > 0 ? nseel_filter_escaped_string(b, sizeof(b), tmp + 1, tmplen - 1, '\'') : 0;
		if (sz > 4)
		{
			if (ctx->last_error_string[0]) lstrcatn(ctx->last_error_string, ", ", sizeof(ctx->last_error_string));
			snprintf_append(ctx->last_error_string, sizeof(ctx->last_error_string), "multi-byte character '%.5s...' too long", b);
			return NULL; // do not allow 'xyzxy', limit to 4 bytes
		}
		for (x = 0; x < sz; x++) rv = (rv << 8) + ((unsigned char*)b)[x];
		return nseel_createCompiledValue(ctx, (float)rv);
	}
	return nseel_createCompiledValue(ctx, (float)atof(tmp));
}