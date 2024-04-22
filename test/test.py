import prescaledfastadj
import numpy as np
from time import perf_counter as timer


n = 10000
d = 3
sigma = 1.0 # note that this sigma is already chosen to be applied to nodes scaled to [-1/4,1/4]^d
numev = 11

x = np.random.randn(n, d)

##################################################################################
# scale data points equally
points_center = np.mean(x, axis=0)
points = x - points_center

# scale features such that abs(x[:,j]) <= 0.25
# scale values in range [-0.25, 0.25]
for j in range(d):
    m = np.max(np.abs(points[:,j]))
    points[:,j] = points[:,j] / m * 0.25
# determine sigma value in this setting
sigma = 1.0
# determine maximum number of features in window/kernel
dmax = 3
# compute maximum radius possible in dmax dimensions
scaling = np.sqrt(dmax)
# ensure max radius 0.25 for points
points = points / scaling
# scale sigma accordingly
scaledsigma = sigma / scaling

#################################################################################

print("\nTest Gaussian kernel!")

adj_gauss = prescaledfastadj.AdjacencyMatrix(points, np.sqrt(2)*scaledsigma, kernel=1, setup='default', diagonal=1.0)

print("Setup done")

degrees_gauss = adj_gauss.apply(np.ones(n))

print("Avg/min/max degree:", degrees_gauss.mean(), degrees_gauss.min(), degrees_gauss.max())

tic = timer()

nrm_gauss = adj_gauss.normalized_laplacian_norm()

time_nrm_gauss = timer() - tic
print("Normalized Laplacian norm_gauss: {}   (computed in {} seconds)".format(nrm_gauss, time_nrm_gauss))


tic = timer()

w_gauss, U_gauss = adj_gauss.normalized_eigs(numev)

time_eigs_gauss = timer() - tic
print("Time for eigenvalue computation: {} seconds".format(time_eigs_gauss))

d_invsqrt_gauss = 1 / np.sqrt(adj_gauss.apply(np.ones(n)))

for i in range(w_gauss.size):
	res_gauss = np.linalg.norm(d_invsqrt_gauss * adj_gauss.apply(d_invsqrt_gauss * U_gauss[:,i]) - U_gauss[:,i] * w_gauss[i])
	print("Eigenvalue #{}: {:.4f} - Residual: {:.4e}".format(i, w_gauss[i], res_gauss))

#################################################################################

print("\nTest Gaussian derivative kernel!")

adj_der = prescaledfastadj.AdjacencyMatrix(points, np.sqrt(2)*scaledsigma, kernel=2, setup='default', diagonal=0.0)

print("Setup done")

degrees_der = (2/sigma)*adj_der.apply(np.ones(n))

print("Avg/min/max degree:", degrees_der.mean(), degrees_der.min(), degrees_der.max())

tic = timer()

nrm_der = adj_der.normalized_laplacian_norm()

time_nrm_der = timer() - tic
print("Normalized Laplacian norm_gauss: {}   (computed in {} seconds)".format(nrm_der, time_nrm_der))


tic = timer()

w_der, U_der = adj_der.normalized_eigs(numev)

time_eigs_der = timer() - tic
print("Time for eigenvalue computation: {} seconds".format(time_eigs_der))

d_invsqrt_der = 1 / np.sqrt(adj_der.apply(np.ones(n)))

for i in range(w_der.size):
	res_der = np.linalg.norm(d_invsqrt_der * adj_der.apply(d_invsqrt_der * U_der[:,i]) - U_der[:,i] * w_der[i])
	print("Eigenvalue #{}: {:.4f} - Residual: {:.4e}".format(i, w_der[i], res_der))

#################################################################################

print("\nTest Matérn(1/2) kernel!")

adj_matern = prescaledfastadj.AdjacencyMatrix(points, scaledsigma, kernel=3, setup='default', diagonal=1.0)

print("Setup done")

degrees_matern = adj_matern.apply(np.ones(n))

print("Avg/min/max degree:", degrees_matern.mean(), degrees_matern.min(), degrees_matern.max())

tic = timer()

nrm_matern = adj_matern.normalized_laplacian_norm()

time_nrm_matern = timer() - tic
print("Normalized Laplacian norm_gauss: {}   (computed in {} seconds)".format(nrm_matern, time_nrm_matern))


tic = timer()

w_matern, U_matern = adj_matern.normalized_eigs(numev)

time_eigs_matern = timer() - tic
print("Time for eigenvalue computation: {} seconds".format(time_eigs_matern))

d_invsqrt_matern = 1 / np.sqrt(adj_matern.apply(np.ones(n)))

for i in range(w_matern.size):
	res_matern = np.linalg.norm(d_invsqrt_matern * adj_matern.apply(d_invsqrt_matern * U_matern[:,i]) - U_matern[:,i] * w_matern[i])
	print("Eigenvalue #{}: {:.4f} - Residual: {:.4e}".format(i, w_matern[i], res_matern))

#################################################################################

print("\nTest Matérn(1/2) derivative kernel!")

adj_dermat = prescaledfastadj.AdjacencyMatrix(points, scaledsigma, kernel=4, setup='default', diagonal=0.0)

print("Setup done")

degrees_dermat = adj_dermat.apply(np.ones(n))

print("Avg/min/max degree:", degrees_dermat.mean(), degrees_dermat.min(), degrees_dermat.max())

tic = timer()

nrm_dermat = adj_dermat.normalized_laplacian_norm()

time_nrm_dermat = timer() - tic
print("Normalized Laplacian norm_gauss: {}   (computed in {} seconds)".format(nrm_dermat, time_nrm_dermat))


tic = timer()

w_dermat, U_dermat = adj_dermat.normalized_eigs(numev)

time_eigs_dermat = timer() - tic
print("Time for eigenvalue computation: {} seconds".format(time_eigs_dermat))

d_invsqrt_dermat = 1 / np.sqrt(adj_dermat.apply(np.ones(n)))

for i in range(w_dermat.size):
	res_dermat = np.linalg.norm(d_invsqrt_dermat * adj_dermat.apply(d_invsqrt_dermat * U_dermat[:,i]) - U_dermat[:,i] * w_dermat[i])
	print("Eigenvalue #{}: {:.4f} - Residual: {:.4e}".format(i, w_dermat[i], res_dermat))
