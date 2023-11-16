from distutils.core import setup, Extension
import os
import numpy as np


# read nfft base dir 
nfft_base = os.environ.get('NFFT_BASE')
assert nfft_base is not None, 'Environment variable NFFT_BASE must be exported before setup'

# set include and library directories
include_dirs = [os.path.join(nfft_base, 'include'), 
                os.path.join(nfft_base, 'applications', 'fastsum'),
                os.path.join(np.__path__[0], 'core', 'include')]

library_dirs = [os.path.join(nfft_base, 'julia', 'fastsum')]

macros = [('MAJOR_VERSION', '0'), ('MINOR_VERSION', '2')]

# C extension fastadj2.core
core_ext = Extension('prescaledfastadj.core',
    define_macros = macros,
    include_dirs = include_dirs,
    libraries = ['fastsumjulia'],
	library_dirs = library_dirs,
    runtime_library_dirs = library_dirs,
    sources = ['prescaledfastadj/core.c'])

# run setup
setup(name = 'prescaledfastadj',
    version = '0.2',
    description = 'Prescaled FastAdjacency Package',
    author = 'Theresa Wagner',
    author_email = 'theresa.wagner@math.tu-chemnitz.de',
    url = 'https://github.com/wagnertheresa/prescaledFastAdj',
    packages = ['prescaledfastadj'],
    py_modules = [],
    ext_modules = [core_ext])
