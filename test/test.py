import fastadj
import prescaledfastadj
import numpy as np
#from time import perf_counter as timer

n = 10000
d = 3
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

##################################################################################

#adj = fastadj.AdjacencyMatrix(x, sigma, 'default')
adj = prescaledfastadj.AdjacencyMatrix(points, scaledsigma, 'default')

print("Setup done")

degrees = adj.apply(np.ones(n))
print("degrees:", degrees)


##################################################################################
# =============================================================================
# print("Avg/min/max degree:", degrees.mean(), degrees.min(), degrees.max())
# 
# 
# tic = timer()
# 
# nrm = adj.normalized_laplacian_norm()
# 
# time_nrm = timer() - tic
# print("Normalized Laplacian norm: {}   (computed in {} seconds)".format(nrm, time_nrm))
# 
# 
# tic = timer()
# 
# w, U = adj.normalized_eigs(numev)
# 
# time_eigs = timer() - tic
# print("Time for eigenvalue computation: {} seconds".format(time_eigs))
# 
# d_invsqrt = 1 / np.sqrt(adj.apply(np.ones(n)))
# 
# for i in range(w.size):
# 	res = np.linalg.norm(d_invsqrt * adj.apply(d_invsqrt * U[:,i]) - U[:,i] * w[i])
# 	print("Eigenvalue #{}: {:.4f} - Residual: {:.4e}".format(i, w[i], res))
# =============================================================================

