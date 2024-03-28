

#include <Python.h>
#include "numpy/arrayobject.h"
#include "structmember.h"

#include <stdio.h>
#include <string.h>
#include <complex.h>
#include <math.h>

#include "nfft3.h"
#include "fastsum.h"
#include "kernels.h"

#ifdef BUILD_EIGS
#include "arpack.h"
#endif

typedef struct {
    PyObject_HEAD
    //char kernel;
    int kernel;
    int d;
    double sigma;
    int N;
    int p;
    int m;
    double eps;
    int NN;
    
    double diagonal;
    int n;
    
    fastsum_plan* fastsum;
} AdjacencyCoreObject;

static int
check_fastsum(AdjacencyCoreObject* self)
{
    if (self->fastsum)
        return 1;
    PyErr_SetString(PyExc_RuntimeError, "Invalid NFFT fastsum object");
    return 0;
}

static void
remove_points(AdjacencyCoreObject* self)
{
    if (self->n) {
       	fastsum_finalize_target_nodes(self->fastsum);
       	fastsum_finalize_source_nodes(self->fastsum);
       	
        self->fastsum->x = NULL;
        self->fastsum->y = NULL;
        self->fastsum->alpha = NULL;
        self->fastsum->f = NULL;
        self->n = 0;
    }
}

static void 
AdjacencyCore_dealloc(AdjacencyCoreObject* self)
{
    if (self->fastsum) {
        remove_points(self);
    	fastsum_finalize_kernel(self->fastsum);
    	nfft_free(self->fastsum);
    	self->fastsum = NULL;
    }
    
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static int
AdjacencyCore_init(AdjacencyCoreObject *self, PyObject *args, PyObject *kwds)
{
    if (!PyArg_ParseTuple(args, "iidiiid|i", &self->kernel, &self->d, &self->sigma, &self->N, &self->p, &self->m, &self->eps, &self->NN))
        return -1;
        
    if (self->NN == 0) {
        self->NN = 2;
        while (2*self->N > self->NN)
            self->NN *= 2;
    }
    
    //TODO
    self->diagonal = 0.0;
    
    self->fastsum = nfft_malloc(sizeof(fastsum_plan));
    
    // init fastsum with chosen kernel
    //char kernel[] = "gaussian";
    
//     if (strcmp(self->kernel,"gaussian") == 0) {
//         fastsum_init_guru_kernel(self->fastsum, self->d, gaussian, &(self->sigma), 
//         STORE_PERMUTATION_X_ALPHA, self->N, self->p, 0.0, self->eps);
//     }
//     else if (strcmp(self->kernel,"xx_gaussian") == 0) {
//         fastsum_init_guru_kernel(self->fastsum, self->d, xx_gaussian, &(self->sigma), 
//         STORE_PERMUTATION_X_ALPHA, self->N, self->p, 0.0, self->eps);
//     }
//     else {
//         fastsum_init_guru_kernel(self->fastsum, self->d, gaussian, &(self->sigma), 
//         STORE_PERMUTATION_X_ALPHA, self->N, self->p, 0.0, self->eps);
//     }
    
    //int kernel = 2;
    
    if (self->kernel == 1) { // Gaussian kernel
        fastsum_init_guru_kernel(self->fastsum, self->d, gaussian, &(self->sigma), 
        STORE_PERMUTATION_X_ALPHA, self->N, self->p, 0.0, self->eps);
    }
    else if (self->kernel == 2) { // xx_Gaussian kernel
        fastsum_init_guru_kernel(self->fastsum, self->d, xx_gaussian, &(self->sigma), 
        STORE_PERMUTATION_X_ALPHA, self->N, self->p, 0.0, self->eps);
    }
    else if (self->kernel == 3) { // laplacian_rbf kernel
        fastsum_init_guru_kernel(self->fastsum, self->d, laplacian_rbf, &(self->sigma), 
        STORE_PERMUTATION_X_ALPHA, self->N, self->p, 0.0, self->eps);
    }
    else {
        fastsum_init_guru_kernel(self->fastsum, self->d, gaussian, &(self->sigma), 
        STORE_PERMUTATION_X_ALPHA, self->N, self->p, 0.0, self->eps);
    }
    
    self->fastsum->x = NULL;
    self->fastsum->y = NULL;
    self->fastsum->alpha = NULL;
    self->fastsum->f = NULL;
    self->n = 0;
    
    return 0;
}


static PyObject *
AdjacencyCore_getpoints(AdjacencyCoreObject* self, void* closure)
{
    npy_intp dims[2];
    PyObject* array;
    
    if (!check_fastsum(self))
         return NULL;
    
    if (!self->n) {
        Py_RETURN_NONE;
    }
    else {    
        dims[0] = self->n;
        dims[1] = self->d;
        
        array = PyArray_SimpleNew(2, dims, NPY_DOUBLE);
        PyArray_ENABLEFLAGS((PyArrayObject*)array, NPY_OWNDATA);
        
        memcpy(PyArray_DATA((PyArrayObject*)array), self->fastsum->x, dims[0]*dims[1]*sizeof(double));
        
        return array;
    }
}

static int
AdjacencyCore_setpoints(AdjacencyCoreObject* self, PyObject* arg, void* closure)
{
    int i, j, n, d=self->d;
    PyObject* item;
    double x;
    PyArrayObject* array;
    
    if (!check_fastsum(self))
        return -1;
    
    remove_points(self);
    
    if (arg == NULL || arg == Py_None)
        return 0;
    
    if (!PyArray_Converter(arg, (PyObject**) &array)) {
        PyErr_Format(PyExc_TypeError, "AdjacencyCore.points must be a 2D numpy array with %d columns", d);
        return -1;
    }
    
    if (PyArray_NDIM(array) != 2 || PyArray_DIM(array, 1) != d) {
        PyErr_Format(PyExc_TypeError, "AdjacencyCore.points must be a 2D numpy array with %d columns", d);
        Py_DECREF(array);
        return -1;
    }
    
    n = PyArray_DIM(array, 0);
    if (n > 0) {
        self->n = n;
    	fastsum_init_guru_source_nodes(self->fastsum, n, self->NN, self->m);
    	fastsum_init_guru_target_nodes(self->fastsum, n, self->NN, self->m);
        
        for (i=0; i<n; ++i) {
            for (j=0; j<d; ++j) {
                item = PyArray_GETITEM(array, PyArray_GETPTR2(array, i, j));
                x = PyFloat_AsDouble(item);
                Py_DECREF(item);
                self->fastsum->x[i*d+j] = x;
                self->fastsum->y[i*d+j] = x;
            }
        }
        
        // PyFloat_AsDouble simply returns -1.0 on errors
        if (PyErr_Occurred()) {
            PyErr_SetString(PyExc_TypeError, "AdjacencyCore.points items must be floating point numbers");
            Py_DECREF(array);
            remove_points(self);
            return -1;
        }
        
        fastsum_precompute(self->fastsum);
    }
    
    Py_DECREF(array);
    return 0;
}

static PyObject *
AdjacencyCore_apply(AdjacencyCoreObject* self, PyObject* args, PyObject *keywds)
{
    int i, n, exact=0;
    PyArrayObject* array;
    PyObject* item;
    double* data;
    static char *kwlist[] = {"points", "exact", NULL};

    if (!check_fastsum(self))
        return NULL;
    
    n = self->n;
    if (!n) {
        PyErr_SetString(PyExc_RuntimeError, "AdjacencyCore.points must be given before calling AdjacencyCore.apply");
        return NULL;
    }
    
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "O&|p", kwlist, PyArray_Converter, &array, &exact)) {
        PyErr_Format(PyExc_TypeError, "Invalid input to AdjacencyCore.apply");
        return NULL;
    }
    
    if (PyArray_NDIM(array) != 1 || PyArray_DIM(array, 0) != n) {
        PyErr_Format(PyExc_ValueError, "First input to AdjacencyCore.apply must be a 1D numpy array with %d entries", n);
        Py_DECREF(array);
        return NULL;
    }
    
    for (i=0; i<n; ++i) {
        item = PyArray_GETITEM(array, PyArray_GETPTR1(array, i));
        self->fastsum->alpha[i] = CMPLX(PyFloat_AsDouble(item), 0.0);
        Py_DECREF(item);
    }
    
    Py_DECREF(array);
    array = NULL;
    
    // PyFloat_AsDouble simply returns -1.0 on errors
    if (PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError, "AdjacencyCore.apply requires a vector of floating point numbers");
        return NULL;
    }
    
    if (exact)
        fastsum_exact(self->fastsum);
    else
        fastsum_trafo(self->fastsum);

    array = (PyArrayObject*) PyArray_SimpleNew(1, (npy_intp*) &n, NPY_DOUBLE);
    PyArray_ENABLEFLAGS(array, NPY_OWNDATA);
    
    data = (double*) PyArray_DATA(array);
    
//     for (i=0; i<n; ++i) {
//         data[i] = CREAL(self->fastsum->f[i]) + (self->diagonal - 1.0)*CREAL(self->fastsum->alpha[i]);
//     }
    
    if (self->kernel == 1) { // gaussian kernel
        for (i=0; i<n; ++i) {
            data[i] = CREAL(self->fastsum->f[i]) + (self->diagonal - 1.0)*CREAL(self->fastsum->alpha[i]);
        }
    }
    else if (self->kernel == 2) { // xx_gaussian kernel
        for (i=0; i<n; ++i) {
            data[i] = CREAL(self->fastsum->f[i]) + (self->diagonal)*CREAL(self->fastsum->alpha[i]);
        }
    }
    else if (self->kernel == 3) { // laplacian_rbf kernel
        for (i=0; i<n; ++i) {
            data[i] = CREAL(self->fastsum->f[i]) + (self->diagonal - 1.0)*CREAL(self->fastsum->alpha[i]);
        }
    }
    else {
        for (i=0; i<n; ++i) {
        data[i] = CREAL(self->fastsum->f[i]) + (self->diagonal - 1.0)*CREAL(self->fastsum->alpha[i]);
    }
    }
    
    return (PyObject*) array;
}

#ifdef BUILD_EIGS
static PyObject *
AdjacencyCore_normalized_eigs(AdjacencyCoreObject* self, PyObject* args, PyObject* keywds) {

    int i, j;
    npy_intp vec_dims[2];
    PyObject* result, * eigenvalues, * eigenvectors;
    double *data;
    static char *kwlist[] = {"nev", "tol", "maxiter", "ncv", "return_eigenvectors", NULL};

    if (!check_fastsum(self))
        return NULL;

    int n = self->n;    // dimension
    int nev = 6;        // number of eigenvalues
    int ncv = 0;        // krylov subspace dimension, default: min(n, max(2*k+1, 20))
    int maxiter = 0;    // maximum number of iterations
    double tol = 0.0;   // tolerance
    int rvecs = 1;      // flag for eigenvector computation
    
    if (!n) {
        PyErr_SetString(PyExc_RuntimeError, "AdjacencyCore.points must be given before calling AdjacencyCore.eigs");
        return NULL;
    }
    
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|diip", kwlist, &nev, &tol, &maxiter, &ncv, &rvecs)) {
        PyErr_Format(PyExc_TypeError, "Invalid input to AdjacencyCore.eigs");
        return NULL;
    }
    
    if (ncv <= 0) {
        if (nev < 10)
            ncv = 20;
        else if (2*nev >= n)
            ncv = n;
        else
            ncv = 2*nev + 1;
    }
    
    if (maxiter <= 0)
        maxiter = 300; // this is the default from matlab; in scipy, it is n*10
    
    
    // Compute degrees
    double* d_invsqrt = (double*) malloc(n*sizeof(double));
    for (i=0; i<n; ++i) {
        self->fastsum->alpha[i] = CMPLX(1.0, 0.0);
    }
    fastsum_trafo(self->fastsum);
    
//     for (i=0; i<n; ++i) {
//         d_invsqrt[i] = 1.0 / sqrt(CREAL(self->fastsum->f[i]) + self->diagonal - 1.0);
//     }
    
    if (self->kernel == 1) { // gaussian kernel
        for (i=0; i<n; ++i) {
            d_invsqrt[i] = 1.0 / sqrt(CREAL(self->fastsum->f[i]) + self->diagonal - 1.0);
        }
    }
    else if (self->kernel == 2) { // xx_gaussian kernel
        for (i=0; i<n; ++i) {
            d_invsqrt[i] = 1.0 / sqrt(CREAL(self->fastsum->f[i]) + self->diagonal);
        }
    }
    else if (self->kernel == 3) { // laplacian_rbf kernel
        for (i=0; i<n; ++i) {
            d_invsqrt[i] = 1.0 / sqrt(CREAL(self->fastsum->f[i]) + self->diagonal - 1.0);
        }
    }
    else {
        for (i=0; i<n; ++i) {
            d_invsqrt[i] = 1.0 / sqrt(CREAL(self->fastsum->f[i]) + self->diagonal - 1.0);
        }
    }
        
    // Additional inputs for ARPACK
    
    int ido = 0;    // reverse communication flag.
    int lworkl = ncv*(ncv+8);   // size of array needed internally
    int info = 0;   // error flag
    
    double* resid = (double*) malloc(n*sizeof(double));
    double* v = (double*) malloc(n*ncv*sizeof(double));
    double* workd = (double*) malloc(3*n*sizeof(double));
    double* workl = (double*) malloc(lworkl*sizeof(double));
    double* d = (double*) malloc(nev*sizeof(double));
    
    int iparam[11] = {1,0,maxiter,1,0,0,1,0,0,0,0};
    int ipntr[11] = {0};
    int *select = (int*) malloc(ncv*sizeof(int));
    
    while (1) {
    
        dsaupd_c(&ido, "I", n, "LM", nev, tol, resid, ncv, v, n, iparam, ipntr, workd, workl, lworkl, &info);
    
        if (ido == 1 || ido == -1) {
            
            // Compute matrix vector product
            
            for (i=0; i<n; ++i) {
                self->fastsum->alpha[i] = CMPLX(d_invsqrt[i] * workd[ipntr[0] + i], 0);
            }
            
            fastsum_trafo(self->fastsum);
            
//             for (i=0; i<n; ++i) {
//                     workd[ipntr[1] + i] = workd[ipntr[0] + i] + d_invsqrt[i] * 
//                                     (CREAL(self->fastsum->f[i]) + (self->diagonal - 1.0)*CREAL(self->fastsum->alpha[i]));
//                 }
            
            if (self->kernel == 1) { // gaussian kernel
                for (i=0; i<n; ++i) {
                    workd[ipntr[1] + i] = workd[ipntr[0] + i] + d_invsqrt[i] * 
                                    (CREAL(self->fastsum->f[i]) + (self->diagonal - 1.0)*CREAL(self->fastsum->alpha[i]));
                }
            }
            else if (self->kernel == 2) { // xx_gaussian kernel
                for (i=0; i<n; ++i) {
                    workd[ipntr[1] + i] = workd[ipntr[0] + i] + d_invsqrt[i] * 
                                    (CREAL(self->fastsum->f[i]) + (self->diagonal)*CREAL(self->fastsum->alpha[i]));
                }
            }
            else if (self->kernel == 3) { // laplacian_rbf kernel
                for (i=0; i<n; ++i) {
                    workd[ipntr[1] + i] = workd[ipntr[0] + i] + d_invsqrt[i] * 
                                    (CREAL(self->fastsum->f[i]) + (self->diagonal - 1.0)*CREAL(self->fastsum->alpha[i]));
                }
            }
            else {
                for (i=0; i<n; ++i) {
                    workd[ipntr[1] + i] = workd[ipntr[0] + i] + d_invsqrt[i] * 
                                    (CREAL(self->fastsum->f[i]) + (self->diagonal - 1.0)*CREAL(self->fastsum->alpha[i]));
                }
            }
        }
        else break;
    }
    
    printf("dsaupd terminated after %d iterations, %d mat-vec products, and %d re-orthogonalizations\n", iparam[2], iparam[8], iparam[10]);
    
    if (info < 0) {
        PyErr_Format(PyExc_RuntimeError, "ARPACK 'dsaupd' failed with error code %d", info);
        result = NULL;
    }
    else {
    
        dseupd_c(rvecs, "A", select, d, v, n, 0.0, "I", n, "LM", nev, tol, resid, ncv, v, n, iparam, ipntr, workd, workl, lworkl, &info);
        
        if (info < 0) {
            PyErr_Format(PyExc_RuntimeError, "ARPACK 'dseupd' failed with error code %d", info);
            result = NULL;
        }
        else {
            // Build eigenvalue object
            vec_dims[0] = nev;
            eigenvalues = PyArray_SimpleNew(1, vec_dims, NPY_DOUBLE);
            PyArray_ENABLEFLAGS((PyArrayObject*) eigenvalues, NPY_OWNDATA);
    
            data = (double*) PyArray_DATA((PyArrayObject*) eigenvalues);
            for (j=0; j<nev; ++j) {
                data[j] = d[j] - 1.0;
            }
            
            if (rvecs) {
                // Build eigenvector object
                vec_dims[0] = n;
                vec_dims[1] = nev;
                eigenvectors = PyArray_SimpleNew(2, vec_dims, NPY_DOUBLE);
                PyArray_ENABLEFLAGS((PyArrayObject*) eigenvectors, NPY_OWNDATA);
                
                data = (double*) PyArray_DATA((PyArrayObject*) eigenvectors);
                for (i=0; i<n; ++i) {
                    for (j=0; j<nev; ++j) {
                        data[i*nev + j] = v[j*n + i];
                    }
                }
                
                result = Py_BuildValue("OO", eigenvalues, eigenvectors);
                Py_DECREF(eigenvalues);
                Py_DECREF(eigenvectors);
            }
            else {
                result = eigenvalues;
            }
        }
        
    }
    
    free(d_invsqrt);
    free(resid);
    free(v);
    free(workd);
    free(workl);
    free(d);
    free(select);
    
    return result;
}
#endif


static PyMemberDef AdjacencyCore_members[] = {
    {"kernel", T_INT, offsetof(AdjacencyCoreObject, kernel), READONLY, "Kernel function"},
    {"d", T_INT, offsetof(AdjacencyCoreObject, d), READONLY, "Spatial dimension"},
    {"sigma", T_DOUBLE, offsetof(AdjacencyCoreObject, sigma), READONLY, "Sigma for kernel"},
    {"N", T_INT, offsetof(AdjacencyCoreObject, N), READONLY, "Expansion degree (n in NFFT)"},
    {"p", T_INT, offsetof(AdjacencyCoreObject, p), READONLY, "Smoothness parameter"},
    {"m", T_INT, offsetof(AdjacencyCoreObject, m), READONLY, "Window cutoff parameter"},
    {"eps", T_DOUBLE, offsetof(AdjacencyCoreObject, eps), READONLY, "Outer boundary width"},
    {"NN", T_INT, offsetof(AdjacencyCoreObject, NN), READONLY, "Oversampling expansion degree (default: a power of two with 2*N <= NN < 4*N)"},
    {"diagonal", T_DOUBLE, offsetof(AdjacencyCoreObject, diagonal), 0, "Value on the diagonal of the adjacency matrix"},
    {"n", T_INT, offsetof(AdjacencyCoreObject, n), READONLY, "Number of points given"},
    {NULL}
};

static PyMethodDef AdjacencyCore_methods[] = {
    {"apply", (PyCFunction) AdjacencyCore_apply, METH_VARARGS | METH_KEYWORDS, "Approximate a matrix-vector product with the adjacency matrix"},
#ifdef BUILD_EIGS
    {"normalized_eigs", (PyCFunction) AdjacencyCore_normalized_eigs, METH_VARARGS | METH_KEYWORDS, "Approximate a few eigenvalues of the symmetrically normalized adjacency matrix"},
#endif
    {NULL}
};

static PyGetSetDef AdjacencyCore_getsetters[] = {
    {"points", (getter) AdjacencyCore_getpoints, (setter) AdjacencyCore_setpoints, "Numpy array of 3D points", NULL},
    {NULL}
};

static PyTypeObject AdjacencyCoreType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "fastadj.core.AdjacencyCore",
    .tp_doc = "FastAdjacency AdjacencyCore object",
    .tp_basicsize = sizeof(AdjacencyCoreObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) AdjacencyCore_init,
    .tp_dealloc = (destructor) AdjacencyCore_dealloc,
    .tp_members = AdjacencyCore_members,
    .tp_methods = AdjacencyCore_methods,
    .tp_getset = AdjacencyCore_getsetters,
};

static PyModuleDef fastadjcoremodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "fastadj.core",
    .m_doc = "Fast multiplication with Gaussian adjacency matrices using NFFT/Fastsum",
    .m_size = -1,
};


PyMODINIT_FUNC
PyInit_core(void)
{
    PyObject *m;
    
    import_array();
    
    if (PyType_Ready(&AdjacencyCoreType) < 0)
        return NULL;

    m = PyModule_Create(&fastadjcoremodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&AdjacencyCoreType);
    if (PyModule_AddObject(m, "AdjacencyCore", (PyObject *) &AdjacencyCoreType) < 0) {
        Py_DECREF(&AdjacencyCoreType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
