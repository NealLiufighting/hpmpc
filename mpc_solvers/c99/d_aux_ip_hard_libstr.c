/**************************************************************************************************
*                                                                                                 *
* This file is part of HPMPC.                                                                     *
*                                                                                                 *
* HPMPC -- Library for High-Performance implementation of solvers for MPC.                        *
* Copyright (C) 2014-2015 by Technical University of Denmark. All rights reserved.                *
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
*                                                                                                 *
**************************************************************************************************/

#include <math.h>

#ifdef BLASFEO

#include <blasfeo_target.h>
#include <blasfeo_common.h>
#include <blasfeo_d_aux.h>
#include <blasfeo_d_blas.h>

#include "../../include/block_size.h" // TODO remove !!!!!



// initialize variables

void d_init_var_mpc_hard_libstr(int N, int *nx, int *nu, int *nb, int **hidxb, int *ng, struct d_strvec *hsux, struct d_strvec *hspi, struct d_strmat *hsDCt, struct d_strvec *hsdb, struct d_strvec *hst, struct d_strvec *hslam, double mu0, int warm_start)
	{

	int jj, ll, ii;

	double *hux[N+1];
	double *hpi[N+1];
	double *hdb[N+1];
	double *ht[N+1];
	double *hlam[N+1];
	for(ii=0; ii<=N; ii++)
		{
		hux[ii] = hsux[ii].pa;
		hpi[ii] = hspi[ii].pa;
		hdb[ii] = hsdb[ii].pa;
		ht[ii] = hst[ii].pa;
		hlam[ii] = hslam[ii].pa;
		}

	int nb0, ng0;
	
	double
		*ptr_t, *ptr_lam, *ptr_db;

	double thr0 = 0.1; // minimum vale of t (minimum distance from a constraint)


	// cold start
	if(warm_start==0)
		{
		for(jj=0; jj<=N; jj++)
			{
			for(ll=0; ll<nu[jj]+nx[jj]; ll++)
				{
				hux[jj][ll] = 0.0;
				}
			}
		}


	// check bounds & initialize multipliers
	for(jj=0; jj<=N; jj++)
		{
		nb0 = nb[jj];
		for(ll=0; ll<nb0; ll++)
			{
			ht[jj][ll]     = - hdb[jj][ll]     + hux[jj][hidxb[jj][ll]];
			ht[jj][nb0+ll] =   hdb[jj][nb0+ll] - hux[jj][hidxb[jj][ll]];
			if(ht[jj][ll] < thr0)
				{
				if(ht[jj][nb0+ll] < thr0)
					{
					hux[jj][hidxb[jj][ll]] = ( - hdb[jj][nb0+ll] + hdb[jj][ll])*0.5;
					ht[jj][ll]     = thr0; //- hdb[jj][ll]     + hux[jj][hidxb[jj][ll]];
					ht[jj][nb0+ll] = thr0; //  hdb[jj][nb0+ll] - hux[jj][hidxb[jj][ll]];
					}
				else
					{
					ht[jj][ll] = thr0;
					hux[jj][hidxb[jj][ll]] = hdb[jj][ll] + thr0;
					}
				}
			else if(ht[jj][nb0+ll] < thr0)
				{
				ht[jj][nb0+ll] = thr0;
				hux[jj][hidxb[jj][ll]] = hdb[jj][nb0+ll] - thr0;
				}
			hlam[jj][ll]     = mu0/ht[jj][ll];
			hlam[jj][nb0+ll] = mu0/ht[jj][nb0+ll];
			}
		}


	// initialize pi
	for(jj=1; jj<=N; jj++)
		for(ll=0; ll<nx[jj]; ll++)
			hpi[jj][ll] = 0.0; // initialize multipliers to zero


	// TODO find a better way to initialize general constraints
	for(jj=0; jj<=N; jj++)
		{
		nb0 = nb[jj];
		ng0 = ng[jj];
		if(ng0>0)
			{
			ptr_t   = ht[jj];
			ptr_lam = hlam[jj];
			ptr_db  = hdb[jj];
			dgemv_t_libstr(nu[jj]+nx[jj], ng0, 1.0, &hsDCt[jj], 0, 0, &hsux[jj], 0, 0.0, &hst[jj], 2*nb0, &hst[jj], 2*nb0);
			for(ll=2*nb0; ll<2*nb0+ng0; ll++)
				{
				ptr_t[ll+ng0] = - ptr_t[ll];
				ptr_t[ll]     -= ptr_db[ll];
				ptr_t[ll+ng0] += ptr_db[ll+ng0];
				ptr_t[ll]     = fmax( thr0, ptr_t[ll] );
				ptr_t[ng0+ll] = fmax( thr0, ptr_t[ng0+ll] );
				ptr_lam[ll]     = mu0/ptr_t[ll];
				ptr_lam[ng0+ll] = mu0/ptr_t[ng0+ll];
				}
			}
		}

	}



// IPM with no residuals

void d_update_hessian_mpc_hard_libstr(int N, int *nx, int *nu, int *nb, int *ng, struct d_strvec *hsdb, double sigma_mu, struct d_strvec *hst, struct d_strvec *hstinv, struct d_strvec *hslam, struct d_strvec *hslamt, struct d_strvec *hsdlam, struct d_strvec *hsQx, struct d_strvec *hsqx)
	{
	
	int ii, jj, bs0;
	
	int nb0, ng0;
	
	double temp0, temp1;
	
	double 
		*ptr_db, *ptr_Qx, *ptr_qx,
		*ptr_t, *ptr_lam, *ptr_lamt, *ptr_dlam, *ptr_tinv;
	
	for(jj=0; jj<=N; jj++)
		{
		
		ptr_t     = hst[jj].pa;
		ptr_lam   = hslam[jj].pa;
		ptr_lamt  = hslamt[jj].pa;
		ptr_dlam  = hsdlam[jj].pa;
		ptr_tinv  = hstinv[jj].pa;
		ptr_db    = hsdb[jj].pa;
		ptr_Qx    = hsQx[jj].pa;
		ptr_qx    = hsqx[jj].pa;

		// box constraints
		nb0 = nb[jj];
		if(nb0>0)
			{

			for(ii=0; ii<nb0-3; ii+=4)
				{

				ptr_tinv[ii+0] = 1.0/ptr_t[ii+0];
				ptr_tinv[ii+nb0+0] = 1.0/ptr_t[ii+nb0+0];
				ptr_lamt[ii+0] = ptr_lam[ii+0]*ptr_tinv[ii+0];
				ptr_lamt[ii+nb0+0] = ptr_lam[ii+nb0+0]*ptr_tinv[ii+nb0+0];
				ptr_dlam[ii+0] = ptr_tinv[ii+0]*sigma_mu; // !!!!!
				ptr_dlam[ii+nb0+0] = ptr_tinv[ii+nb0+0]*sigma_mu; // !!!!!
				ptr_Qx[ii+0] = ptr_lamt[ii+0] + ptr_lamt[ii+nb0+0];
				ptr_qx[ii+0] = ptr_lam[ii+nb0+0] - ptr_lamt[ii+nb0+0]*ptr_db[ii+nb0+0] + ptr_dlam[ii+nb0+0] - ptr_lam[ii+0] - ptr_lamt[ii+0]*ptr_db[ii+0] - ptr_dlam[ii+0];

				ptr_tinv[ii+1] = 1.0/ptr_t[ii+1];
				ptr_tinv[ii+nb0+1] = 1.0/ptr_t[ii+nb0+1];
				ptr_lamt[ii+1] = ptr_lam[ii+1]*ptr_tinv[ii+1];
				ptr_lamt[ii+nb0+1] = ptr_lam[ii+nb0+1]*ptr_tinv[ii+nb0+1];
				ptr_dlam[ii+1] = ptr_tinv[ii+1]*sigma_mu; // !!!!!
				ptr_dlam[ii+nb0+1] = ptr_tinv[ii+nb0+1]*sigma_mu; // !!!!!
				ptr_Qx[ii+1] = ptr_lamt[ii+1] + ptr_lamt[ii+nb0+1];
				ptr_qx[ii+1] = ptr_lam[ii+nb0+1] - ptr_lamt[ii+nb0+1]*ptr_db[ii+nb0+1] + ptr_dlam[ii+nb0+1] - ptr_lam[ii+1] - ptr_lamt[ii+1]*ptr_db[ii+1] - ptr_dlam[ii+1];

				ptr_tinv[ii+2] = 1.0/ptr_t[ii+2];
				ptr_tinv[ii+nb0+2] = 1.0/ptr_t[ii+nb0+2];
				ptr_lamt[ii+2] = ptr_lam[ii+2]*ptr_tinv[ii+2];
				ptr_lamt[ii+nb0+2] = ptr_lam[ii+nb0+2]*ptr_tinv[ii+nb0+2];
				ptr_dlam[ii+2] = ptr_tinv[ii+2]*sigma_mu; // !!!!!
				ptr_dlam[ii+nb0+2] = ptr_tinv[ii+nb0+2]*sigma_mu; // !!!!!
				ptr_Qx[ii+2] = ptr_lamt[ii+2] + ptr_lamt[ii+nb0+2];
				ptr_qx[ii+2] = ptr_lam[ii+nb0+2] - ptr_lamt[ii+nb0+2]*ptr_db[ii+nb0+2] + ptr_dlam[ii+nb0+2] - ptr_lam[ii+2] - ptr_lamt[ii+2]*ptr_db[ii+2] - ptr_dlam[ii+2];

				ptr_tinv[ii+3] = 1.0/ptr_t[ii+3];
				ptr_tinv[ii+nb0+3] = 1.0/ptr_t[ii+nb0+3];
				ptr_lamt[ii+3] = ptr_lam[ii+3]*ptr_tinv[ii+3];
				ptr_lamt[ii+nb0+3] = ptr_lam[ii+nb0+3]*ptr_tinv[ii+nb0+3];
				ptr_dlam[ii+3] = ptr_tinv[ii+3]*sigma_mu; // !!!!!
				ptr_dlam[ii+nb0+3] = ptr_tinv[ii+nb0+3]*sigma_mu; // !!!!!
				ptr_Qx[ii+3] = ptr_lamt[ii+3] + ptr_lamt[ii+nb0+3];
				ptr_qx[ii+3] = ptr_lam[ii+nb0+3] - ptr_lamt[ii+nb0+3]*ptr_db[ii+nb0+3] + ptr_dlam[ii+nb0+3] - ptr_lam[ii+3] - ptr_lamt[ii+3]*ptr_db[ii+3] - ptr_dlam[ii+3];

				}
			for(; ii<nb0; ii++)
				{

				ptr_tinv[ii+0] = 1.0/ptr_t[ii+0];
				ptr_tinv[ii+nb0+0] = 1.0/ptr_t[ii+nb0+0];
				ptr_lamt[ii+0] = ptr_lam[ii+0]*ptr_tinv[ii+0];
				ptr_lamt[ii+nb0+0] = ptr_lam[ii+nb0+0]*ptr_tinv[ii+nb0+0];
				ptr_dlam[ii+0] = ptr_tinv[ii+0]*sigma_mu; // !!!!!
				ptr_dlam[ii+nb0+0] = ptr_tinv[ii+nb0+0]*sigma_mu; // !!!!!
				ptr_Qx[ii] = ptr_lamt[ii+0] + ptr_lamt[ii+nb0+0];
				ptr_qx[ii] = ptr_lam[ii+nb0+0] - ptr_lamt[ii+nb0+0]*ptr_db[ii+nb0+0] + ptr_dlam[ii+nb0+0] - ptr_lam[ii+0] - ptr_lamt[ii+0]*ptr_db[ii+0] - ptr_dlam[ii+0];

				}

			ptr_t     += 2*nb0;
			ptr_lam   += 2*nb0;
			ptr_lamt  += 2*nb0;
			ptr_dlam  += 2*nb0;
			ptr_tinv  += 2*nb0;
			ptr_db    += 2*nb0;
			ptr_Qx    += nb0;
			ptr_qx    += nb0;

			}

		// general constraints
		ng0 = ng[jj];
		if(ng0>0)
			{

			for(ii=0; ii<ng0-3; ii+=4)
				{

				ptr_tinv[ii+0] = 1.0/ptr_t[ii+0];
				ptr_tinv[ii+ng0+0] = 1.0/ptr_t[ii+ng0+0];
				ptr_lamt[ii+0] = ptr_lam[ii+0]*ptr_tinv[ii+0];
				ptr_lamt[ii+ng0+0] = ptr_lam[ii+ng0+0]*ptr_tinv[ii+ng0+0];
				ptr_dlam[ii+0] = ptr_tinv[ii+0]*sigma_mu; // !!!!!
				ptr_dlam[ii+ng0+0] = ptr_tinv[ii+ng0+0]*sigma_mu; // !!!!!
				ptr_Qx[ii+0] = ptr_lamt[ii+0] + ptr_lamt[ii+ng0+0];
				ptr_qx[ii+0] =  ptr_lam[ii+ng0+0] - ptr_lamt[ii+ng0+0]*ptr_db[ii+ng0+0] + ptr_dlam[ii+ng0+0] - ptr_lam[ii+0] - ptr_lamt[ii+0]*ptr_db[ii+0] - ptr_dlam[ii+0];

				ptr_tinv[ii+1] = 1.0/ptr_t[ii+1];
				ptr_tinv[ii+ng0+1] = 1.0/ptr_t[ii+ng0+1];
				ptr_lamt[ii+1] = ptr_lam[ii+1]*ptr_tinv[ii+1];
				ptr_lamt[ii+ng0+1] = ptr_lam[ii+ng0+1]*ptr_tinv[ii+ng0+1];
				ptr_dlam[ii+1] = ptr_tinv[ii+1]*sigma_mu; // !!!!!
				ptr_dlam[ii+ng0+1] = ptr_tinv[ii+ng0+1]*sigma_mu; // !!!!!
				ptr_Qx[ii+1] = ptr_lamt[ii+1] + ptr_lamt[ii+ng0+1];
				ptr_qx[ii+1] =  ptr_lam[ii+ng0+1] - ptr_lamt[ii+ng0+1]*ptr_db[ii+ng0+1] + ptr_dlam[ii+ng0+1] - ptr_lam[ii+1] - ptr_lamt[ii+1]*ptr_db[ii+1] - ptr_dlam[ii+1];

				ptr_tinv[ii+2] = 1.0/ptr_t[ii+2];
				ptr_tinv[ii+ng0+2] = 1.0/ptr_t[ii+ng0+2];
				ptr_lamt[ii+2] = ptr_lam[ii+2]*ptr_tinv[ii+2];
				ptr_lamt[ii+ng0+2] = ptr_lam[ii+ng0+2]*ptr_tinv[ii+ng0+2];
				ptr_dlam[ii+2] = ptr_tinv[ii+2]*sigma_mu; // !!!!!
				ptr_dlam[ii+ng0+2] = ptr_tinv[ii+ng0+2]*sigma_mu; // !!!!!
				ptr_Qx[ii+2] = ptr_lamt[ii+2] + ptr_lamt[ii+ng0+2];
				ptr_qx[ii+2] =  ptr_lam[ii+ng0+2] - ptr_lamt[ii+ng0+2]*ptr_db[ii+ng0+2] + ptr_dlam[ii+ng0+2] - ptr_lam[ii+2] - ptr_lamt[ii+2]*ptr_db[ii+2] - ptr_dlam[ii+2];

				ptr_tinv[ii+3] = 1.0/ptr_t[ii+3];
				ptr_tinv[ii+ng0+3] = 1.0/ptr_t[ii+ng0+3];
				ptr_lamt[ii+3] = ptr_lam[ii+3]*ptr_tinv[ii+3];
				ptr_lamt[ii+ng0+3] = ptr_lam[ii+ng0+3]*ptr_tinv[ii+ng0+3];
				ptr_dlam[ii+3] = ptr_tinv[ii+3]*sigma_mu; // !!!!!
				ptr_dlam[ii+ng0+3] = ptr_tinv[ii+ng0+3]*sigma_mu; // !!!!!
				ptr_Qx[ii+3] = ptr_lamt[ii+3] + ptr_lamt[ii+ng0+3];
				ptr_qx[ii+3] =  ptr_lam[ii+ng0+3] - ptr_lamt[ii+ng0+3]*ptr_db[ii+ng0+3] + ptr_dlam[ii+ng0+3] - ptr_lam[ii+3] - ptr_lamt[ii+3]*ptr_db[ii+3] - ptr_dlam[ii+3];

				}
			for(; ii<ng0; ii++)
				{

				ptr_tinv[ii+0] = 1.0/ptr_t[ii+0];
				ptr_tinv[ii+ng0+0] = 1.0/ptr_t[ii+ng0+0];
				ptr_lamt[ii+0] = ptr_lam[ii+0]*ptr_tinv[ii+0];
				ptr_lamt[ii+ng0+0] = ptr_lam[ii+ng0+0]*ptr_tinv[ii+ng0+0];
				ptr_dlam[ii+0] = ptr_tinv[ii+0]*sigma_mu; // !!!!!
				ptr_dlam[ii+ng0+0] = ptr_tinv[ii+ng0+0]*sigma_mu; // !!!!!
				ptr_Qx[ii+0] = ptr_lamt[ii+0] + ptr_lamt[ii+ng0+0];
				ptr_qx[ii+0] =  ptr_lam[ii+ng0+0] - ptr_lamt[ii+ng0+0]*ptr_db[ii+ng0+0] + ptr_dlam[ii+ng0+0] - ptr_lam[ii+0] - ptr_lamt[ii+0]*ptr_db[ii+0] - ptr_dlam[ii+0];

				}

			}

		}

	}



void d_update_gradient_mpc_hard_libstr(int N, int *nx, int *nu, int *nb, int *ng, double sigma_mu, struct d_strvec *hsdt, struct d_strvec *hsdlam, struct d_strvec *hstinv, struct d_strvec *hsqx)
	{

	int ii, jj;

	int nb0, ng0;

	double
		*ptr_dlam, *ptr_t_inv, *ptr_dt, *ptr_pl2, *ptr_qx;

	for(jj=0; jj<=N; jj++)
		{

		ptr_dlam  = hsdlam[jj].pa;
		ptr_dt    = hsdt[jj].pa;
		ptr_t_inv = hstinv[jj].pa;
		ptr_qx    = hsqx[jj].pa;

		// box constraints
		nb0 = nb[jj];
		if(nb0>0)
			{

			for(ii=0; ii<nb0-3; ii+=4)
				{
				ptr_dlam[0*nb0+ii+0] = ptr_t_inv[0*nb0+ii+0]*(sigma_mu - ptr_dlam[0*nb0+ii+0]*ptr_dt[0*nb0+ii+0]);
				ptr_dlam[1*nb0+ii+0] = ptr_t_inv[1*nb0+ii+0]*(sigma_mu - ptr_dlam[1*nb0+ii+0]*ptr_dt[1*nb0+ii+0]);
				ptr_qx[ii+0] += ptr_dlam[1*nb0+ii+0] - ptr_dlam[0*nb0+ii+0];

				ptr_dlam[0*nb0+ii+1] = ptr_t_inv[0*nb0+ii+1]*(sigma_mu - ptr_dlam[0*nb0+ii+1]*ptr_dt[0*nb0+ii+1]);
				ptr_dlam[1*nb0+ii+1] = ptr_t_inv[1*nb0+ii+1]*(sigma_mu - ptr_dlam[1*nb0+ii+1]*ptr_dt[1*nb0+ii+1]);
				ptr_qx[ii+1] += ptr_dlam[1*nb0+ii+1] - ptr_dlam[0*nb0+ii+1];

				ptr_dlam[0*nb0+ii+2] = ptr_t_inv[0*nb0+ii+2]*(sigma_mu - ptr_dlam[0*nb0+ii+2]*ptr_dt[0*nb0+ii+2]);
				ptr_dlam[1*nb0+ii+2] = ptr_t_inv[1*nb0+ii+2]*(sigma_mu - ptr_dlam[1*nb0+ii+2]*ptr_dt[1*nb0+ii+2]);
				ptr_qx[ii+2] += ptr_dlam[1*nb0+ii+2] - ptr_dlam[0*nb0+ii+2];

				ptr_dlam[0*nb0+ii+3] = ptr_t_inv[0*nb0+ii+3]*(sigma_mu - ptr_dlam[0*nb0+ii+3]*ptr_dt[0*nb0+ii+3]);
				ptr_dlam[1*nb0+ii+3] = ptr_t_inv[1*nb0+ii+3]*(sigma_mu - ptr_dlam[1*nb0+ii+3]*ptr_dt[1*nb0+ii+3]);
				ptr_qx[ii+3] += ptr_dlam[1*nb0+ii+3] - ptr_dlam[0*nb0+ii+3];
				}
			for(; ii<nb0; ii++)
				{
				ptr_dlam[0*nb0+ii+0] = ptr_t_inv[0*nb0+ii+0]*(sigma_mu - ptr_dlam[0*nb0+ii+0]*ptr_dt[0*nb0+ii+0]);
				ptr_dlam[1*nb0+ii+0] = ptr_t_inv[1*nb0+ii+0]*(sigma_mu - ptr_dlam[1*nb0+ii+0]*ptr_dt[1*nb0+ii+0]);
				ptr_qx[ii+0] += ptr_dlam[1*nb0+ii+0] - ptr_dlam[0*nb0+ii+0];
				}

			ptr_dlam  += 2*nb0;
			ptr_dt    += 2*nb0;
			ptr_t_inv += 2*nb0;
			ptr_qx    += nb0;

			}

		// general constraints
		ng0 = ng[jj];
		if(ng0>0)
			{

			for(ii=0; ii<ng0-3; ii+=4)
				{
				ptr_dlam[0*ng0+ii+0] = ptr_t_inv[0*ng0+ii+0]*(sigma_mu - ptr_dlam[0*ng0+ii+0]*ptr_dt[0*ng0+ii+0]);
				ptr_dlam[1*ng0+ii+0] = ptr_t_inv[1*ng0+ii+0]*(sigma_mu - ptr_dlam[1*ng0+ii+0]*ptr_dt[1*ng0+ii+0]);
				ptr_qx[ii+0] += ptr_dlam[1*ng0+ii+0] - ptr_dlam[0*ng0+ii+0];

				ptr_dlam[0*ng0+ii+1] = ptr_t_inv[0*ng0+ii+1]*(sigma_mu - ptr_dlam[0*ng0+ii+1]*ptr_dt[0*ng0+ii+1]);
				ptr_dlam[1*ng0+ii+1] = ptr_t_inv[1*ng0+ii+1]*(sigma_mu - ptr_dlam[1*ng0+ii+1]*ptr_dt[1*ng0+ii+1]);
				ptr_qx[ii+1] += ptr_dlam[1*ng0+ii+1] - ptr_dlam[0*ng0+ii+1];

				ptr_dlam[0*ng0+ii+2] = ptr_t_inv[0*ng0+ii+2]*(sigma_mu - ptr_dlam[0*ng0+ii+2]*ptr_dt[0*ng0+ii+2]);
				ptr_dlam[1*ng0+ii+2] = ptr_t_inv[1*ng0+ii+2]*(sigma_mu - ptr_dlam[1*ng0+ii+2]*ptr_dt[1*ng0+ii+2]);
				ptr_qx[ii+2] += ptr_dlam[1*ng0+ii+2] - ptr_dlam[0*ng0+ii+2];

				ptr_dlam[0*ng0+ii+3] = ptr_t_inv[0*ng0+ii+3]*(sigma_mu - ptr_dlam[0*ng0+ii+3]*ptr_dt[0*ng0+ii+3]);
				ptr_dlam[1*ng0+ii+3] = ptr_t_inv[1*ng0+ii+3]*(sigma_mu - ptr_dlam[1*ng0+ii+3]*ptr_dt[1*ng0+ii+3]);
				ptr_qx[ii+3] += ptr_dlam[1*ng0+ii+3] - ptr_dlam[0*ng0+ii+3];

				}
			for(; ii<ng0; ii++)
				{
				ptr_dlam[0*ng0+ii+0] = ptr_t_inv[0*ng0+ii+0]*(sigma_mu - ptr_dlam[0*ng0+ii+0]*ptr_dt[0*ng0+ii+0]);
				ptr_dlam[1*ng0+ii+0] = ptr_t_inv[1*ng0+ii+0]*(sigma_mu - ptr_dlam[1*ng0+ii+0]*ptr_dt[1*ng0+ii+0]);
				ptr_qx[ii+0] += ptr_dlam[1*ng0+ii+0] - ptr_dlam[0*ng0+ii+0];
				}

			}

		}

	}



void d_compute_alpha_mpc_hard_libstr(int N, int *nx, int *nu, int *nb, int **idxb, int *ng, double *ptr_alpha, struct d_strvec *hst, struct d_strvec *hsdt, struct d_strvec *hslam, struct d_strvec *hsdlam, struct d_strvec *hslamt, struct d_strvec *hsdux, struct d_strmat *hsDCt, struct d_strvec *hsdb)
	{
	
	int nu0, nx0, nb0, ng0;

	double alpha = ptr_alpha[0];
	
	double
		*ptr_db, *ptr_dux, *ptr_t, *ptr_dt, *ptr_lamt, *ptr_lam, *ptr_dlam;
	
	int
		*ptr_idxb;
	
	int jj, ll;

	for(jj=0; jj<=N; jj++)
		{

		ptr_db   = hsdb[jj].pa;
		ptr_dux  = hsdux[jj].pa;
		ptr_t    = hst[jj].pa;
		ptr_dt   = hsdt[jj].pa;
		ptr_lamt = hslamt[jj].pa;
		ptr_lam  = hslam[jj].pa;
		ptr_dlam = hsdlam[jj].pa;
		ptr_idxb = idxb[jj];

		// box constraints
		nb0 = nb[jj];
		if(nb0>0)
			{

			// box constraints
			for(ll=0; ll<nb0; ll++)
				{

				ptr_dt[ll+0]   =   ptr_dux[ptr_idxb[ll]] - ptr_db[ll+0]   - ptr_t[ll+0];
				ptr_dt[ll+nb0] = - ptr_dux[ptr_idxb[ll]] + ptr_db[ll+nb0] - ptr_t[ll+nb0];
				ptr_dlam[ll+0]   -= ptr_lamt[ll+0]   * ptr_dt[ll+0]   + ptr_lam[ll+0];
				ptr_dlam[ll+nb0] -= ptr_lamt[ll+nb0] * ptr_dt[ll+nb0] + ptr_lam[ll+nb0];
				if( -alpha*ptr_dlam[ll+0]>ptr_lam[ll+0] )
					{
					alpha = - ptr_lam[ll+0] / ptr_dlam[ll+0];
					}
				if( -alpha*ptr_dlam[ll+nb0]>ptr_lam[ll+nb0] )
					{
					alpha = - ptr_lam[ll+nb0] / ptr_dlam[ll+nb0];
					}
				if( -alpha*ptr_dt[ll+0]>ptr_t[ll+0] )
					{
					alpha = - ptr_t[ll+0] / ptr_dt[ll+0];
					}
				if( -alpha*ptr_dt[ll+nb0]>ptr_t[ll+nb0] )
					{
					alpha = - ptr_t[ll+nb0] / ptr_dt[ll+nb0];
					}

				}

			ptr_db   += 2*nb0;
			ptr_t    += 2*nb0;
			ptr_dt   += 2*nb0;
			ptr_lamt += 2*nb0;
			ptr_lam  += 2*nb0;
			ptr_dlam += 2*nb0;

			}

		// general constraints
		ng0 = ng[jj];
		if(ng0>0)
			{

			nu0 = nu[jj];
			nx0 = nx[jj];

			dgemv_t_libstr(nx0+nu0, ng0, 1.0, &hsDCt[jj], 0, 0, &hsdux[jj], 0, 0.0, &hsdt[jj], 0, &hsdt[jj], 0);

			for(ll=0; ll<ng0; ll++)
				{
				ptr_dt[ll+ng0] = - ptr_dt[ll];
				ptr_dt[ll+0]   += - ptr_db[ll+0]   - ptr_t[ll+0];
				ptr_dt[ll+ng0] +=   ptr_db[ll+ng0] - ptr_t[ll+ng0];
				ptr_dlam[ll+0]   -= ptr_lamt[ll+0]   * ptr_dt[ll+0]   + ptr_lam[ll+0];
				ptr_dlam[ll+ng0] -= ptr_lamt[ll+ng0] * ptr_dt[ll+ng0] + ptr_lam[ll+ng0];
				if( -alpha*ptr_dlam[ll+0]>ptr_lam[ll+0] )
					{
					alpha = - ptr_lam[ll+0] / ptr_dlam[ll+0];
					}
				if( -alpha*ptr_dlam[ll+ng0]>ptr_lam[ll+ng0] )
					{
					alpha = - ptr_lam[ll+ng0] / ptr_dlam[ll+ng0];
					}
				if( -alpha*ptr_dt[ll+0]>ptr_t[ll+0] )
					{
					alpha = - ptr_t[ll+0] / ptr_dt[ll+0];
					}
				if( -alpha*ptr_dt[ll+ng0]>ptr_t[ll+ng0] )
					{
					alpha = - ptr_t[ll+ng0] / ptr_dt[ll+ng0];
					}

				}

			}

		}		

	// store alpha
	ptr_alpha[0] = alpha;

	return;
	
	}



void d_update_var_mpc_hard_libstr(int N, int *nx, int *nu, int *nb, int *ng, double *ptr_mu, double mu_scal, double alpha, struct d_strvec *hsux, struct d_strvec *hsdux, struct d_strvec *hst, struct d_strvec *hsdt, struct d_strvec *hslam, struct d_strvec *hsdlam, struct d_strvec *hspi, struct d_strvec *hsdpi)
	{

	int nu0, nx0, nb0, ng0;

	int jj, ll;
	
	double
		*ptr_pi, *ptr_dpi, *ptr_ux, *ptr_dux, *ptr_t, *ptr_dt, *ptr_lam, *ptr_dlam;

	double mu = 0;

	for(jj=1; jj<=N; jj++)
		{

		nx0 = nx[jj];

		ptr_pi   = hspi[jj].pa;
		ptr_dpi  = hsdpi[jj].pa;

		// update equality constrained multipliers
		for(ll=0; ll<nx0-3; ll+=4)
			{
			ptr_pi[ll+0] += alpha*(ptr_dpi[ll+0] - ptr_pi[ll+0]);
			ptr_pi[ll+1] += alpha*(ptr_dpi[ll+1] - ptr_pi[ll+1]);
			ptr_pi[ll+2] += alpha*(ptr_dpi[ll+2] - ptr_pi[ll+2]);
			ptr_pi[ll+3] += alpha*(ptr_dpi[ll+3] - ptr_pi[ll+3]);
			}
		for(; ll<nx0; ll++)
			ptr_pi[ll] += alpha*(ptr_dpi[ll] - ptr_pi[ll]);

		}

	for(jj=0; jj<=N; jj++)
		{

		nx0 = nx[jj];
		nu0 = nu[jj];
		nb0 = nb[jj];
		ng0 = ng[jj];
		
		ptr_ux   = hsux[jj].pa;
		ptr_dux  = hsdux[jj].pa;
		ptr_t    = hst[jj].pa;
		ptr_dt   = hsdt[jj].pa;
		ptr_lam  = hslam[jj].pa;
		ptr_dlam = hsdlam[jj].pa;

		// update inputs and states
		for(ll=0; ll<nu0+nx0-3; ll+=4)
			{
			ptr_ux[ll+0] += alpha*(ptr_dux[ll+0] - ptr_ux[ll+0]);
			ptr_ux[ll+1] += alpha*(ptr_dux[ll+1] - ptr_ux[ll+1]);
			ptr_ux[ll+2] += alpha*(ptr_dux[ll+2] - ptr_ux[ll+2]);
			ptr_ux[ll+3] += alpha*(ptr_dux[ll+3] - ptr_ux[ll+3]);
			}
		for(; ll<nu0+nx0; ll++)
			ptr_ux[ll] += alpha*(ptr_dux[ll] - ptr_ux[ll]);
		// box constraints
		for(ll=0; ll<nb0; ll++)
			{
			ptr_lam[ll+0] += alpha*ptr_dlam[ll+0];
			ptr_lam[ll+nb0] += alpha*ptr_dlam[ll+nb0];
			ptr_t[ll+0] += alpha*ptr_dt[ll+0];
			ptr_t[ll+nb0] += alpha*ptr_dt[ll+nb0];
			mu += ptr_lam[ll+0] * ptr_t[ll+0] + ptr_lam[ll+nb0] * ptr_t[ll+nb0];
			}

		ptr_t    += 2*nb0;
		ptr_dt   += 2*nb0;
		ptr_lam  += 2*nb0;
		ptr_dlam += 2*nb0;

		// genreal constraints
		for(ll=0; ll<ng0; ll++)
			{
			ptr_lam[ll+0] += alpha*ptr_dlam[ll+0];
			ptr_lam[ll+ng0] += alpha*ptr_dlam[ll+ng0];
			ptr_t[ll+0] += alpha*ptr_dt[ll+0];
			ptr_t[ll+ng0] += alpha*ptr_dt[ll+ng0];
			mu += ptr_lam[ll+0] * ptr_t[ll+0] + ptr_lam[ll+ng0] * ptr_t[ll+ng0];
			}

		}

	// scale mu
	mu *= mu_scal;

	ptr_mu[0] = mu;

	return;
	
	}



void d_compute_mu_mpc_hard_libstr(int N, int *nx, int *nu, int *nb, int *ng, double *ptr_mu, double mu_scal, double alpha, struct d_strvec *hslam, struct d_strvec *hsdlam, struct d_strvec *hst, struct d_strvec *hsdt)
	{
	
	int nb0, ng0;

	int jj, ll;
	
	double
		*ptr_t, *ptr_lam, *ptr_dt, *ptr_dlam;
		
	double mu = 0;
	
	for(jj=0; jj<=N; jj++)
		{
		
		nb0 = nb[jj];
		ng0 = ng[jj];
		
		ptr_t    = hst[jj].pa;
		ptr_lam  = hslam[jj].pa;
		ptr_dt   = hsdt[jj].pa;
		ptr_dlam = hsdlam[jj].pa;

		// box constraints
		for(ll=0 ; ll<nb0; ll++)
			{
			mu += (ptr_lam[ll+0] + alpha*ptr_dlam[ll+0]) * (ptr_t[ll+0] + alpha*ptr_dt[ll+0]) + (ptr_lam[ll+nb0] + alpha*ptr_dlam[ll+nb0]) * (ptr_t[ll+nb0] + alpha*ptr_dt[ll+nb0]);
			}

		ptr_t    += 2*nb0;
		ptr_dt   += 2*nb0;
		ptr_lam  += 2*nb0;
		ptr_dlam += 2*nb0;

		// general constraints
		for(ll=0; ll<ng0; ll++)
			{
			mu += (ptr_lam[ll+0] + alpha*ptr_dlam[ll+0]) * (ptr_t[ll+0] + alpha*ptr_dt[ll+0]) + (ptr_lam[ll+ng0] + alpha*ptr_dlam[ll+ng0]) * (ptr_t[ll+ng0] + alpha*ptr_dt[ll+ng0]);
			}

		}

	// scale mu
	mu *= mu_scal;
		
	ptr_mu[0] = mu;

	return;

	}



void d_update_gradient_new_rhs_mpc_hard_tv(int N, int *nx, int *nu, int *nb, int *ng, double **db, double **lamt, double **qx)
	{
	
	// constants
	const int bs = D_MR;
	const int ncl = D_NCL;

	int nb0, pnb, ng0, png;
	
	double temp0, temp1;
	
	double 
		*ptr_db, *ptr_Qx, *ptr_qx,
		*ptr_lamt;
	
	int ii, jj, bs0;
	
	for(jj=0; jj<=N; jj++)
		{
		
		ptr_lamt  = lamt[jj];
		ptr_db    = db[jj];
		ptr_qx    = qx[jj];

		// box constraints
		nb0 = nb[jj];
		if(nb0>0)
			{

			pnb  = (nb0+bs-1)/bs*bs; // simd aligned number of box constraints

			for(ii=0; ii<nb0-3; ii+=4)
				{

				ptr_qx[ii+0] = - ptr_lamt[ii+pnb+0]*ptr_db[ii+pnb+0] - ptr_lamt[ii+0]*ptr_db[ii+0];

				ptr_qx[ii+1] = - ptr_lamt[ii+pnb+1]*ptr_db[ii+pnb+1] - ptr_lamt[ii+1]*ptr_db[ii+1];

				ptr_qx[ii+2] = - ptr_lamt[ii+pnb+2]*ptr_db[ii+pnb+2] - ptr_lamt[ii+2]*ptr_db[ii+2];

				ptr_qx[ii+3] = - ptr_lamt[ii+pnb+3]*ptr_db[ii+pnb+3] - ptr_lamt[ii+3]*ptr_db[ii+3];

				}
			for(; ii<nb0; ii++)
				{

				ptr_qx[ii+0] = - ptr_lamt[ii+pnb+0]*ptr_db[ii+pnb+0] - ptr_lamt[ii+0]*ptr_db[ii+0];

				}

			ptr_lamt  += 2*pnb;
			ptr_db    += 2*pnb;
			ptr_qx    += pnb;

			} // end of if nb0>0

		// general constraints
		ng0 = ng[jj];
		if(ng0>0)
			{

			png = (ng0+bs-1)/bs*bs; // simd aligned number of general constraints

			for(ii=0; ii<ng0-3; ii+=4)
				{

				ptr_qx[ii+0] = - ptr_lamt[ii+png+0]*ptr_db[ii+png+0] - ptr_lamt[ii+0]*ptr_db[ii+0];

				ptr_qx[ii+1] = - ptr_lamt[ii+png+1]*ptr_db[ii+png+1] - ptr_lamt[ii+1]*ptr_db[ii+1];

				ptr_qx[ii+2] = - ptr_lamt[ii+png+2]*ptr_db[ii+png+2] - ptr_lamt[ii+2]*ptr_db[ii+2];

				ptr_qx[ii+3] = - ptr_lamt[ii+png+3]*ptr_db[ii+png+3] - ptr_lamt[ii+3]*ptr_db[ii+3];

				}
			for(; ii<ng0; ii++)
				{

				ptr_qx[ii+0] = - ptr_lamt[ii+png+0]*ptr_db[ii+png+0] - ptr_lamt[ii+0]*ptr_db[ii+0];

				}

			} // end of if ng0>0

		} // end of jj loop over N

	}



void d_compute_t_lam_new_rhs_mpc_hard_tv(int N, int *nx, int *nu, int *nb, int **idxb, int *ng, double **t_aff, double **lam_aff, double **lamt, double **tinv, double **dux, double **pDCt, double **db)
	{
	
	// constants
	const int bs = D_MR;
	const int ncl = D_NCL;

	int nu0, nx0, nb0, pnb, ng0, png, cng;

	double
		*ptr_db, *ptr_dux, *ptr_t_aff, *ptr_lam_aff, *ptr_lamt, *ptr_tinv;
	
	int
		*ptr_idxb;
	
	int jj, ll;

	for(jj=0; jj<=N; jj++)
		{

		ptr_db      = db[jj];
		ptr_dux     = dux[jj];
		ptr_t_aff   = t_aff[jj];
		ptr_lam_aff = lam_aff[jj];
		ptr_lamt    = lamt[jj];
		ptr_tinv    = tinv[jj];
		ptr_idxb    = idxb[jj];

		// box constraints
		nb0 = nb[jj];
		if(nb0>0)
			{

			pnb = (nb0+bs-1)/bs*bs;

			// box constraints
			for(ll=0; ll<nb0; ll++)
				{

				ptr_t_aff[ll+0]   =   ptr_dux[ptr_idxb[ll]] - ptr_db[ll+0];
				ptr_t_aff[ll+pnb] = - ptr_dux[ptr_idxb[ll]] + ptr_db[ll+pnb];
				ptr_lam_aff[ll+0]   = - ptr_lamt[ll+0]   * ptr_t_aff[ll+0];
				ptr_lam_aff[ll+pnb] = - ptr_lamt[ll+pnb] * ptr_t_aff[ll+pnb];
				}

			ptr_db      += 2*pnb;
			ptr_t_aff   += 2*pnb;
			ptr_lam_aff += 2*pnb;
			ptr_lamt    += 2*pnb;
			ptr_tinv    += 2*pnb;

			}

		// general constraints
		ng0 = ng[jj];
		if(ng0>0)
			{

			nu0 = nu[jj];
			nx0 = nx[jj];
			png = (ng0+bs-1)/bs*bs;
			cng = (ng0+ncl-1)/ncl*ncl;

#ifdef BLASFEO
			dgemv_t_lib(nx0+nu0, ng0, 1.0, pDCt[jj], cng, ptr_dux, 0.0, ptr_t_aff, ptr_t_aff);
#else
			dgemv_t_lib(nx0+nu0, ng0, pDCt[jj], cng, ptr_dux, 0, ptr_t_aff, ptr_t_aff);
#endif

			for(ll=0; ll<ng0; ll++)
				{
				ptr_t_aff[ll+png] = - ptr_t_aff[ll+0];
				ptr_t_aff[ll+0]   -= ptr_db[ll+0];
				ptr_t_aff[ll+png] += ptr_db[ll+png];
				ptr_lam_aff[ll+0]   = - ptr_lamt[ll+0]   * ptr_t_aff[ll+0];
				ptr_lam_aff[ll+png] = - ptr_lamt[ll+png] * ptr_t_aff[ll+png];
				}

			}

		}		

	return;
	
	}



// IPM with residuals

void d_update_hessian_gradient_res_mpc_hard_tv(int N, int *nx, int *nu, int *nb, int *ng, double **res_d, double **res_m, double **t, double **lam, double **t_inv, double **Qx, double **qx)
	{
	
	// constants
	const int bs = D_MR;
	const int ncl = D_NCL;

	int nb0, pnb, ng0, png;
	
	double temp0, temp1;
	
	double 
		*ptr_res_d, *ptr_Qx, *ptr_qx, *ptr_t, *ptr_lam, *ptr_res_m, *ptr_t_inv;
	
	int ii, jj, bs0;
	
	for(jj=0; jj<=N; jj++)
		{
		
		ptr_t     = t[jj];
		ptr_lam   = lam[jj];
		ptr_t_inv = t_inv[jj];
		ptr_res_d = res_d[jj];
		ptr_res_m = res_m[jj];
		ptr_Qx    = Qx[jj];
		ptr_qx    = qx[jj];

		// box constraints
		nb0 = nb[jj];
		if(nb0>0)
			{

			pnb  = (nb0+bs-1)/bs*bs; // simd aligned number of box constraints

			for(ii=0; ii<nb0-3; ii+=4)
				{

				ptr_t_inv[ii+0] = 1.0/ptr_t[ii+0];
				ptr_t_inv[ii+pnb+0] = 1.0/ptr_t[ii+pnb+0];
				ptr_Qx[ii+0] = ptr_t_inv[ii+0]*ptr_lam[ii+0] + ptr_t_inv[ii+pnb+0]*ptr_lam[ii+pnb+0];
				ptr_qx[ii+0] = ptr_t_inv[ii+0]*(ptr_res_m[ii+0]-ptr_lam[ii+0]*ptr_res_d[ii+0]) - ptr_t_inv[ii+pnb+0]*(ptr_res_m[ii+pnb+0]+ptr_lam[ii+pnb+0]*ptr_res_d[ii+pnb+0]);

				ptr_t_inv[ii+1] = 1.0/ptr_t[ii+1];
				ptr_t_inv[ii+pnb+1] = 1.0/ptr_t[ii+pnb+1];
				ptr_Qx[ii+1] = ptr_t_inv[ii+1]*ptr_lam[ii+1] + ptr_t_inv[ii+pnb+1]*ptr_lam[ii+pnb+1];
				ptr_qx[ii+1] = ptr_t_inv[ii+1]*(ptr_res_m[ii+1]-ptr_lam[ii+1]*ptr_res_d[ii+1]) - ptr_t_inv[ii+pnb+1]*(ptr_res_m[ii+pnb+1]+ptr_lam[ii+pnb+1]*ptr_res_d[ii+pnb+1]);

				ptr_t_inv[ii+2] = 1.0/ptr_t[ii+2];
				ptr_t_inv[ii+pnb+2] = 1.0/ptr_t[ii+pnb+2];
				ptr_Qx[ii+2] = ptr_t_inv[ii+2]*ptr_lam[ii+2] + ptr_t_inv[ii+pnb+2]*ptr_lam[ii+pnb+2];
				ptr_qx[ii+2] = ptr_t_inv[ii+2]*(ptr_res_m[ii+2]-ptr_lam[ii+2]*ptr_res_d[ii+2]) - ptr_t_inv[ii+pnb+2]*(ptr_res_m[ii+pnb+2]+ptr_lam[ii+pnb+2]*ptr_res_d[ii+pnb+2]);

				ptr_t_inv[ii+3] = 1.0/ptr_t[ii+3];
				ptr_t_inv[ii+pnb+3] = 1.0/ptr_t[ii+pnb+3];
				ptr_Qx[ii+3] = ptr_t_inv[ii+3]*ptr_lam[ii+3] + ptr_t_inv[ii+pnb+3]*ptr_lam[ii+pnb+3];
				ptr_qx[ii+3] = ptr_t_inv[ii+3]*(ptr_res_m[ii+3]-ptr_lam[ii+3]*ptr_res_d[ii+3]) - ptr_t_inv[ii+pnb+3]*(ptr_res_m[ii+pnb+3]+ptr_lam[ii+pnb+3]*ptr_res_d[ii+pnb+3]);

				}
			for(; ii<nb0; ii++)
				{

				ptr_t_inv[ii+0] = 1.0/ptr_t[ii+0];
				ptr_t_inv[ii+pnb+0] = 1.0/ptr_t[ii+pnb+0];
				ptr_Qx[ii+0] = ptr_t_inv[ii+0]*ptr_lam[ii+0] + ptr_t_inv[ii+pnb+0]*ptr_lam[ii+pnb+0];
				ptr_qx[ii+0] = ptr_t_inv[ii+0]*(ptr_res_m[ii+0]-ptr_lam[ii+0]*ptr_res_d[ii+0]) - ptr_t_inv[ii+pnb+0]*(ptr_res_m[ii+pnb+0]+ptr_lam[ii+pnb+0]*ptr_res_d[ii+pnb+0]);

				}

			ptr_t     += 2*pnb;
			ptr_lam   += 2*pnb;
			ptr_t_inv += 2*pnb;
			ptr_res_d += 2*pnb;
			ptr_res_m += 2*pnb;
			ptr_Qx    += pnb;
			ptr_qx    += pnb;

			}

		// general constraints
		ng0 = ng[jj];
		if(ng0>0)
			{

		
			png = (ng0+bs-1)/bs*bs; // simd aligned number of general constraints

			for(ii=0; ii<ng0-3; ii+=4)
				{

				ptr_t_inv[ii+0] = 1.0/ptr_t[ii+0];
				ptr_t_inv[ii+png+0] = 1.0/ptr_t[ii+png+0];
				ptr_Qx[ii+0] = ptr_t_inv[ii+0]*ptr_lam[ii+0] + ptr_t_inv[ii+png+0]*ptr_lam[ii+png+0];
				ptr_qx[ii+0] = ptr_t_inv[ii+0]*(ptr_res_m[ii+0]-ptr_lam[ii+0]*ptr_res_d[ii+0]) - ptr_t_inv[ii+png+0]*(ptr_res_m[ii+png+0]+ptr_lam[ii+png+0]*ptr_res_d[ii+png+0]);

				ptr_t_inv[ii+1] = 1.0/ptr_t[ii+1];
				ptr_t_inv[ii+png+1] = 1.0/ptr_t[ii+png+1];
				ptr_Qx[ii+1] = ptr_t_inv[ii+1]*ptr_lam[ii+1] + ptr_t_inv[ii+png+1]*ptr_lam[ii+png+1];
				ptr_qx[ii+1] = ptr_t_inv[ii+1]*(ptr_res_m[ii+1]-ptr_lam[ii+1]*ptr_res_d[ii+1]) - ptr_t_inv[ii+png+1]*(ptr_res_m[ii+png+1]+ptr_lam[ii+png+1]*ptr_res_d[ii+png+1]);

				ptr_t_inv[ii+2] = 1.0/ptr_t[ii+2];
				ptr_t_inv[ii+png+2] = 1.0/ptr_t[ii+png+2];
				ptr_Qx[ii+2] = ptr_t_inv[ii+2]*ptr_lam[ii+2] + ptr_t_inv[ii+png+2]*ptr_lam[ii+png+2];
				ptr_qx[ii+2] = ptr_t_inv[ii+2]*(ptr_res_m[ii+2]-ptr_lam[ii+2]*ptr_res_d[ii+2]) - ptr_t_inv[ii+png+2]*(ptr_res_m[ii+png+2]+ptr_lam[ii+png+2]*ptr_res_d[ii+png+2]);

				ptr_t_inv[ii+3] = 1.0/ptr_t[ii+3];
				ptr_t_inv[ii+png+3] = 1.0/ptr_t[ii+png+3];
				ptr_Qx[ii+3] = ptr_t_inv[ii+3]*ptr_lam[ii+3] + ptr_t_inv[ii+png+3]*ptr_lam[ii+png+3];
				ptr_qx[ii+3] = ptr_t_inv[ii+3]*(ptr_res_m[ii+3]-ptr_lam[ii+3]*ptr_res_d[ii+3]) - ptr_t_inv[ii+png+3]*(ptr_res_m[ii+png+3]+ptr_lam[ii+png+3]*ptr_res_d[ii+png+3]);

				}
			for(; ii<ng0; ii++)
				{
				
				ptr_t_inv[ii+0] = 1.0/ptr_t[ii+0];
				ptr_t_inv[ii+png+0] = 1.0/ptr_t[ii+png+0];
				ptr_Qx[ii+0] = ptr_t_inv[ii+0]*ptr_lam[ii+0] + ptr_t_inv[ii+png+0]*ptr_lam[ii+png+0];
				ptr_qx[ii+0] = ptr_t_inv[ii+0]*(ptr_res_m[ii+0]-ptr_lam[ii+0]*ptr_res_d[ii+0]) - ptr_t_inv[ii+png+0]*(ptr_res_m[ii+png+0]+ptr_lam[ii+png+0]*ptr_res_d[ii+png+0]);

				}

			}

		}

	}



void d_compute_dt_dlam_res_mpc_hard_tv(int N, int *nx, int *nu, int *nb, int **idxb, int *ng, double **dux, double **t, double **t_inv, double **lam, double **pDCt, double **res_d, double **res_m, double **dt, double **dlam)
	{
	
	// constants
	const int bs = D_MR;
	const int ncl = D_NCL;

	int nu0, nx0, nb0, pnb, ng0, png, cng;

	double
		*ptr_res_d, *ptr_res_m, *ptr_dux, *ptr_t, *ptr_t_inv, *ptr_dt, *ptr_lam, *ptr_dlam;
	
	int
		*ptr_idxb;
	
	int jj, ll;

	for(jj=0; jj<=N; jj++)
		{

		ptr_res_d = res_d[jj];
		ptr_res_m = res_m[jj];
		ptr_dux   = dux[jj];
		ptr_t     = t[jj];
		ptr_t_inv = t_inv[jj];
		ptr_dt    = dt[jj];
		ptr_lam   = lam[jj];
		ptr_dlam  = dlam[jj];
		ptr_idxb  = idxb[jj];

		// box constraints
		nb0 = nb[jj];
		if(nb0>0)
			{

			pnb = (nb0+bs-1)/bs*bs;

			// box constraints
			for(ll=0; ll<nb0; ll++)
				{
				
				ptr_dt[ll+0]   =   ptr_dux[ptr_idxb[ll]] - ptr_res_d[ll+0];
				ptr_dt[ll+pnb] = - ptr_dux[ptr_idxb[ll]] + ptr_res_d[ll+pnb];

				ptr_dlam[ll+0]   = - ptr_t_inv[ll+0]   * ( ptr_lam[ll+0]*ptr_dt[ll+0]     + ptr_res_m[ll+0] );
				ptr_dlam[ll+pnb] = - ptr_t_inv[ll+pnb] * ( ptr_lam[ll+pnb]*ptr_dt[ll+pnb] + ptr_res_m[ll+pnb] );

				}

			ptr_res_d += 2*pnb;
			ptr_res_m += 2*pnb;
			ptr_t     += 2*pnb;
			ptr_t_inv += 2*pnb;
			ptr_dt    += 2*pnb;
			ptr_lam   += 2*pnb;
			ptr_dlam  += 2*pnb;

			}

		// general constraints
		ng0 = ng[jj];
		if(ng0>0)
			{

			nu0 = nu[jj];
			nx0 = nx[jj];
			png = (ng0+bs-1)/bs*bs;
			cng = (ng0+ncl-1)/ncl*ncl;

#ifdef BLASFEO
			dgemv_t_lib(nx0+nu0, ng0, 1.0, pDCt[jj], cng, ptr_dux, 0.0, ptr_dt, ptr_dt);
#else
			dgemv_t_lib(nx0+nu0, ng0, pDCt[jj], cng, ptr_dux, 0, ptr_dt, ptr_dt);
#endif

			for(ll=0; ll<ng0; ll++)
				{

				ptr_dt[ll+png] = - ptr_dt[ll];

				ptr_dt[ll+0]   -= ptr_res_d[ll+0];
				ptr_dt[ll+png] += ptr_res_d[ll+png];

				ptr_dlam[ll+0]   = - ptr_t_inv[ll+0]   * ( ptr_lam[ll+0]*ptr_dt[ll+0]     + ptr_res_m[ll+0] );
				ptr_dlam[ll+png] = - ptr_t_inv[ll+png] * ( ptr_lam[ll+png]*ptr_dt[ll+png] + ptr_res_m[ll+png] );

				}

			}

		}		

	return;
	
	}



void d_compute_alpha_res_mpc_hard_tv(int N, int *nx, int *nu, int *nb, int **idxb, int *ng, double **dux, double **t, double **t_inv, double **lam, double **pDCt, double **res_d, double **res_m, double **dt, double **dlam, double *ptr_alpha)
	{
	
	// constants
	const int bs = D_MR;
	const int ncl = D_NCL;

	int nu0, nx0, nb0, pnb, ng0, png, cng;

	double alpha = ptr_alpha[0];
	
	double
		*ptr_res_d, *ptr_res_m, *ptr_dux, *ptr_t, *ptr_t_inv, *ptr_dt, *ptr_lam, *ptr_dlam;
	
	int
		*ptr_idxb;
	
	int jj, ll;

	for(jj=0; jj<=N; jj++)
		{

		ptr_res_d = res_d[jj];
		ptr_res_m = res_m[jj];
		ptr_dux   = dux[jj];
		ptr_t     = t[jj];
		ptr_t_inv = t_inv[jj];
		ptr_dt    = dt[jj];
		ptr_lam   = lam[jj];
		ptr_dlam  = dlam[jj];
		ptr_idxb  = idxb[jj];

		// box constraints
		nb0 = nb[jj];
		if(nb0>0)
			{

			pnb = (nb0+bs-1)/bs*bs;

			// box constraints
			for(ll=0; ll<nb0; ll++)
				{
				
				ptr_dt[ll+0]   =   ptr_dux[ptr_idxb[ll]] - ptr_res_d[ll+0];
				ptr_dt[ll+pnb] = - ptr_dux[ptr_idxb[ll]] + ptr_res_d[ll+pnb];

				ptr_dlam[ll+0]   = - ptr_t_inv[ll+0]   * ( ptr_lam[ll+0]*ptr_dt[ll+0]     + ptr_res_m[ll+0] );
				ptr_dlam[ll+pnb] = - ptr_t_inv[ll+pnb] * ( ptr_lam[ll+pnb]*ptr_dt[ll+pnb] + ptr_res_m[ll+pnb] );

				if( -alpha*ptr_dlam[ll+0]>ptr_lam[ll+0] )
					{
					alpha = - ptr_lam[ll+0] / ptr_dlam[ll+0];
					}
				if( -alpha*ptr_dlam[ll+pnb]>ptr_lam[ll+pnb] )
					{
					alpha = - ptr_lam[ll+pnb] / ptr_dlam[ll+pnb];
					}
				if( -alpha*ptr_dt[ll+0]>ptr_t[ll+0] )
					{
					alpha = - ptr_t[ll+0] / ptr_dt[ll+0];
					}
				if( -alpha*ptr_dt[ll+pnb]>ptr_t[ll+pnb] )
					{
					alpha = - ptr_t[ll+pnb] / ptr_dt[ll+pnb];
					}

				}

			ptr_res_d += 2*pnb;
			ptr_res_m += 2*pnb;
			ptr_t     += 2*pnb;
			ptr_t_inv += 2*pnb;
			ptr_dt    += 2*pnb;
			ptr_lam   += 2*pnb;
			ptr_dlam  += 2*pnb;

			}

		// general constraints
		ng0 = ng[jj];
		if(ng0>0)
			{

			nu0 = nu[jj];
			nx0 = nx[jj];
			png = (ng0+bs-1)/bs*bs;
			cng = (ng0+ncl-1)/ncl*ncl;

#ifdef BLASFEO
			dgemv_t_lib(nx0+nu0, ng0, 1.0, pDCt[jj], cng, ptr_dux, 0.0, ptr_dt, ptr_dt);
#else
			dgemv_t_lib(nx0+nu0, ng0, pDCt[jj], cng, ptr_dux, 0, ptr_dt, ptr_dt);
#endif

			for(ll=0; ll<ng0; ll++)
				{

				ptr_dt[ll+png] = - ptr_dt[ll];

				ptr_dt[ll+0]   -= ptr_res_d[ll+0];
				ptr_dt[ll+png] += ptr_res_d[ll+png];

				ptr_dlam[ll+0]   = - ptr_t_inv[ll+0]   * ( ptr_lam[ll+0]*ptr_dt[ll+0]     + ptr_res_m[ll+0] );
				ptr_dlam[ll+png] = - ptr_t_inv[ll+png] * ( ptr_lam[ll+png]*ptr_dt[ll+png] + ptr_res_m[ll+png] );

				if( -alpha*ptr_dlam[ll+0]>ptr_lam[ll+0] )
					{
					alpha = - ptr_lam[ll+0] / ptr_dlam[ll+0];
					}
				if( -alpha*ptr_dlam[ll+png]>ptr_lam[ll+png] )
					{
					alpha = - ptr_lam[ll+png] / ptr_dlam[ll+png];
					}
				if( -alpha*ptr_dt[ll+0]>ptr_t[ll+0] )
					{
					alpha = - ptr_t[ll+0] / ptr_dt[ll+0];
					}
				if( -alpha*ptr_dt[ll+png]>ptr_t[ll+png] )
					{
					alpha = - ptr_t[ll+png] / ptr_dt[ll+png];
					}

				}

			}

		}		

	// store alpha
	ptr_alpha[0] = alpha;

	return;
	
	}



void d_update_var_res_mpc_hard_tv(int N, int *nx, int *nu, int *nb, int *ng, double alpha, double **ux, double **dux, double **pi, double **dpi, double **t, double **dt, double **lam, double **dlam)
	{

	// constants
	const int bs = D_MR;

	int nu0, nx0, nx1, nb0, pnb, ng0, png;

	int jj, ll;
	
	double
		*ptr_ux, *ptr_dux, *ptr_pi, *ptr_dpi, *ptr_t, *ptr_dt, *ptr_lam, *ptr_dlam;

	for(jj=0; jj<=N; jj++)
		{

		nx0 = nx[jj];
		nu0 = nu[jj];
		nb0 = nb[jj];
		pnb = bs*((nb0+bs-1)/bs); // cache aligned number of box constraints
		ng0 = ng[jj];
		png = bs*((ng0+bs-1)/bs); // cache aligned number of box constraints
		if(jj<N)
			nx1 = nx[jj+1];
		else
			nx1 = 0;
		
		// update inputs and states
		ptr_ux     = ux[jj];
		ptr_dux    = dux[jj];
		daxpy_lib(nu0+nx0, alpha, ptr_dux, ptr_ux);

		// update equality constrained multipliers
		ptr_pi     = pi[jj];
		ptr_dpi    = dpi[jj];
		daxpy_lib(nx1, alpha, ptr_dpi, ptr_pi);

		// box constraints
		ptr_t       = t[jj];
		ptr_dt      = dt[jj];
		ptr_lam     = lam[jj];
		ptr_dlam    = dlam[jj];
		daxpy_lib(nb0, alpha, &ptr_dlam[0], &ptr_lam[0]);
		daxpy_lib(nb0, alpha, &ptr_dlam[pnb], &ptr_lam[pnb]);
		daxpy_lib(nb0, alpha, &ptr_dt[0], &ptr_t[0]);
		daxpy_lib(nb0, alpha, &ptr_dt[pnb], &ptr_t[pnb]);

		// general constraints
		ptr_t       += 2*pnb;
		ptr_dt      += 2*pnb;
		ptr_lam     += 2*pnb;
		ptr_dlam    += 2*pnb;
		daxpy_lib(ng0, alpha, &ptr_dlam[0], &ptr_lam[0]);
		daxpy_lib(ng0, alpha, &ptr_dlam[png], &ptr_lam[png]);
		daxpy_lib(ng0, alpha, &ptr_dt[0], &ptr_t[0]);
		daxpy_lib(ng0, alpha, &ptr_dt[png], &ptr_t[png]);

		}

	return;
	
	}



void d_backup_update_var_res_mpc_hard_tv(int N, int *nx, int *nu, int *nb, int *ng, double alpha, double **ux_bkp, double **ux, double **dux, double **pi_bkp, double **pi, double **dpi, double **t_bkp, double **t, double **dt, double **lam_bkp, double **lam, double **dlam)
	{

	// constants
	const int bs = D_MR;

	int nu0, nx0, nx1, nb0, pnb, ng0, png;

	int jj, ll;
	
	double
		*ptr_ux_bkp, *ptr_ux, *ptr_dux, *ptr_pi_bkp, *ptr_pi, *ptr_dpi, *ptr_t_bkp, *ptr_t, *ptr_dt, *ptr_lam_bkp, *ptr_lam, *ptr_dlam;

	for(jj=0; jj<=N; jj++)
		{

		nx0 = nx[jj];
		nu0 = nu[jj];
		nb0 = nb[jj];
		pnb = bs*((nb0+bs-1)/bs); // cache aligned number of box constraints
		ng0 = ng[jj];
		png = bs*((ng0+bs-1)/bs); // cache aligned number of box constraints
		if(jj<N)
			nx1 = nx[jj+1];
		else
			nx1 = 0;
		
		// update inputs and states
		ptr_ux_bkp = ux_bkp[jj];
		ptr_ux     = ux[jj];
		ptr_dux    = dux[jj];
		daxpy_bkp_lib(nu0+nx0, alpha, ptr_dux, ptr_ux, ptr_ux_bkp);

		// update equality constrained multipliers
		ptr_pi_bkp = pi_bkp[jj];
		ptr_pi     = pi[jj];
		ptr_dpi    = dpi[jj];
		daxpy_bkp_lib(nx1, alpha, ptr_dpi, ptr_pi, ptr_pi_bkp);

		// box constraints
		ptr_t_bkp   = t_bkp[jj];
		ptr_t       = t[jj];
		ptr_dt      = dt[jj];
		ptr_lam_bkp = lam_bkp[jj];
		ptr_lam     = lam[jj];
		ptr_dlam    = dlam[jj];
		daxpy_bkp_lib(nb0, alpha, &ptr_dlam[0], &ptr_lam[0], &ptr_lam_bkp[0]);
		daxpy_bkp_lib(nb0, alpha, &ptr_dlam[pnb], &ptr_lam[pnb], &ptr_lam_bkp[pnb]);
		daxpy_bkp_lib(nb0, alpha, &ptr_dt[0], &ptr_t[0], &ptr_t_bkp[0]);
		daxpy_bkp_lib(nb0, alpha, &ptr_dt[pnb], &ptr_t[pnb], &ptr_t_bkp[pnb]);

		// general constraints
		ptr_t_bkp   += 2*pnb;
		ptr_t       += 2*pnb;
		ptr_dt      += 2*pnb;
		ptr_lam_bkp += 2*pnb;
		ptr_lam     += 2*pnb;
		ptr_dlam    += 2*pnb;
		daxpy_bkp_lib(ng0, alpha, &ptr_dlam[0], &ptr_lam[0], &ptr_lam_bkp[0]);
		daxpy_bkp_lib(ng0, alpha, &ptr_dlam[png], &ptr_lam[png], &ptr_lam_bkp[png]);
		daxpy_bkp_lib(ng0, alpha, &ptr_dt[0], &ptr_t[0], &ptr_t_bkp[0]);
		daxpy_bkp_lib(ng0, alpha, &ptr_dt[png], &ptr_t[png], &ptr_t_bkp[png]);

		}

	return;
	
	}



void d_compute_mu_res_mpc_hard_tv(int N, int *nx, int *nu, int *nb, int *ng, double alpha, double **lam, double **dlam, double **t, double **dt, double *ptr_mu, double mu_scal)
	{
	
	// constants
	const int bs = D_MR;
	const int ncl = D_NCL;

	int nb0, pnb, ng0, png;

	int jj, ll;
	
	double
		*ptr_t, *ptr_lam, *ptr_dt, *ptr_dlam;
		
	double mu = 0;
	
	for(jj=0; jj<=N; jj++)
		{
		
		nb0 = nb[jj];
		pnb = (nb0+bs-1)/bs*bs;
		ng0 = ng[jj];
		png = (ng0+bs-1)/bs*bs;
		
		ptr_t    = t[jj];
		ptr_lam  = lam[jj];
		ptr_dt   = dt[jj];
		ptr_dlam = dlam[jj];

		// box constraints
		for(ll=0 ; ll<nb0; ll++)
			{
			mu += (ptr_lam[ll+0] + alpha*ptr_dlam[ll+0]) * (ptr_t[ll+0] + alpha*ptr_dt[ll+0]) + (ptr_lam[ll+pnb] + alpha*ptr_dlam[ll+pnb]) * (ptr_t[ll+pnb] + alpha*ptr_dt[ll+pnb]);
			}

		ptr_t    += 2*pnb;
		ptr_dt   += 2*pnb;
		ptr_lam  += 2*pnb;
		ptr_dlam += 2*pnb;

		// general constraints
		for(ll=0; ll<ng0; ll++)
			{
			mu += (ptr_lam[ll+0] + alpha*ptr_dlam[ll+0]) * (ptr_t[ll+0] + alpha*ptr_dt[ll+0]) + (ptr_lam[ll+png] + alpha*ptr_dlam[ll+png]) * (ptr_t[ll+png] + alpha*ptr_dt[ll+png]);
			}

		}

	// scale mu
	mu *= mu_scal;
		
	ptr_mu[0] = mu;

	return;

	}



void d_compute_centering_correction_res_mpc_hard_tv(int N, int *nb, int *ng, double sigma_mu, double **dt, double **dlam, double **res_m)
	{

	const int bs = D_MR;

	int pnb, png;

	int ii, jj;

	double
		*ptr_res_m, *ptr_dt, *ptr_dlam;
	
	for(ii=0; ii<=N; ii++)
		{

		pnb = (nb[ii]+bs-1)/bs*bs;
		png = (ng[ii]+bs-1)/bs*bs;

		ptr_res_m = res_m[ii];
		ptr_dt    = dt[ii];
		ptr_dlam  = dlam[ii];

		for(jj=0; jj<nb[ii]; jj++)
			{
			ptr_res_m[jj]     += ptr_dt[jj]     * ptr_dlam[jj]     - sigma_mu;
			ptr_res_m[pnb+jj] += ptr_dt[pnb+jj] * ptr_dlam[pnb+jj] - sigma_mu;
			}
		for(jj=0; jj<ng[ii]; jj++)
			{
			ptr_res_m[2*pnb+jj]     += ptr_dt[2*pnb+jj]     * ptr_dlam[2*pnb+jj]     - sigma_mu;
			ptr_res_m[2*pnb+png+jj] += ptr_dt[2*pnb+png+jj] * ptr_dlam[2*pnb+png+jj] - sigma_mu;
			}
		}

	}



void d_update_gradient_res_mpc_hard_tv(int N, int *nx, int *nu, int *nb, int *ng, double **res_d, double **res_m, double **lam, double **t_inv, double **qx)
	{
	
	// constants
	const int bs = D_MR;
	const int ncl = D_NCL;

	int nb0, pnb, ng0, png;
	
	double temp0, temp1;
	
	double 
		*ptr_res_d, *ptr_Qx, *ptr_qx, *ptr_lam, *ptr_res_m, *ptr_t_inv;
	
	int ii, jj, bs0;
	
	for(jj=0; jj<=N; jj++)
		{
		
		ptr_lam   = lam[jj];
		ptr_t_inv = t_inv[jj];
		ptr_res_d = res_d[jj];
		ptr_res_m = res_m[jj];
		ptr_qx    = qx[jj];

		// box constraints
		nb0 = nb[jj];
		if(nb0>0)
			{

			pnb  = (nb0+bs-1)/bs*bs; // simd aligned number of box constraints

			for(ii=0; ii<nb0-3; ii+=4)
				{

				ptr_qx[ii+0] = ptr_t_inv[ii+0]*(ptr_res_m[ii+0]-ptr_lam[ii+0]*ptr_res_d[ii+0]) - ptr_t_inv[ii+pnb+0]*(ptr_res_m[ii+pnb+0]+ptr_lam[ii+pnb+0]*ptr_res_d[ii+pnb+0]);

				ptr_qx[ii+1] = ptr_t_inv[ii+1]*(ptr_res_m[ii+1]-ptr_lam[ii+1]*ptr_res_d[ii+1]) - ptr_t_inv[ii+pnb+1]*(ptr_res_m[ii+pnb+1]+ptr_lam[ii+pnb+1]*ptr_res_d[ii+pnb+1]);

				ptr_qx[ii+2] = ptr_t_inv[ii+2]*(ptr_res_m[ii+2]-ptr_lam[ii+2]*ptr_res_d[ii+2]) - ptr_t_inv[ii+pnb+2]*(ptr_res_m[ii+pnb+2]+ptr_lam[ii+pnb+2]*ptr_res_d[ii+pnb+2]);

				ptr_qx[ii+3] = ptr_t_inv[ii+3]*(ptr_res_m[ii+3]-ptr_lam[ii+3]*ptr_res_d[ii+3]) - ptr_t_inv[ii+pnb+3]*(ptr_res_m[ii+pnb+3]+ptr_lam[ii+pnb+3]*ptr_res_d[ii+pnb+3]);

				}
			for(; ii<nb0; ii++)
				{

				ptr_qx[ii+0] = ptr_t_inv[ii+0]*(ptr_res_m[ii+0]-ptr_lam[ii+0]*ptr_res_d[ii+0]) - ptr_t_inv[ii+pnb+0]*(ptr_res_m[ii+pnb+0]+ptr_lam[ii+pnb+0]*ptr_res_d[ii+pnb+0]);

				}

			ptr_lam   += 2*pnb;
			ptr_t_inv += 2*pnb;
			ptr_res_d += 2*pnb;
			ptr_res_m += 2*pnb;
			ptr_qx    += pnb;

			}

		// general constraints
		ng0 = ng[jj];
		if(ng0>0)
			{

			png = (ng0+bs-1)/bs*bs; // simd aligned number of general constraints

			for(ii=0; ii<ng0-3; ii+=4)
				{

				ptr_qx[ii+0] = ptr_t_inv[ii+0]*(ptr_res_m[ii+0]-ptr_lam[ii+0]*ptr_res_d[ii+0]) - ptr_t_inv[ii+png+0]*(ptr_res_m[ii+png+0]+ptr_lam[ii+png+0]*ptr_res_d[ii+png+0]);

				ptr_qx[ii+1] = ptr_t_inv[ii+1]*(ptr_res_m[ii+1]-ptr_lam[ii+1]*ptr_res_d[ii+1]) - ptr_t_inv[ii+png+1]*(ptr_res_m[ii+png+1]+ptr_lam[ii+png+1]*ptr_res_d[ii+png+1]);

				ptr_qx[ii+2] = ptr_t_inv[ii+2]*(ptr_res_m[ii+2]-ptr_lam[ii+2]*ptr_res_d[ii+2]) - ptr_t_inv[ii+png+2]*(ptr_res_m[ii+png+2]+ptr_lam[ii+png+2]*ptr_res_d[ii+png+2]);

				ptr_qx[ii+3] = ptr_t_inv[ii+3]*(ptr_res_m[ii+3]-ptr_lam[ii+3]*ptr_res_d[ii+3]) - ptr_t_inv[ii+png+3]*(ptr_res_m[ii+png+3]+ptr_lam[ii+png+3]*ptr_res_d[ii+png+3]);

				}
			for(; ii<ng0; ii++)
				{
				
				ptr_qx[ii+0] = ptr_t_inv[ii+0]*(ptr_res_m[ii+0]-ptr_lam[ii+0]*ptr_res_d[ii+0]) - ptr_t_inv[ii+png+0]*(ptr_res_m[ii+png+0]+ptr_lam[ii+png+0]*ptr_res_d[ii+png+0]);

				}

			}

		}

	}



#endif
