# prescaledFastAdj

Extended version of the [FastAdjacency](https://github.com/dominikbuenger/FastAdjacency) package of Dominik Bünger.

### Python extension to compute fast approximate evaluations of the Gaussian kernel, its derivative kernel and the Matérn(1/2) kernel for our paper "Fast Evaluation of Additive Kernels: Feature Arrangement, Fourier Methods and Kernel Derivatives".

Different to the original [FastAdjacency](https://github.com/dominikbuenger/FastAdjacency), this package enables to evaluate the Gaussian derivative kernel matrix and the Matérn(1/2) kernel. Moreover, the data is prescaled before for a transparent and comparable global sensitivity analysis.

In this package the data scaling happens outside of the package for 

This package provides a Python interface to a part of the [NFFT3](https://github.com/NFFT/nfft) library to quickly approximate adjacency matrices of fully connected graphs with Gaussian edge weights.
See [test/showcase.ipynb](test/showcase.ipynb) for an overview of the method.

# Installation

* This software has been tested with Python 3.7.

* This software currently depends on the *Julia interface* of the NFFT3 library.
  - Download the NFFT3 source from [https://github.com/NFFT/nfft](https://github.com/NFFT/nfft).
  - When building the prerequesite FFTW3, make sure to call the configure script with `--enable-shared --enable-openmp --enable-threads`.
  - Configure, build and install NFFT3 following the instructions on the homepage. Make sure to configure the library with `--enable-julia`.

* Call `echo $LD_LIBRARY_PATH` to make sure that it contains `/usr/local/lib`, where the shared libraries of FFTW and NFFT are installed. If not, add the line `export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib` to your `~/.bashrc` file and run `source ~/.bashrc`.

* Navigate to the prescaledFastAdj folder.

* Export the path to your NFFT3 installation folder as the `NFFT_BASE` environment variable via `export NFFT_BASE=/path/to/your/NFFT/base/directory`. 

* From the same terminal, run `make` and `make install`. Optionally run `make check` to test your installation.

* To rebuild, run `make clean` before `make`.


# Usage

This package mainly consists of the following three classes:

* `AccuracySetup` stores setup parameters for the NFFT Fastsum algorithm. You can load one of three presets via `AccuracySetup(preset='name')`. Available preset names are `'default'`, `'fine'`, and `'rough'`.

* `AdjacencyCore` is the core type of fastadj, implemented in C. It serves as a wrapper for the NFFT `fastsum` type. Although it can be instantiated directly, we advise against it.

* `AdjacencyMatrix` is a convenient wrapper for `AdjacencyCore`, written in Python. This is the recommended way to use this package.

See [`test/showcase.ipynb`](test/showcase.ipynb) and [`test/test.py`](test/test.py) for an example.
