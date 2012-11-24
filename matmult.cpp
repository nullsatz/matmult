#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<Rcpp.h>
using namespace Rcpp;

#include<cuda.h>
#include<cuda_runtime.h>
#include<cublas_v2.h>

RcppExport SEXP gpuMatMult(SEXP a, SEXP b) {

	size_t free_mem, total_mem;
	cudaError_t cudaStat = cudaMemGetInfo(&free_mem, &total_mem);
	if(cudaStat != cudaSuccess)
		Rf_error("error fetching mem info from cuda device\n");

	double
		* gpua, * gpub, * gpuc;

	SEXP dim_a = Rf_getAttrib(a, R_DimSymbol);
	R_len_t
		rowsa = INTEGER(dim_a)[0], colsa = INTEGER(dim_a)[1];

	SEXP dim_b = Rf_getAttrib(b, R_DimSymbol);
	R_len_t
		rowsb = INTEGER(dim_b)[0], colsb = INTEGER(dim_b)[1];

	R_len_t gpu_mem_needed = sizeof(double) *
		(rowsa * colsa + rowsb * colsb + rowsa * colsb);
	
	if(gpu_mem_needed > free_mem) {
		Rprintf("needed: %ld, free: %ld, total: %ld\n",
			gpu_mem_needed, free_mem, total_mem);
		Rf_error("cuda device does not have enough free memory to perform this operation\n");
	}

	cudaStat = cudaMalloc((void**) &gpua, rowsa * colsa * sizeof(double));
	if (cudaStat != cudaSuccess) {
		cudaFree(gpua);
		Rf_error("gpu memory allocation failed\n");
	}

	cudaStat = cudaMalloc((void**) &gpub, rowsb * colsb * sizeof(double));
	if (cudaStat != cudaSuccess) {
		cudaFree(gpua);
		cudaFree(gpub);
		Rf_error("gpu memory allocation failed\n");
	}

	cudaStat = cudaMalloc((void**) &gpuc, rowsa * colsb * sizeof(double));
	if (cudaStat != cudaSuccess) {
		cudaFree(gpua);
		cudaFree(gpub);
		cudaFree(gpuc);
		Rf_error("gpu memory allocation failed\n");
	}

	cublasHandle_t handle;
	cublasStatus_t stat = cublasCreate(&handle);
	if(stat != CUBLAS_STATUS_SUCCESS) {
		cudaFree(gpua);
		cudaFree(gpub);
		cudaFree(gpuc);
		cublasDestroy(handle);
		Rf_error("gpu initialization failed\n");
	}

	stat = cublasSetMatrix(rowsa, colsa, sizeof(double), REAL(a), rowsa,
		gpua, rowsa);
	if(stat != CUBLAS_STATUS_SUCCESS) {
		cudaFree(gpua);
		cudaFree(gpub);
		cudaFree(gpuc);
		cublasDestroy(handle);
		Rf_error("moving data to gpu failed\n");
	}

	stat = cublasSetMatrix(rowsb, colsb, sizeof(double), REAL(b), rowsb,
		gpub, rowsb);
	if(stat != CUBLAS_STATUS_SUCCESS) {
		cudaFree(gpua);
		cudaFree(gpub);
		cudaFree(gpuc);
		cublasDestroy(handle);
		Rf_error("moving data to gpu failed\n");
	}

	const double alpha = 1.0, beta = 0.0;
	stat = cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, rowsa, colsb, colsa,
		&alpha, (const double *) gpua, rowsa, (const double *) gpub, rowsb,
		&beta, gpuc, rowsa);
	if(stat != CUBLAS_STATUS_SUCCESS) {
		cudaFree(gpua);
		cudaFree(gpub);
		cudaFree(gpuc);
		cublasDestroy(handle);
		Rf_error("gpu matrix computation failed\n");
	}

	SEXP ab;
	PROTECT(ab = Rf_allocVector(REALSXP, rowsa * colsb));
	double * rab = REAL(ab);

	stat = cublasGetMatrix(rowsa, colsb, sizeof(double), gpuc, rowsa,
		rab, rowsa);
	if(stat != CUBLAS_STATUS_SUCCESS) {
		cudaFree(gpua);
		cudaFree(gpub);
		cudaFree(gpuc);
		cublasDestroy(handle);
		UNPROTECT(1);
		Rf_error("moving data from gpu failed\n");
	}

	cudaFree(gpua);
	cudaFree(gpub);
	cudaFree(gpuc);

	cublasDestroy(handle);

	SEXP dim_ab;
	PROTECT(dim_ab = Rf_allocVector(INTSXP, 2));
	INTEGER(dim_ab)[0] = rowsa;
	INTEGER(dim_ab)[1] = colsb;
	Rf_setAttrib(ab, R_DimSymbol, dim_ab);

	UNPROTECT(2);
	return ab;
}
