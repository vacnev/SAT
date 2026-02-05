# Fousaty

Fousaty is a lightweight SAT solver built for performance in C++.

# Current features:
	EVSIDS
	CDCL
	2 Watched literals
	PHASE SAVING
	RESTARTS

# Building the solver:

	$ cmake -S . -Bbuild -DCMAKE_BUILD_TYPE=Release
	$ cd build && make

# Running the solver:

To run the solver on a dimacs file, run:

	$ ./fousaty [path-to-dimacs]

