#include <iostream>

#include "solver.hpp"
#include "parser.hpp"

int main( int argc, char *argv[] ){

    if ( argc == 1 ) {
        std::cout << "UNKNOWN\n";
        return 0;
    }

    std::string res;
    std::string model_output = "../test/model_file.txt";

    for ( int i = 1; i < argc; ++i ) {
        formula f = parse_dimacs( argv[i] );
        solver s = solver( std::move( f ) );
        res = s.solve() ? "SAT" : "UNSAT";
        std::cout << res << "\n";

        s.output_model( model_output );
    }

    return 0;
}
