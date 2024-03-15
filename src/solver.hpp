#pragma once

#include "solver_types.hpp"

void ahoj();

struct solver {
    formula form;
    // std::map< var_t, lbool > asgn;
    assignment asgn;
    
    std::vector< lit_t > trail;
    std::vector< int > decisions; // indices into trail
    std::vector< int > reasons; // indices into form, -1 if dec
    std::map< lit_t, std::vector< int > > occurs;

    solver(formula _form) : form(std::move(_form)), asgn(form.var_count) {}

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

    bool unit_propagation() {
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
                } else if ( c.status == clause::CONFLICT ) {
                    return false; // UNSAT
                }
            }
        }

        return true;
    }

    void add_base_clause(clause c) {
        form.add_base_clause(std::move(c));
    }

    void add_learnt_clause(clause c) {
        form.add_learnt_clause(std::move(c));
    }

    // DPLL
    void backtrack() {
        for ( std::size_t i = decisions.back() + 1; i < trail.size(); ++i ) {
            asgn.unassign( std::abs( trail[i] ) );
        }

        trail.resize( decisions.back() + 1 );
        decisions.pop_back();
        trail.back() *= -1;
        var_t var = std::abs(trail.back());
        asgn[var] = 1 - *asgn[var];
    }

    bool solve() {
        // if ( !unit_propagation() ) { return false; } // tohle asi nechci ?? --- no asi chci queue z unit klauzuli z initu

        while ( var_t var = asgn.get_unassigned() ) {

            decide(var, false);
            if ( !unit_propagation() ) {
                if ( decisions.empty() ) {
                    return false;
                }

                backtrack();
            }
        }

        return true;
    }

    
};
