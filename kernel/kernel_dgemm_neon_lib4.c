/**************************************************************************************************
*                                                                                                 *
* This file is part of HPMPC.                                                                     *
*                                                                                                 *
* HPMPC -- Library for High-Performance implementation of solvers for MPC.                        *
* Copyright (C) 2014 by Technical Univeristy of Denmark. All rights reserved.                     *
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

// prefetch optimized for Cortex-A9 (cache line is 32 bytes, while A15 is 64 bytes)
void kernel_dgemm_pp_nt_4x4_lib4(int kmax, double *A, double *B, double *C, int bs_dummy, int alg)
	{
	
	if(kmax<=0)
		return;
	
	__builtin_prefetch( A );
	__builtin_prefetch( B );
	__builtin_prefetch( A+4 );
	__builtin_prefetch( B+4 );

	int k_iter = kmax/4;
	int k_left = kmax%4;
	
//	printf("\n%d %d %d\n", kmax, k_iter, k_left);

	__asm__ volatile
	(
		"                                \n\t"
//		"ldr    r0, %2                   \n\t" // load address of A
//		"ldr    r1, %3                   \n\t" // load address of B
		"mov    r0, %2                   \n\t" // load address of A
		"mov    r1, %3                   \n\t" // load address of B
		"                                \n\t"
		"                                \n\t"
//		"ldr    r3, %0                   \n\t" // k_iter
		"mov    r3, %0                   \n\t" // k_iter
		"                                \n\t"
		"                                \n\t"
		"pld    [r0, #64]                \n\t"
		"pld    [r1, #64]                \n\t"
		"pld    [r0, #96]                \n\t"
		"pld    [r1, #96]                \n\t"
		"                                \n\t"
		"                                \n\t"
//		"ldr    r2, %4                   \n\t" // load address of C
		"mov    r2, %4                   \n\t" // load address of C
		"                                \n\t"
		"                                \n\t"
		"fldd   d16, [r0, #0]            \n\t" // prefetch A_even
		"fldd   d17, [r0, #8]            \n\t"
		"fldd   d18, [r0, #16]           \n\t"
		"fldd   d19, [r0, #24]           \n\t"
		"                                \n\t"
		"fldd   d20, [r1, #0]            \n\t" // prefetch B_even
		"fldd   d21, [r1, #8]            \n\t"
		"fldd   d22, [r1, #16]           \n\t"
		"fldd   d23, [r1, #24]           \n\t"
		"                                \n\t"
		"cmp    r3, #0                   \n\t"
		"                                \n\t"
		"fldd   d24, [r0, #32]           \n\t" // prefetch A_odd
		"fldd   d25, [r0, #40]           \n\t"
		"fldd   d26, [r0, #48]           \n\t"
		"fldd   d27, [r0, #56]           \n\t"
		"                                \n\t"
		"fldd   d28, [r1, #32]           \n\t" // prefetch B_odd
		"fldd   d29, [r1, #40]           \n\t"
		"fldd   d30, [r1, #48]           \n\t"
		"fldd   d31, [r1, #56]           \n\t"
		"                                \n\t"
		"                                \n\t"
		"                                \n\t"
		"fldd   d0, .DOUBLEZERO          \n\t" // load zero double
		"fcpyd  d1, d0                   \n\t"
		"fcpyd  d2, d0                   \n\t"
		"fcpyd  d3, d0                   \n\t"
		"fcpyd  d4, d0                   \n\t"
		"fcpyd  d5, d0                   \n\t"
		"fcpyd  d6, d0                   \n\t"
		"fcpyd  d7, d0                   \n\t"
		"fcpyd  d8, d0                   \n\t"
		"fcpyd  d9, d0                   \n\t"
		"fcpyd  d10, d0                  \n\t"
		"fcpyd  d11, d0                  \n\t"
		"fcpyd  d12, d0                  \n\t"
		"fcpyd  d13, d0                  \n\t"
		"fcpyd  d14, d0                  \n\t"
		"fcpyd  d15, d0                  \n\t"
		"                                \n\t"
		"                                \n\t"
		"ble    .DCONSIDERLEFT           \n\t"
		"                                \n\t"
		"                                \n\t"
		".DLOOPKITER:                    \n\t" // main loop
		"                                \n\t"
		"                                \n\t"
		"pld    [r0, #128]               \n\t"
		"pld    [r1, #128]               \n\t"
		"                                \n\t"
		"                                \n\t"
		"fmacd  d0, d16, d20             \n\t"
		"fmacd  d1, d17, d20             \n\t"
		"fmacd  d2, d18, d20             \n\t"
		"fmacd  d3, d19, d20             \n\t"
		"fldd   d20, [r1, #64]           \n\t" // prefetch B_even
		"                                \n\t"
		"fmacd  d4, d16, d21             \n\t"
		"fmacd  d5, d17, d21             \n\t"
		"fmacd  d6, d18, d21             \n\t"
		"fmacd  d7, d19, d21             \n\t"
		"fldd   d21, [r1, #72]           \n\t"
		"                                \n\t"
		"fmacd  d8, d16, d22             \n\t"
		"fmacd  d9, d17, d22             \n\t"
		"fmacd  d10, d18, d22            \n\t"
		"fmacd  d11, d19, d22            \n\t"
		"fldd   d22, [r1, #80]           \n\t"
		"                                \n\t"
		"fmacd  d12, d16, d23            \n\t"
		"fldd   d16, [r0, #64]           \n\t" // prefetch A_even
		"fmacd  d13, d17, d23            \n\t"
		"fldd   d17, [r0, #72]           \n\t"
		"fmacd  d14, d18, d23            \n\t"
		"fldd   d18, [r0, #80]           \n\t"
		"fmacd  d15, d19, d23            \n\t"
		"fldd   d23, [r1, #88]           \n\t"
		"                                \n\t"
		"                                \n\t"
/*		"pld    [r0, #192]               \n\t"*/
/*		"pld    [r1, #192]               \n\t"*/
		"pld    [r0, #160]               \n\t"
		"pld    [r1, #160]               \n\t"
		"                                \n\t"
		"                                \n\t"
		"fmacd  d0, d24, d28             \n\t"
		"fldd   d19, [r0, #88]           \n\t"
		"fmacd  d1, d25, d28             \n\t"
		"sub    r3, r3, #1               \n\t" // iter++
		"fmacd  d2, d26, d28             \n\t"
		"fmacd  d3, d27, d28             \n\t"
		"fldd   d28, [r1, #96]           \n\t" // prefetch B_odd
		"                                \n\t"
		"fmacd  d4, d24, d29             \n\t"
		"fmacd  d5, d25, d29             \n\t"
		"fmacd  d6, d26, d29             \n\t"
		"fmacd  d7, d27, d29             \n\t"
		"fldd   d29, [r1, #104]          \n\t"
		"                                \n\t"
		"fmacd  d8, d24, d30             \n\t"
		"fmacd  d9, d25, d30             \n\t"
		"fmacd  d10, d26, d30            \n\t"
		"fmacd  d11, d27, d30            \n\t"
		"fldd   d30, [r1, #112]          \n\t"
		"                                \n\t"
		"fmacd  d12, d24, d31            \n\t"
		"fldd   d24, [r0, #96]           \n\t" // prefetch A_odd
		"fmacd  d13, d25, d31            \n\t"
		"fldd   d25, [r0, #104]          \n\t"
		"fmacd  d14, d26, d31            \n\t"
		"fldd   d26, [r0, #112]          \n\t"
		"fmacd  d15, d27, d31            \n\t"
		"fldd   d31, [r1, #120]          \n\t"
		"                                \n\t"
		"                                \n\t"
/*		"pld    [r0, #256]               \n\t"*/
/*		"pld    [r1, #256]               \n\t"*/
		"pld    [r0, #192]               \n\t"
		"pld    [r1, #192]               \n\t"
		"                                \n\t"
		"                                \n\t"
		"fmacd  d0, d16, d20             \n\t"
		"fldd   d27, [r0, #120]          \n\t"
		"fmacd  d1, d17, d20             \n\t"
		"cmp    r3, #0                   \n\t" // next iter?
		"fmacd  d2, d18, d20             \n\t"
		"fmacd  d3, d19, d20             \n\t"
		"fldd   d20, [r1, #128]          \n\t" // prefetch B_even
		"                                \n\t"
		"fmacd  d4, d16, d21             \n\t"
		"fmacd  d5, d17, d21             \n\t"
		"fmacd  d6, d18, d21             \n\t"
		"fmacd  d7, d19, d21             \n\t"
		"fldd   d21, [r1, #136]          \n\t"
		"                                \n\t"
		"fmacd  d8, d16, d22             \n\t"
		"fmacd  d9, d17, d22             \n\t"
		"fmacd  d10, d18, d22            \n\t"
		"fmacd  d11, d19, d22            \n\t"
		"fldd   d22, [r1, #144]          \n\t"
		"                                \n\t"
		"fmacd  d12, d16, d23            \n\t"
		"fldd   d16, [r0, #128]          \n\t" // prefetch A_even
		"fmacd  d13, d17, d23            \n\t"
		"fldd   d17, [r0, #136]          \n\t"
		"fmacd  d14, d18, d23            \n\t"
		"fldd   d18, [r0, #144]          \n\t"
		"fmacd  d15, d19, d23            \n\t"
		"fldd   d19, [r0, #152]          \n\t"
		"                                \n\t"
		"                                \n\t"
/*		"pld    [r0, #384]               \n\t"*/
/*		"pld    [r1, #384]               \n\t"*/
		"pld    [r0, #224]               \n\t"
		"pld    [r1, #224]               \n\t"
		"                                \n\t"
		"                                \n\t"
		"fmacd  d0, d24, d28             \n\t"
		"add    r0, r0, #128             \n\t" // increase A
		"fmacd  d1, d25, d28             \n\t"
		"fldd   d23, [r1, #152]          \n\t"
		"fmacd  d2, d26, d28             \n\t"
		"add    r1, r1, #128             \n\t" // increase B
		"fmacd  d3, d27, d28             \n\t"
		"fldd   d28, [r1, #32]           \n\t" // prefetch B_odd
		"                                \n\t"
		"fmacd  d4, d24, d29             \n\t"
		"fmacd  d5, d25, d29             \n\t"
		"fmacd  d6, d26, d29             \n\t"
		"fmacd  d7, d27, d29             \n\t"
		"fldd   d29, [r1, #40]           \n\t"
		"                                \n\t"
		"fmacd  d8, d24, d30             \n\t"
		"fmacd  d9, d25, d30             \n\t"
		"fmacd  d10, d26, d30            \n\t"
		"fmacd  d11, d27, d30            \n\t"
		"fldd   d30, [r1, #48]           \n\t"
		"                                \n\t"
		"fmacd  d12, d24, d31            \n\t"
		"fldd   d24, [r0, #32]           \n\t" // prefetch A_odd
		"fmacd  d13, d25, d31            \n\t"
		"fldd   d25, [r0, #40]           \n\t"
		"fmacd  d14, d26, d31            \n\t"
		"fldd   d26, [r0, #48]           \n\t"
		"fmacd  d15, d27, d31            \n\t"
		"fldd   d31, [r1, #56]           \n\t"
		"fldd   d27, [r0, #56]           \n\t"
		"                                \n\t"
		"                                \n\t"
		"                                \n\t"
		"bgt    .DLOOPKITER              \n\t"
		"                                \n\t"
		"                                \n\t"
		".DCONSIDERLEFT:                 \n\t" // consider left
		"                                \n\t"
//		"pldw   [r2, #0]                 \n\t" // prefetch C
//		"pldw   [r2, #32]                \n\t"
//		"pldw   [r2, #64]                \n\t"
//		"pldw   [r2, #96]                \n\t"
		"                                \n\t"
		"                                \n\t"
		"                                \n\t"
//		"ldr    r4, %1                   \n\t" // k_left
		"mov    r4, %1                   \n\t" // k_left
		"cmp    r4, #0                   \n\t"
		"ble    .DPOSTACCUM              \n\t"
		"                                \n\t"
		"                                \n\t"
		"                                \n\t"
		".DLOOPKLEFT:                    \n\t" // clean up loop
		"                                \n\t"
		"sub    r4, r4, #1               \n\t"
		"                                \n\t"
		"fmacd  d0, d16, d20             \n\t"
		"fmacd  d1, d17, d20             \n\t"
		"fmacd  d2, d18, d20             \n\t"
		"fmacd  d3, d19, d20             \n\t"
		"fldd   d20, [r1, #32]           \n\t" // prefetch B_even
		"                                \n\t"
		"fmacd  d4, d16, d21             \n\t"
		"fmacd  d5, d17, d21             \n\t"
		"fmacd  d6, d18, d21             \n\t"
		"fmacd  d7, d19, d21             \n\t"
		"fldd   d21, [r1, #40]           \n\t"
		"                                \n\t"
		"fmacd  d8, d16, d22             \n\t"
		"fmacd  d9, d17, d22             \n\t"
		"fmacd  d10, d18, d22            \n\t"
		"fmacd  d11, d19, d22            \n\t"
		"fldd   d22, [r1, #48]           \n\t"
		"                                \n\t"
		"cmp    r4, #0                   \n\t"
		"                                \n\t"
		"fmacd  d12, d16, d23            \n\t"
		"fldd   d16, [r0, #32]           \n\t" // prefetch A_even
		"fmacd  d13, d17, d23            \n\t"
		"fldd   d17, [r0, #40]           \n\t"
		"fmacd  d14, d18, d23            \n\t"
		"fldd   d18, [r0, #48]           \n\t"
		"fmacd  d15, d19, d23            \n\t"
		"fldd   d19, [r0, #56]           \n\t"
		"add    r0, r0, #32              \n\t"
		"fldd   d23, [r1, #56]           \n\t"
		"add    r1, r1, #32              \n\t"
		"                                \n\t"
		"                                \n\t"
		"bgt    .DLOOPKLEFT              \n\t"
		"                                \n\t"
		"                                \n\t"
		"                                \n\t"
		".DPOSTACCUM:                    \n\t"
		"                                \n\t"
		"                                \n\t"
//		"ldr    r5, %5                   \n\t" // alg
		"mov    r5, %5                   \n\t" // alg
		"cmp    r5, #0                   \n\t"
		"beq    .D0                      \n\t" // if alg==0, jump
		"                                \n\t"
		"cmp    r5, #1                   \n\t"
		"                                \n\t"
		"fldd   d16, [r2, #0]            \n\t" // load C elements
		"fldd   d17, [r2, #8]            \n\t"
		"fldd   d18, [r2, #16]           \n\t"
		"fldd   d19, [r2, #24]           \n\t"
		"                                \n\t"
		"fldd   d20, [r2, #32]           \n\t"
		"fldd   d21, [r2, #40]           \n\t"
		"fldd   d22, [r2, #48]           \n\t"
		"fldd   d23, [r2, #56]           \n\t"
		"                                \n\t"
		"fldd   d24, [r2, #64]           \n\t"
		"fldd   d25, [r2, #72]           \n\t"
		"fldd   d26, [r2, #80]           \n\t"
		"fldd   d27, [r2, #88]           \n\t"
		"                                \n\t"
		"fldd   d28, [r2, #96]           \n\t"
		"fldd   d29, [r2, #104]          \n\t"
		"fldd   d30, [r2, #112]          \n\t"
		"fldd   d31, [r2, #120]          \n\t"
		"                                \n\t"
		"beq    .D1                      \n\t" // if alg==1, jump
		"                                \n\t"
		"                                \n\t"// alg==-1
		"fsubd  d0, d16, d0              \n\t"
		"fsubd  d1, d17, d1              \n\t"
		"fsubd  d2, d18, d2              \n\t"
		"fsubd  d3, d19, d3              \n\t"
		"                                \n\t"
		"fsubd  d4, d20, d4              \n\t"
		"fsubd  d5, d21, d5              \n\t"
		"fsubd  d6, d22, d6              \n\t"
		"fsubd  d7, d23, d7              \n\t"
		"                                \n\t"
		"fsubd  d8, d24, d8              \n\t"
		"fsubd  d9, d25, d9              \n\t"
		"fsubd  d10, d26, d10            \n\t"
		"fsubd  d11, d27, d11            \n\t"
		"                                \n\t"
		"fsubd  d12, d28, d12            \n\t"
		"fsubd  d13, d29, d13            \n\t"
		"fsubd  d14, d30, d14            \n\t"
		"fsubd  d15, d31, d15            \n\t"
		"                                \n\t"
		"b      .D0                      \n\t" // jump to end
		"                                \n\t"
		"                                \n\t"
		".D1:                            \n\t" // alg==1
		"                                \n\t"
		"faddd  d0, d0, d16              \n\t"
		"faddd  d1, d1, d17              \n\t"
		"faddd  d2, d2, d18              \n\t"
		"faddd  d3, d3, d19              \n\t"
		"                                \n\t"
		"faddd  d4, d4, d20              \n\t"
		"faddd  d5, d5, d21              \n\t"
		"faddd  d6, d6, d22              \n\t"
		"faddd  d7, d7, d23              \n\t"
		"                                \n\t"
		"faddd  d8, d8, d24              \n\t"
		"faddd  d9, d9, d25              \n\t"
		"faddd  d10, d10, d26            \n\t"
		"faddd  d11, d11, d27            \n\t"
		"                                \n\t"
		"faddd  d12, d12, d28            \n\t"
		"faddd  d13, d13, d29            \n\t"
		"faddd  d14, d14, d30            \n\t"
		"faddd  d15, d15, d31            \n\t"
		"                                \n\t"
		".D0:                            \n\t" // alg==0
		"                                \n\t"
		"fstd   d0, [r2, #0]             \n\t" // store result
		"fstd   d1, [r2, #8]             \n\t"
		"fstd   d2, [r2, #16]            \n\t"
		"fstd   d3, [r2, #24]            \n\t"
		"                                \n\t"
		"fstd   d4, [r2, #32]            \n\t"
		"fstd   d5, [r2, #40]            \n\t"
		"fstd   d6, [r2, #48]            \n\t"
		"fstd   d7, [r2, #56]            \n\t"
		"                                \n\t"
		"fstd   d8, [r2, #64]            \n\t"
		"fstd   d9, [r2, #72]            \n\t"
		"fstd   d10, [r2, #80]           \n\t"
		"fstd   d11, [r2, #88]           \n\t"
		"                                \n\t"
		"fstd   d12, [r2, #96]           \n\t"
		"fstd   d13, [r2, #104]          \n\t"
		"fstd   d14, [r2, #112]          \n\t"
		"fstd   d15, [r2, #120]          \n\t"
		"                                \n\t"
		"                                \n\t"
		"                                \n\t"
		".align 3                        \n\t"
		".DOUBLEZERO:                    \n\t" // zero double word
		".word  0                        \n\t"
		".word  0                        \n\t"
		"                                \n\t"
		"                                \n\t"
		"                                \n\t"
		: // output operands (none)
		: // input operands
		  "r" (k_iter),		// %0
		  "r" (k_left),		// %1
		  "r" (A),			// %2
		  "r" (B),			// %3
		  "r" (C),			// %4
		  "r" (alg)			// %5
		: // register clobber list
		  "r0", "r1", "r2", "r3", "r4", "r5",
		  "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
		  "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15",
		  "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
		  "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
		  "memory"
	);
}



// normal-transposed, 4x3 with data packed in 4
void kernel_dgemm_pp_nt_4x3_lib4(int kmax, double *A, double *B, double *C, int ldc, int alg)
	{
	
	if(kmax<=0)
		return;

/*	const int bs = 4;*/

	int k;
	
	double
		a_0, a_1, a_2, a_3,
		b_0, b_1, b_2,
		c_00=0, c_01=0, c_02=0,
		c_10=0, c_11=0, c_12=0,
		c_20=0, c_21=0, c_22=0,
		c_30=0, c_31=0, c_32=0;
	
	k = 0;
	for(; k<kmax-3; k+=4)
		{

		a_0 = A[0];
		a_1 = A[1];
		a_2 = A[2];
		a_3 = A[3];
		
		b_0 = B[0];
		b_1 = B[1];
		b_2 = B[2];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;

		c_02 += a_0 * b_2;
		c_12 += a_1 * b_2;
		c_22 += a_2 * b_2;
		c_32 += a_3 * b_2;
		
		
		a_0 = A[4];
		a_1 = A[5];
		a_2 = A[6];
		a_3 = A[7];
		
		b_0 = B[4];
		b_1 = B[5];
		b_2 = B[6];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;

		c_02 += a_0 * b_2;
		c_12 += a_1 * b_2;
		c_22 += a_2 * b_2;
		c_32 += a_3 * b_2;
		
		
		a_0 = A[8];
		a_1 = A[9];
		a_2 = A[10];
		a_3 = A[11];
		
		b_0 = B[8];
		b_1 = B[9];
		b_2 = B[10];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;

		c_02 += a_0 * b_2;
		c_12 += a_1 * b_2;
		c_22 += a_2 * b_2;
		c_32 += a_3 * b_2;
		
		
		a_0 = A[12];
		a_1 = A[13];
		a_2 = A[14];
		a_3 = A[15];
		
		b_0 = B[12];
		b_1 = B[13];
		b_2 = B[14];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;

		c_02 += a_0 * b_2;
		c_12 += a_1 * b_2;
		c_22 += a_2 * b_2;
		c_32 += a_3 * b_2;
		
		A += 16;
		B += 16;

		}
	
	for(; k<kmax; k++)
		{

		a_0 = A[0];
		a_1 = A[1];
		a_2 = A[2];
		a_3 = A[3];
		
		b_0 = B[0];
		b_1 = B[1];
		b_2 = B[2];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;

		c_02 += a_0 * b_2;
		c_12 += a_1 * b_2;
		c_22 += a_2 * b_2;
		c_32 += a_3 * b_2;
		
		A += 4;
		B += 4;
		
		}

	if(alg==0)
		{
		C[0+ldc*0] = c_00;
		C[1+ldc*0] = c_10;
		C[2+ldc*0] = c_20;
		C[3+ldc*0] = c_30;

		C[0+ldc*1] = c_01;
		C[1+ldc*1] = c_11;
		C[2+ldc*1] = c_21;
		C[3+ldc*1] = c_31;

		C[0+ldc*2] = c_02;
		C[1+ldc*2] = c_12;
		C[2+ldc*2] = c_22;
		C[3+ldc*2] = c_32;
		}
	else if(alg==1)
		{
		C[0+ldc*0] += c_00;
		C[1+ldc*0] += c_10;
		C[2+ldc*0] += c_20;
		C[3+ldc*0] += c_30;

		C[0+ldc*1] += c_01;
		C[1+ldc*1] += c_11;
		C[2+ldc*1] += c_21;
		C[3+ldc*1] += c_31;

		C[0+ldc*2] += c_02;
		C[1+ldc*2] += c_12;
		C[2+ldc*2] += c_22;
		C[3+ldc*2] += c_32;
		}
	else
		{
		C[0+ldc*0] -= c_00;
		C[1+ldc*0] -= c_10;
		C[2+ldc*0] -= c_20;
		C[3+ldc*0] -= c_30;

		C[0+ldc*1] -= c_01;
		C[1+ldc*1] -= c_11;
		C[2+ldc*1] -= c_21;
		C[3+ldc*1] -= c_31;

		C[0+ldc*2] -= c_02;
		C[1+ldc*2] -= c_12;
		C[2+ldc*2] -= c_22;
		C[3+ldc*2] -= c_32;
		}

	}



// normal-transposed, 4x2 with data packed in 4
void kernel_dgemm_pp_nt_4x2_lib4(int kmax, double *A, double *B, double *C, int ldc, int alg)
	{
	
	if(kmax<=0)
		return;

/*	const int ldc = 4;*/

	int k;
	
	double
		a_0, a_1, a_2, a_3,
		b_0, b_1,
		c_00=0, c_01=0,
		c_10=0, c_11=0,
		c_20=0, c_21=0,
		c_30=0, c_31=0;
	
	k = 0;
	for(; k<kmax-3; k+=4)
		{

		a_0 = A[0];
		a_1 = A[1];
		a_2 = A[2];
		a_3 = A[3];
		
		b_0 = B[0];
		b_1 = B[1];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;
		
		
		a_0 = A[4];
		a_1 = A[5];
		a_2 = A[6];
		a_3 = A[7];
		
		b_0 = B[4];
		b_1 = B[5];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;
		
		
		a_0 = A[8];
		a_1 = A[9];
		a_2 = A[10];
		a_3 = A[11];
		
		b_0 = B[8];
		b_1 = B[9];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;
		
		
		a_0 = A[12];
		a_1 = A[13];
		a_2 = A[14];
		a_3 = A[15];
		
		b_0 = B[12];
		b_1 = B[13];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;
		
		A += 16;
		B += 16;

		}
	
	for(; k<kmax; k++)
		{

		a_0 = A[0];
		a_1 = A[1];
		a_2 = A[2];
		a_3 = A[3];
		
		b_0 = B[0];
		b_1 = B[1];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		c_01 += a_0 * b_1;
		c_11 += a_1 * b_1;
		c_21 += a_2 * b_1;
		c_31 += a_3 * b_1;
		
		A += 4;
		B += 4;
		
		}

	if(alg==0)
		{
		C[0+ldc*0] = c_00;
		C[1+ldc*0] = c_10;
		C[2+ldc*0] = c_20;
		C[3+ldc*0] = c_30;

		C[0+ldc*1] = c_01;
		C[1+ldc*1] = c_11;
		C[2+ldc*1] = c_21;
		C[3+ldc*1] = c_31;
		}
	else if(alg==1)
		{
		C[0+ldc*0] += c_00;
		C[1+ldc*0] += c_10;
		C[2+ldc*0] += c_20;
		C[3+ldc*0] += c_30;

		C[0+ldc*1] += c_01;
		C[1+ldc*1] += c_11;
		C[2+ldc*1] += c_21;
		C[3+ldc*1] += c_31;
		}
	else
		{
		C[0+ldc*0] -= c_00;
		C[1+ldc*0] -= c_10;
		C[2+ldc*0] -= c_20;
		C[3+ldc*0] -= c_30;

		C[0+ldc*1] -= c_01;
		C[1+ldc*1] -= c_11;
		C[2+ldc*1] -= c_21;
		C[3+ldc*1] -= c_31;
		}

	}



// normal-transposed, 4x1 with data packed in 4
void kernel_dgemm_pp_nt_4x1_lib4(int kmax, double *A, double *B, double *C, int ldc, int alg)
	{
	
	if(kmax<=0)
		return;

/*	const int ldc = 4;*/

	int k;
	
	double
		a_0, a_1, a_2, a_3,
		b_0,
		c_00=0,
		c_10=0,
		c_20=0,
		c_30=0;
	
	k = 0;
	for(; k<kmax-3; k+=4)
		{

		a_0 = A[0];
		a_1 = A[1];
		a_2 = A[2];
		a_3 = A[3];
		
		b_0 = B[0];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		
		a_0 = A[4];
		a_1 = A[5];
		a_2 = A[6];
		a_3 = A[7];
		
		b_0 = B[4];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		
		a_0 = A[8];
		a_1 = A[9];
		a_2 = A[10];
		a_3 = A[11];
		
		b_0 = B[8];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		
		a_0 = A[12];
		a_1 = A[13];
		a_2 = A[14];
		a_3 = A[15];
		
		b_0 = B[12];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		A += 16;
		B += 16;

		}
	
	for(; k<kmax; k++)
		{

		a_0 = A[0];
		a_1 = A[1];
		a_2 = A[2];
		a_3 = A[3];
		
		b_0 = B[0];
		
		c_00 += a_0 * b_0;
		c_10 += a_1 * b_0;
		c_20 += a_2 * b_0;
		c_30 += a_3 * b_0;
		
		A += 4;
		B += 4;
		
		}

	if(alg==0)
		{
		C[0+ldc*0] = c_00;
		C[1+ldc*0] = c_10;
		C[2+ldc*0] = c_20;
		C[3+ldc*0] = c_30;
		}
	else if(alg==1)
		{
		C[0+ldc*0] += c_00;
		C[1+ldc*0] += c_10;
		C[2+ldc*0] += c_20;
		C[3+ldc*0] += c_30;
		}
	else
		{
		C[0+ldc*0] -= c_00;
		C[1+ldc*0] -= c_10;
		C[2+ldc*0] -= c_20;
		C[3+ldc*0] -= c_30;
		}

	}

