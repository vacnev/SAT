#pragma once

#include "solver_types.hpp"

void ahoj();

struct solver {
    formula form;
    std::map< var_t, lbool > asgn;
    
    std::vector< lit_t > trail;
    std::vector< int > decisions; // indices into trail
    std::vector< int > reasons; // indices into form, -1 if dec
    std::map< lit_t, std::vector< int > > occurs;

    solver(formula _form) : form(std::move(_form)) {}

    void decide( var_t x, bool v ) {
        assign(x, v);
        decisions.push_back(trail.back());
        reasons.push_back(-1);
    }

    void assign( var_t x, bool v ) {
        asgn[x] = v;
        lit_t l = ( v ) ? x : -x;
        trail.push_back(l);
    }

    void unit_propagation() {
        int index = trail.size() - 1;

        while ( index < trail.size() ) {
            lit_t lit = trail[index];

            for ( int i : occurs.extract(-lit).mapped() ) {
                clause& c = form[i];
                c.resolve_watched(i, -lit, asgn, occurs);
                if ( c.status == clause::UNIT ) {
                    lit_t l = c.watched_lits().first; 
                    assign( std::abs(l), l > 0 );
                    // c.status = clause::SATISFIED; // is needed?
                    reasons.push_back(i);
                }
            }
        }
    }
};