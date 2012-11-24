source('gpuMatMult.R')

dim <- 100L
a <- matrix(runif(dim^2), dim)
gpuMatMult(a, a)
