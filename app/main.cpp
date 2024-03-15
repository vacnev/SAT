#include <iostream>

#include "solver.hpp"
#include "parser.hpp"

int main(){
    std::cout << "Hello world!\n";
    ahoj();


    formula f = parse_dimacs("../test/all_satisfiable/uf20-0100.cnf");
    solver s( std::move( f ) );

    std::string res = s.solve() ? "SAT" : "UNSAT";
    std::cout << res << "\n";

    return 0;
}
