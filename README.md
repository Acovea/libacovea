#ACOVEA
Analysis of Compiler Options via Evolutionary Algorithm 

version 4.0



Copyright 2003, 2004 Scott Robert Ladd. All rights reserved.

Contact information:    coyote@coyotegulch.com
                        http://www.coyotegulch.com
                        


ACOVEA (Analysis of Compiler Options via Evolutionary Algorithm)
implements a genetic algorithm to find the "best" options for
compiling programs with the GNU Compiler Collection (GCC) C and C++
compilers. "Best", in this context, is defined as those options that
produce the fastest executable program from a given source code.

Acovea is a C++ framework that can be extended to test other
programming languages and non-GCC compilers.

I envision Acovea as an optimization tool, similar in purpose to
profiling. Traditional function-level profiling identifies the
algorithms most influential in a program's performance; Acovea is then
applied to those algorithms to find the compiler flags and options
that generate the fastest code. Acovea is also useful for testing
combinations of flags for pessimistic interactions, and for examining
the reliability of the compiler.

BUILDING ACOVEA
---------------

Configuration and building proceeds along the usual lines:

	./configure
    ./make
    ./make install

To build Acovea, you will also need the Coyotl and Evocosm libraries:

	http://www.coyotegulch.com/libcoyotl
    http://www.coyotegulch.com/libevocosm
    
The installation creates a program, runacovea, installed by default
to /usr/local/bin. Running this program without arguments will display
a command summary.

For parsing the compiler configuration files, which are XML, Acovea
uses the expat library. Your distribution likely installed expat;
if not, its home page is:

	http://expat.sourceforge.net/

The installation directories can be changed using the --prefix option
when invoking configure.

RUNNING ACOVEA
--------------

To run Acovea, use a command of the form:

	runacovea -config gcc34_opteron.acovea -bench huffbench.c
    
CONFIGURATIONS
--------------
    
The configuration file is an XML document that sets the default and
evolvable options and commands for a given compiler and platform. For
example, the gcc34_opteron.acovea configuration is for GCC version
3.4 running on a 64-bit AMD Opteron system. 

A sample set of GCC C compiler configuration files is located (by
default) in:

	/usr/local/share/acovea/config
    
If the given configuration file can not be found based on the
explicit name given, Acovea will attempt to locate the file
in the installation directory above.

I provide configurations for Pentium 3, Pentium 4, and Opteron
processors using GCC 3.3 and 3.4. As I develop more configuration
files (or if I received them from other users), I'll post the
additional .acovea files on the web site.

BENCHMARKS
----------
    
The benchmark file is a program in a language appropriate for the
chosen configuration file; it must write its fitness value to 
standard output, where it will be read via pipe by the Acovea
framework.

A sample set of C benchmark programs is located (by default) in:

	/usr/local/share/acovea/benchmarks
    
If the given benchmark program file can not be found based on the
explicit name given, Acovea will attempt to locate the benchmark
in the installation directory above.

BENCHMARK DESIGN
----------------

Acovea was designed with "spikes" in mind -- short programs that
implement a limited algorithm that runs quickly (2-10 seconds per
run). Using the default evolutionary settings, Acovea will perform
4,000 compiles and runs of the benchmark; even with a very small
and fast benchmark, this can take several hours.

If your programs takes a full minute to compiler, and another 
minute to run, Acove will require 8,000 minutes, or more than
5.5 DAYS to evolve optimal option sets!
    
However, due to popular demand, a future version of Acovea will
support the use of Makefiles and an optional timing mechanism for
large applications.

The homepage for Acovea can be found at: http://www.coyotegulch.com/acovea
    

Other useful URLs:
- [Home Page](http://www.coyotegulch.com)
- [GNU Compiler Collection](http://gcc.gnu.org)
- [Intel C++ Compiler](http://www.intel.com/software/products/compilers)
- [Doxygen Documentation Tool](http://www.doxygen.org)
