#ifndef FFT_H_
#define FFT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef fixed
#define fixed short
#endif

int  fix_fft(fixed fr[], fixed fi[], int m, int inverse);
void window(fixed fr[], int n);
void fix_loud(fixed loud[], fixed fr[], fixed fi[], int n, int scale_shift);

#ifdef __cplusplus
}
#endif


#endif /* FFT_H_ */
