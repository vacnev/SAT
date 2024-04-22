#include <iostream>

#include "solver.hpp"
#include "parser.hpp"

int main( int argc, char *argv[] ){

    if ( argc == 1 ) {
        std::cout << "UNKNOWN\n";
        return 0;
    }

    std::string res;

    for ( int i = 1; i < argc; ++i ) {
        
        formula f = parse_dimacs( argv[i] );
        solver s = solver( std::move( f ) );

        bool satisfiable = s.solve();
        res = satisfiable ? "s SATISFIABLE" : "s UNSATISFIABLE";
        std::cout << res << "\n";

        if ( satisfiable )
            std::cout << s.get_model_string();
            s.output_model( "../test/model_file.txt" );
    }

    return 0;
}
