#include <iostream>

#include "solver.hpp"
#include "parser.hpp"

int main(){
    std::cout << "Hello world!\n";

    formula f = parse_dimacs("../test/ezpz.cnf");
    solver s( std::move( f ) );

    std::string res = s.solve() ? "SAT" : "UNSAT";
    std::cout << res << "\n";

    formula f2 = parse_dimacs("../test/all_satisfiable/uf20-0478.cnf");
    solver s2( std::move( f2 ) );

    std::string res2 = s2.solve() ? "SAT" : "UNSAT";
    std::cout << res2 << "\n";

    return 0;
}
