#ifndef Jfir_H
#define Jfir_H
void wKaiser(float w[], const int N, float beta);
void JfirLP(float *h, int *N, const int wnd, const float fc, float beta, float gain);
void JfirHP(float *h, int *N, const int wnd, const float fc, float beta, float gain);
void JfirBS(float *h, int *N, const int wnd, const float fc1, const float fc2, float beta, float gain);
void JfirBP(float *h, int *N, const int wnd, const float fc1, const float fc2, float beta, float gain);
#endif
