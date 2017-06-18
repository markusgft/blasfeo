/**************************************************************************************************
*                                                                                                 *
* This file is part of BLASFEO.                                                                   *
*                                                                                                 *
* BLASFEO -- BLAS For Embedded Optimization.                                                      *
* Copyright (C) 2016-2017 by Gianluca Frison.                                                     *
* Developed at IMTEK (University of Freiburg) under the supervision of Moritz Diehl.              *
* All rights reserved.                                                                            *
*                                                                                                 *
* HPMPC is free software; you can redistribute it and/or                                          *
* modify it under the terms of the GNU Lesser General Public                                      *
* License as published by the Free Software Foundation; either                                    *
* version 2.1 of the License, or (at your option) any later version.                              *
*                                                                                                 *
* HPMPC is distributed in the hope that it will be useful,                                        *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                                  *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                                            *
* See the GNU Lesser General Public License for more details.                                     *
*                                                                                                 *
* You should have received a copy of the GNU Lesser General Public                                *
* License along with HPMPC; if not, write to the Free Software                                    *
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA                  *
*                                                                                                 *
* Author: Gianluca Frison, giaf (at) dtu.dk                                                       *
*                          gianluca.frison (at) imtek.uni-freiburg.de                             *
*                                                                                                 *
**************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "../include/blasfeo_common.h"
#include "../include/blasfeo_s_kernel.h"
#include "../include/blasfeo_s_aux.h"



#if defined(LA_HIGH_PERFORMANCE)



void sgemv_n_libstr(int m, int n, float alpha, struct s_strmat *sA, int ai, int aj, struct s_strvec *sx, int xi, float beta, struct s_strvec *sy, int yi, struct s_strvec *sz, int zi)
	{

	if(m<0)
		return;

	const int bs = 8;

	int i;

	int sda = sA->cn;
	float *pA = sA->pA + aj*bs + ai/bs*bs*sda;
	float *x = sx->pa + xi;
	float *y = sy->pa + yi;
	float *z = sz->pa + zi;

	i = 0;
	// clean up at the beginning
	if(ai%bs!=0)
		{
		kernel_sgemv_n_8_gen_lib8(n, &alpha, pA, x, &beta, y-ai%bs, z-ai%bs, ai%bs, m+ai%bs);
		pA += bs*sda;
		y += 8 - ai%bs;
		z += 8 - ai%bs;
		m -= 8 - ai%bs;
		}
	// main loop
	for( ; i<m-7; i+=8)
		{
		kernel_sgemv_n_8_lib8(n, &alpha, &pA[i*sda], x, &beta, &y[i], &z[i]);
		}
	if(i<m)
		{
		kernel_sgemv_n_8_vs_lib8(n, &alpha, &pA[i*sda], x, &beta, &y[i], &z[i], m-i);
		}
		
	return;

	}



void sgemv_t_libstr(int m, int n, float alpha, struct s_strmat *sA, int ai, int aj, struct s_strvec *sx, int xi, float beta, struct s_strvec *sy, int yi, struct s_strvec *sz, int zi)
	{

	if(n<=0)
		return;
	
	const int bs = 8;

	int i;

	int sda = sA->cn;
	float *pA = sA->pA + aj*bs + ai/bs*bs*sda + ai%bs;
	float *x = sx->pa + xi;
	float *y = sy->pa + yi;
	float *z = sz->pa + zi;

	if(ai%bs==0)
		{
		i = 0;
		for( ; i<n-7; i+=8)
			{
			kernel_sgemv_t_8_lib8(m, &alpha, &pA[i*bs], sda, x, &beta, &y[i], &z[i]);
			}
		if(i<n)
			{
			kernel_sgemv_t_8_vs_lib8(m, &alpha, &pA[i*bs], sda, x, &beta, &y[i], &z[i], n-i);
			}
		}
	else
		{
		i = 0;
		for( ; i<n; i+=8)
			{
			kernel_sgemv_t_8_gen_lib8(m, &alpha, ai%bs, &pA[i*bs], sda, x, &beta, &y[i], &z[i], n-i);
			}
		}
	
	return;

	}



// m >= n
void strmv_lnn_libstr(int m, int n, struct s_strmat *sA, int ai, int aj, struct s_strvec *sx, int xi, struct s_strvec *sz, int zi)
	{

	if(m<=0)
		return;

	const int bs = 8;

	int sda = sA->cn;
	float *pA = sA->pA + aj*bs + ai/bs*bs*sda + ai%bs;
	float *x = sx->pa + xi;
	float *z = sz->pa + zi;

	if(m-n>0)
		sgemv_n_libstr(m-n, n, 1.0, sA, ai+n, aj, sx, xi, 0.0, sz, zi+n, sz, zi+n);

	float *pA2 = pA;
	float *z2 = z;
	int m2 = n;
	int n2 = 0;
	float *pA3, *x3;

	float alpha = 1.0;
	float beta = 1.0;

	float zt[8];

	int ii, jj, jj_end;

	ii = 0;

	if(ai%4!=0)
		{
		pA2 += sda*bs - ai%bs;
		z2 += bs-ai%bs;
		m2 -= bs-ai%bs;
		n2 += bs-ai%bs;
		}
	
	pA2 += m2/bs*bs*sda;
	z2 += m2/bs*bs;
	n2 += m2/bs*bs;

	if(m2%bs!=0)
		{
		//
		pA3 = pA2 + bs*n2;
		x3 = x + n2;
		zt[7] = pA3[7+bs*0]*x3[0] + pA3[7+bs*1]*x3[1] + pA3[7+bs*2]*x3[2] + pA3[7+bs*3]*x3[3] + pA3[7+bs*4]*x3[4] + pA3[7+bs*5]*x3[5] + pA3[7+bs*6]*x3[6] + pA3[7+bs*7]*x3[7];
		zt[6] = pA3[6+bs*0]*x3[0] + pA3[6+bs*1]*x3[1] + pA3[6+bs*2]*x3[2] + pA3[6+bs*3]*x3[3] + pA3[6+bs*4]*x3[4] + pA3[6+bs*5]*x3[5] + pA3[6+bs*6]*x3[6];
		zt[5] = pA3[5+bs*0]*x3[0] + pA3[5+bs*1]*x3[1] + pA3[5+bs*2]*x3[2] + pA3[5+bs*3]*x3[3] + pA3[5+bs*4]*x3[4] + pA3[5+bs*5]*x3[5];
		zt[4] = pA3[4+bs*0]*x3[0] + pA3[4+bs*1]*x3[1] + pA3[4+bs*2]*x3[2] + pA3[4+bs*3]*x3[3] + pA3[4+bs*4]*x3[4];
		zt[3] = pA3[3+bs*0]*x3[0] + pA3[3+bs*1]*x3[1] + pA3[3+bs*2]*x3[2] + pA3[3+bs*3]*x3[3];
		zt[2] = pA3[2+bs*0]*x3[0] + pA3[2+bs*1]*x3[1] + pA3[2+bs*2]*x3[2];
		zt[1] = pA3[1+bs*0]*x3[0] + pA3[1+bs*1]*x3[1];
		zt[0] = pA3[0+bs*0]*x3[0];
		kernel_sgemv_n_8_lib8(n2, &alpha, pA2, x, &beta, zt, zt);
		for(jj=0; jj<m2%bs; jj++)
			z2[jj] = zt[jj];
		}
	for(; ii<m2-3; ii+=4)
		{
		pA2 -= bs*sda;
		z2 -= 8;
		n2 -= 8;
		pA3 = pA2 + bs*n2;
		x3 = x + n2;
		z2[7] = pA3[7+bs*0]*x3[0] + pA3[7+bs*1]*x3[1] + pA3[7+bs*2]*x3[2] + pA3[7+bs*3]*x3[3] + pA3[7+bs*4]*x3[4] + pA3[7+bs*5]*x3[5] + pA3[7+bs*6]*x3[6] + pA3[7+bs*7]*x3[7];
		z2[6] = pA3[6+bs*0]*x3[0] + pA3[6+bs*1]*x3[1] + pA3[6+bs*2]*x3[2] + pA3[6+bs*3]*x3[3] + pA3[6+bs*4]*x3[4] + pA3[6+bs*5]*x3[5] + pA3[6+bs*6]*x3[6];
		z2[5] = pA3[5+bs*0]*x3[0] + pA3[5+bs*1]*x3[1] + pA3[5+bs*2]*x3[2] + pA3[5+bs*3]*x3[3] + pA3[5+bs*4]*x3[4] + pA3[5+bs*5]*x3[5];
		z2[4] = pA3[4+bs*0]*x3[0] + pA3[4+bs*1]*x3[1] + pA3[4+bs*2]*x3[2] + pA3[4+bs*3]*x3[3] + pA3[4+bs*4]*x3[4];
		z2[3] = pA3[3+bs*0]*x3[0] + pA3[3+bs*1]*x3[1] + pA3[3+bs*2]*x3[2] + pA3[3+bs*3]*x3[3];
		z2[2] = pA3[2+bs*0]*x3[0] + pA3[2+bs*1]*x3[1] + pA3[2+bs*2]*x3[2];
		z2[1] = pA3[1+bs*0]*x3[0] + pA3[1+bs*1]*x3[1];
		z2[0] = pA3[0+bs*0]*x3[0];
		kernel_sgemv_n_8_lib8(n2, &alpha, pA2, x, &beta, z2, z2);
		}
	if(ai%8!=0)
		{
		if(ai%bs==1)
			{
			zt[6] = pA[6+bs*0]*x[0] + pA[6+bs*1]*x[1] + pA[6+bs*2]*x[2] + pA[6+bs*3]*x[3] + pA[6+bs*4]*x[4] + pA[6+bs*5]*x[5] + pA[6+bs*6]*x[6];
			zt[5] = pA[5+bs*0]*x[0] + pA[5+bs*1]*x[1] + pA[5+bs*2]*x[2] + pA[5+bs*3]*x[3] + pA[5+bs*4]*x[4] + pA[5+bs*5]*x[5];
			zt[4] = pA[4+bs*0]*x[0] + pA[4+bs*1]*x[1] + pA[4+bs*2]*x[2] + pA[4+bs*3]*x[3] + pA[4+bs*4]*x[4];
			zt[3] = pA[3+bs*0]*x[0] + pA[3+bs*1]*x[1] + pA[3+bs*2]*x[2] + pA[3+bs*3]*x[3];
			zt[2] = pA[2+bs*0]*x[0] + pA[2+bs*1]*x[1] + pA[2+bs*2]*x[2];
			zt[1] = pA[1+bs*0]*x[0] + pA[1+bs*1]*x[1];
			zt[0] = pA[0+bs*0]*x[0];
			jj_end = 8-ai%bs<n ? 8-ai%bs : n;
			for(jj=0; jj<jj_end; jj++)
				z[jj] = zt[jj];
			}
		else if(ai%bs==2)
			{
			zt[5] = pA[5+bs*0]*x[0] + pA[5+bs*1]*x[1] + pA[5+bs*2]*x[2] + pA[5+bs*3]*x[3] + pA[5+bs*4]*x[4] + pA[5+bs*5]*x[5];
			zt[4] = pA[4+bs*0]*x[0] + pA[4+bs*1]*x[1] + pA[4+bs*2]*x[2] + pA[4+bs*3]*x[3] + pA[4+bs*4]*x[4];
			zt[3] = pA[3+bs*0]*x[0] + pA[3+bs*1]*x[1] + pA[3+bs*2]*x[2] + pA[3+bs*3]*x[3];
			zt[2] = pA[2+bs*0]*x[0] + pA[2+bs*1]*x[1] + pA[2+bs*2]*x[2];
			zt[1] = pA[1+bs*0]*x[0] + pA[1+bs*1]*x[1];
			zt[0] = pA[0+bs*0]*x[0];
			jj_end = 8-ai%bs<n ? 8-ai%bs : n;
			for(jj=0; jj<jj_end; jj++)
				z[jj] = zt[jj];
			}
		else if(ai%bs==3)
			{
			zt[4] = pA[4+bs*0]*x[0] + pA[4+bs*1]*x[1] + pA[4+bs*2]*x[2] + pA[4+bs*3]*x[3] + pA[4+bs*4]*x[4];
			zt[3] = pA[3+bs*0]*x[0] + pA[3+bs*1]*x[1] + pA[3+bs*2]*x[2] + pA[3+bs*3]*x[3];
			zt[2] = pA[2+bs*0]*x[0] + pA[2+bs*1]*x[1] + pA[2+bs*2]*x[2];
			zt[1] = pA[1+bs*0]*x[0] + pA[1+bs*1]*x[1];
			zt[0] = pA[0+bs*0]*x[0];
			jj_end = 8-ai%bs<n ? 8-ai%bs : n;
			for(jj=0; jj<jj_end; jj++)
				z[jj] = zt[jj];
			}
		else if(ai%bs==4)
			{
			zt[3] = pA[3+bs*0]*x[0] + pA[3+bs*1]*x[1] + pA[3+bs*2]*x[2] + pA[3+bs*3]*x[3];
			zt[2] = pA[2+bs*0]*x[0] + pA[2+bs*1]*x[1] + pA[2+bs*2]*x[2];
			zt[1] = pA[1+bs*0]*x[0] + pA[1+bs*1]*x[1];
			zt[0] = pA[0+bs*0]*x[0];
			jj_end = 8-ai%bs<n ? 8-ai%bs : n;
			for(jj=0; jj<jj_end; jj++)
				z[jj] = zt[jj];
			}
		else if(ai%bs==5)
			{
			zt[2] = pA[2+bs*0]*x[0] + pA[2+bs*1]*x[1] + pA[2+bs*2]*x[2];
			zt[1] = pA[1+bs*0]*x[0] + pA[1+bs*1]*x[1];
			zt[0] = pA[0+bs*0]*x[0];
			jj_end = 8-ai%bs<n ? 8-ai%bs : n;
			for(jj=0; jj<jj_end; jj++)
				z[jj] = zt[jj];
			}
		else if(ai%bs==6)
			{
			zt[1] = pA[1+bs*0]*x[0] + pA[1+bs*1]*x[1];
			zt[0] = pA[0+bs*0]*x[0];
			jj_end = 8-ai%bs<n ? 8-ai%bs : n;
			for(jj=0; jj<jj_end; jj++)
				z[jj] = zt[jj];
			}
		else // if (ai%bs==7)
			{
			z[0] = pA[0+bs*0]*x[0];
			}
		}

	return;

	}



// m >= n
void strmv_ltn_libstr(int m, int n, struct s_strmat *sA, int ai, int aj, struct s_strvec *sx, int xi, struct s_strvec *sz, int zi)
	{

	if(m<=0)
		return;

	const int bs = 8;

	int sda = sA->cn;
	float *pA = sA->pA + aj*bs + ai/bs*bs*sda + ai%bs;
	float *x = sx->pa + xi;
	float *z = sz->pa + zi;

	float xt[8];
	float zt[8];

	float alpha = 1.0;
	float beta = 1.0;

	int ii, jj, ll, ll_max;

	jj = 0;

	if(ai%bs!=0)
		{

		if(ai%bs==1)
			{
			ll_max = m-jj<7 ? m-jj : 7;
			for(ll=0; ll<ll_max; ll++)
				xt[ll] = x[ll];
			for(; ll<7; ll++)
				xt[ll] = 0.0;
			zt[0] = pA[0+bs*0]*xt[0] + pA[1+bs*0]*xt[1] + pA[2+bs*0]*xt[2] + pA[3+bs*0]*xt[3] + pA[4+bs*0]*xt[4] + pA[5+bs*0]*xt[5] + pA[6+bs*0]*xt[6];
			zt[1] = pA[1+bs*1]*xt[1] + pA[2+bs*1]*xt[2] + pA[3+bs*1]*xt[3] + pA[4+bs*1]*xt[4] + pA[5+bs*1]*xt[5] + pA[6+bs*1]*xt[6];
			zt[2] = pA[2+bs*2]*xt[2] + pA[3+bs*2]*xt[3] + pA[4+bs*2]*xt[4] + pA[5+bs*2]*xt[5] + pA[6+bs*2]*xt[6];
			zt[3] = pA[3+bs*3]*xt[3] + pA[4+bs*3]*xt[4] + pA[5+bs*3]*xt[5] + pA[6+bs*3]*xt[6];
			zt[4] = pA[4+bs*4]*xt[4] + pA[5+bs*4]*xt[5] + pA[6+bs*4]*xt[6];
			zt[5] = pA[5+bs*5]*xt[5] + pA[6+bs*5]*xt[6];
			zt[6] = pA[6+bs*6]*xt[6];
			pA += bs*sda - 1;
			x += 7;
			kernel_sgemv_t_8_lib8(m-7-jj, &alpha, pA, sda, x, &beta, zt, zt);
			ll_max = n-jj<7 ? n-jj : 7;
			for(ll=0; ll<ll_max; ll++)
				z[ll] = zt[ll];
			pA += bs*7;
			z += 7;
			jj += 7;
			}
		else if(ai%bs==2)
			{
			ll_max = m-jj<6 ? m-jj : 6;
			for(ll=0; ll<ll_max; ll++)
				xt[ll] = x[ll];
			for(; ll<6; ll++)
				xt[ll] = 0.0;
			zt[0] = pA[0+bs*0]*xt[0] + pA[1+bs*0]*xt[1] + pA[2+bs*0]*xt[2] + pA[3+bs*0]*xt[3] + pA[4+bs*0]*xt[4] + pA[5+bs*0]*xt[5];
			zt[1] = pA[1+bs*1]*xt[1] + pA[2+bs*1]*xt[2] + pA[3+bs*1]*xt[3] + pA[4+bs*1]*xt[4] + pA[5+bs*1]*xt[5];
			zt[2] = pA[2+bs*2]*xt[2] + pA[3+bs*2]*xt[3] + pA[4+bs*2]*xt[4] + pA[5+bs*2]*xt[5];
			zt[3] = pA[3+bs*3]*xt[3] + pA[4+bs*3]*xt[4] + pA[5+bs*3]*xt[5];
			zt[4] = pA[4+bs*4]*xt[4] + pA[5+bs*4]*xt[5];
			zt[5] = pA[5+bs*5]*xt[5];
			pA += bs*sda - 2;
			x += 6;
			kernel_sgemv_t_8_lib8(m-6-jj, &alpha, pA, sda, x, &beta, zt, zt);
			ll_max = n-jj<6 ? n-jj : 6;
			for(ll=0; ll<ll_max; ll++)
				z[ll] = zt[ll];
			pA += bs*6;
			z += 6;
			jj += 6;
			}
		else if(ai%bs==3)
			{
			ll_max = m-jj<5 ? m-jj : 5;
			for(ll=0; ll<ll_max; ll++)
				xt[ll] = x[ll];
			for(; ll<5; ll++)
				xt[ll] = 0.0;
			zt[0] = pA[0+bs*0]*xt[0] + pA[1+bs*0]*xt[1] + pA[2+bs*0]*xt[2] + pA[3+bs*0]*xt[3] + pA[4+bs*0]*xt[4];
			zt[1] = pA[1+bs*1]*xt[1] + pA[2+bs*1]*xt[2] + pA[3+bs*1]*xt[3] + pA[4+bs*1]*xt[4];
			zt[2] = pA[2+bs*2]*xt[2] + pA[3+bs*2]*xt[3] + pA[4+bs*2]*xt[4];
			zt[3] = pA[3+bs*3]*xt[3] + pA[4+bs*3]*xt[4];
			zt[4] = pA[4+bs*4]*xt[4];
			pA += bs*sda - 3;
			x += 5;
			kernel_sgemv_t_8_lib8(m-5-jj, &alpha, pA, sda, x, &beta, zt, zt);
			ll_max = n-jj<5 ? n-jj : 5;
			for(ll=0; ll<ll_max; ll++)
				z[ll] = zt[ll];
			pA += bs*5;
			z += 5;
			jj += 5;
			}
		else if(ai%bs==4)
			{
			ll_max = m-jj<4 ? m-jj : 4;
			for(ll=0; ll<ll_max; ll++)
				xt[ll] = x[ll];
			for(; ll<4; ll++)
				xt[ll] = 0.0;
			zt[0] = pA[0+bs*0]*xt[0] + pA[1+bs*0]*xt[1] + pA[2+bs*0]*xt[2] + pA[3+bs*0]*xt[3];
			zt[1] = pA[1+bs*1]*xt[1] + pA[2+bs*1]*xt[2] + pA[3+bs*1]*xt[3];
			zt[2] = pA[2+bs*2]*xt[2] + pA[3+bs*2]*xt[3];
			zt[3] = pA[3+bs*3]*xt[3];
			pA += bs*sda - 4;
			x += 4;
			kernel_sgemv_t_8_lib8(m-4-jj, &alpha, pA, sda, x, &beta, zt, zt);
			ll_max = n-jj<4 ? n-jj : 4;
			for(ll=0; ll<ll_max; ll++)
				z[ll] = zt[ll];
			pA += bs*4;
			z += 4;
			jj += 4;
			}
		else if(ai%bs==5)
			{
			ll_max = m-jj<3 ? m-jj : 3;
			for(ll=0; ll<ll_max; ll++)
				xt[ll] = x[ll];
			for(; ll<3; ll++)
				xt[ll] = 0.0;
			zt[0] = pA[0+bs*0]*xt[0] + pA[1+bs*0]*xt[1] + pA[2+bs*0]*xt[2];
			zt[1] = pA[1+bs*1]*xt[1] + pA[2+bs*1]*xt[2];
			zt[2] = pA[2+bs*2]*xt[2];
			pA += bs*sda - 5;
			x += 3;
			kernel_sgemv_t_8_lib8(m-3-jj, &alpha, pA, sda, x, &beta, zt, zt);
			ll_max = n-jj<3 ? n-jj : 3;
			for(ll=0; ll<ll_max; ll++)
				z[ll] = zt[ll];
			pA += bs*3;
			z += 3;
			jj += 3;
			}
		else if(ai%bs==6)
			{
			ll_max = m-jj<2 ? m-jj : 2;
			for(ll=0; ll<ll_max; ll++)
				xt[ll] = x[ll];
			for(; ll<2; ll++)
				xt[ll] = 0.0;
			zt[0] = pA[0+bs*0]*xt[0] + pA[1+bs*0]*xt[1];
			zt[1] = pA[1+bs*1]*xt[1];
			pA += bs*sda - 6;
			x += 2;
			kernel_sgemv_t_8_lib8(m-2-jj, &alpha, pA, sda, x, &beta, zt, zt);
			ll_max = n-jj<2 ? n-jj : 2;
			for(ll=0; ll<ll_max; ll++)
				z[ll] = zt[ll];
			pA += bs*2;
			z += 2;
			jj += 2;
			}
		else // if(ai%bs==7)
			{
			ll_max = m-jj<1 ? m-jj : 1;
			for(ll=0; ll<ll_max; ll++)
				xt[ll] = x[ll];
			for(; ll<1; ll++)
				xt[ll] = 0.0;
			zt[0] = pA[0+bs*0]*xt[0];
			pA += bs*sda - 7;
			x += 1;
			kernel_sgemv_t_8_lib8(m-1-jj, &alpha, pA, sda, x, &beta, zt, zt);
			ll_max = n-jj<1 ? n-jj : 1;
			for(ll=0; ll<ll_max; ll++)
				z[ll] = zt[ll];
			pA += bs*1;
			z += 1;
			jj += 1;
			}

		}
	
	for(; jj<n-3; jj+=4)
		{
		zt[0] = pA[0+bs*0]*xt[0] + pA[1+bs*0]*xt[1] + pA[2+bs*0]*xt[2] + pA[3+bs*0]*xt[3] + pA[4+bs*0]*xt[4] + pA[5+bs*0]*xt[5] + pA[6+bs*0]*xt[6] + pA[7+bs*0]*xt[7];
		zt[1] = pA[1+bs*1]*xt[1] + pA[2+bs*1]*xt[2] + pA[3+bs*1]*xt[3] + pA[4+bs*1]*xt[4] + pA[5+bs*1]*xt[5] + pA[6+bs*1]*xt[6] + pA[7+bs*1]*xt[7];
		zt[2] = pA[2+bs*2]*xt[2] + pA[3+bs*2]*xt[3] + pA[4+bs*2]*xt[4] + pA[5+bs*2]*xt[5] + pA[6+bs*2]*xt[6] + pA[7+bs*2]*xt[7];
		zt[3] = pA[3+bs*3]*xt[3] + pA[4+bs*3]*xt[4] + pA[5+bs*3]*xt[5] + pA[6+bs*3]*xt[6] + pA[7+bs*3]*xt[7];
		zt[4] = pA[4+bs*4]*xt[4] + pA[5+bs*4]*xt[5] + pA[6+bs*4]*xt[6] + pA[7+bs*4]*xt[7];
		zt[5] = pA[5+bs*5]*xt[5] + pA[6+bs*5]*xt[6] + pA[7+bs*5]*xt[7];
		zt[6] = pA[6+bs*6]*xt[6] + pA[7+bs*6]*xt[7];
		zt[7] = pA[7+bs*7]*xt[7];
		pA += bs*sda;
		x += 8;
		kernel_sgemv_t_8_lib8(m-8-jj, &alpha, pA, sda, x, &beta, zt, z);
		pA += bs*8;
		z += 8;
		}
	if(jj<n)
		{
		ll_max = m-jj<8 ? m-jj : 8;
		for(ll=0; ll<ll_max; ll++)
			xt[ll] = x[ll];
		for(; ll<8; ll++)
			xt[ll] = 0.0;
		zt[0] = pA[0+bs*0]*xt[0] + pA[1+bs*0]*xt[1] + pA[2+bs*0]*xt[2] + pA[3+bs*0]*xt[3] + pA[4+bs*0]*xt[4] + pA[5+bs*0]*xt[5] + pA[6+bs*0]*xt[6] + pA[7+bs*0]*xt[7];
		zt[1] = pA[1+bs*1]*xt[1] + pA[2+bs*1]*xt[2] + pA[3+bs*1]*xt[3] + pA[4+bs*1]*xt[4] + pA[5+bs*1]*xt[5] + pA[6+bs*1]*xt[6] + pA[7+bs*1]*xt[7];
		zt[2] = pA[2+bs*2]*xt[2] + pA[3+bs*2]*xt[3] + pA[4+bs*2]*xt[4] + pA[5+bs*2]*xt[5] + pA[6+bs*2]*xt[6] + pA[7+bs*2]*xt[7];
		zt[3] = pA[3+bs*3]*xt[3] + pA[4+bs*3]*xt[4] + pA[5+bs*3]*xt[5] + pA[6+bs*3]*xt[6] + pA[7+bs*3]*xt[7];
		zt[4] = pA[4+bs*4]*xt[4] + pA[5+bs*4]*xt[5] + pA[6+bs*4]*xt[6] + pA[7+bs*4]*xt[7];
		zt[5] = pA[5+bs*5]*xt[5] + pA[6+bs*5]*xt[6] + pA[7+bs*5]*xt[7];
		zt[6] = pA[6+bs*6]*xt[6] + pA[7+bs*6]*xt[7];
		zt[7] = pA[7+bs*7]*xt[7];
		pA += bs*sda;
		x += 8;
		kernel_sgemv_t_8_lib8(m-8-jj, &alpha, pA, sda, x, &beta, zt, zt);
		for(ll=0; ll<n-jj; ll++)
			z[ll] = zt[ll];
//		pA += bs*8;
//		z += 8;
		}

	return;

	}



#else

#error : wrong LA choice

#endif
