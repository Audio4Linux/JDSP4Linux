#ifndef Jfir_H
#define Jfir_H
void wKaiser(float w[], unsigned int N, float beta);
void JfirLP(float *h, unsigned int *N, unsigned int wnd, const float fc, float beta, float gain);
void JfirHP(float *h, unsigned int *N, unsigned int wnd, const float fc, float beta, float gain);
void JfirBS(float *h, unsigned int *N, unsigned int wnd, const float fc1, const float fc2, float beta, float gain);
void JfirBP(float *h, unsigned int *N, unsigned int wnd, const float fc1, const float fc2, float beta, float gain);
#endif
