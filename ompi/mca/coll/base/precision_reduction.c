#include <stdio.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <math.h>

void conv_DOUBLE_to_FLOAT(double* sbuf, float* tbuf, int pd_iter)
{
    int i;
    for (i = 0; i < pd_iter*4; i += 4){
        __m256d double_vector = _mm256_load_pd(&sbuf[i]);
        __m128 float_vector = _mm256_cvtpd_ps(double_vector);
        _mm_store_ps(&tbuf[i], float_vector);
    }
}

void conv_FLOAT_to_HALF(float* sbuf, char* tbuf, int ps_iter)
{
    int i;
    for (i = 0; i < ps_iter*8; i += 8){
        __m256 float_vector = _mm256_load_ps(&sbuf[i]);
        __m128i half_vector = _mm256_cvtps_ph(float_vector, 0);
        _mm_store_si128((__m128i*)&tbuf[i*2], half_vector);
    }
}

void conv_DOUBLE_to_HALF(double* sbuf, char* tbuf, int pd_iter)
{
    float tmpbuf[pd_iter*8];
    conv_DOUBLE_to_FLOAT(sbuf, tmpbuf, pd_iter);
    conv_FLOAT_to_HALF(tmpbuf, tbuf, pd_iter);
}

void conv_HALF_to_FLOAT(char* sbuf, float* tbuf, int ps_iter)
{
    int i;
    for (i = 0; i < ps_iter*8; i += 8){
        __m128i half_vector = _mm_load_si128((__m128i*)&sbuf[i*2]);
        __m256 float_vector = _mm256_cvtph_ps(half_vector);
        _mm256_store_ps(&tbuf[i], float_vector);
    }
}

void conv_FLOAT_to_DOUBLE(float* sbuf, double* tbuf, int ps_iter)
{
    int i;
    for (i = 0; i < ps_iter*4; i += 4){
        __m128 float_vector = _mm_load_ps(&sbuf[i]);
        __m256d double_vector = _mm256_cvtps_pd(float_vector);
        _mm256_store_pd(&tbuf[i], double_vector);
    }
}

void conv_HALF_to_DOUBLE(char* sbuf, double* tbuf, int ps_iter)
{
    float tmpbuf[ps_iter*8];
    conv_HALF_to_FLOAT(sbuf, tmpbuf, ps_iter);
    conv_FLOAT_to_DOUBLE(tmpbuf, tbuf, ps_iter);
}

