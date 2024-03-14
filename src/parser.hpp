#pragma once
#include "solver_types.hpp"
#include <fstream>
#include <stdexcept>
#include <string>


/*
 *
 * dimacs format:
 *
 * begins with n comment lines starting with c ... 
 * config line : p cnf 3 ( n vars ) 4 ( n clauses )
 * each clause is list of nums terminated by 0
 *
 */

void skip_ws( const std::string &line, size_t &pos ){
    while ( pos < line.length() && std::isspace( line[pos] ) ) { pos++; }
}

int parse_int( const std::string &line, size_t &pos ) {
    std::string int_str;
    size_t i = pos;

    if ( line[i++] == '-' ) { int_str.push_back( 'i' ); }
    while ( i < line.length() && std::isdigit( line[i] ) ) {
        int_str.push_back( line[i] );
        i++;
    }

    pos = i;
    return std::stoi( int_str );
}

bool ignore_line( const std::string &line, size_t& pos ) {
   skip_ws( line, pos );
   return pos == line.length();
}

formula parse_dimacs( const std::string &filename ) {

    std::ifstream input( filename );
    std::string line;
    std::string word;

    std::getline( input, line );

    while ( line.empty() || line[0] == 'c' ) { std::getline( input, line ); }

    if ( line.length() < 5 || line.substr(0, 5) != "p cnf" ) {
        throw std::runtime_error( "parser error, expected p cnf line" );
    }

    size_t pos = 6;
    skip_ws( line, pos );
    
    // assuming config line has correct format for now
    int num_vars = parse_int( line, pos );
    skip_ws( line, pos );
    int num_clauses = parse_int( line, pos );

    formula f;
    f.base.reserve( num_clauses );
    f.base.push_back( clause() );

    // current clause parsed
    size_t curr = 0;

    while ( std::getline( input, line ) ) {

        size_t pos = 0; 
        if ( ignore_line( line, pos ) ) { continue; }

        while ( pos != line.size() ) {
            int lit = parse_int( line, pos );

            if ( lit != 0 ) { f.base[curr].data.push_back( lit ); }
            else { curr++;
                   f.base.push_back( clause() );
            }

            skip_ws( line, pos );
        }
    }


    return f;
}



