#pragma once

#include "solver_types.hpp"

struct solver {

    formula form;
    // std::map< var_t, lbool > asgn;
    assignment asgn;


    /*
     * solver is in unsat state already,
     * used when adding clauses in the beginning
     */

    bool unsat_constr;
    
    // index into trail, head of queue to propagate
    std::size_t index;

    std::vector< lit_t > trail;

    std::vector< int > decisions; // indices into trail
    std::vector< int > reasons; // indices into form, -1 if dec

    std::map< lit_t, std::vector< int > > occurs;

    solver(formula _form) : form(std::move(_form)), asgn(form.var_count) {
        initialize_structures();
    }

    /* 
     * set up state of the solver after loading formula 
     * clref is the index of cl in form, used to setup watched lits
     */
    void initialize_clause( const clause& cl, int clref ) {
        auto [l1, l2] = cl.watched_lits();

        occurs[l1].push_back( clref );

        // unit clause, setup trail for first UP
        if ( cl.status == clause::UNIT ) {
            trail.push_back( l1 );
        }

        else {
            occurs[l2].push_back( clref );
        }

    }
    
    // init occurs
    void initialize_structures() {
        for ( std::size_t i = 0; i < form.clause_count; i++ ){
            initialize_clause( form[i], i );
        }

        index = 0;
    }


    // assigns + sets starting idx of new decision level
    void decide( var_t x, bool v ) {
        assign(x, v);
        decisions.push_back(trail.size() - 1);
        reasons.push_back(-1);
    }

    // sets x to value, adds the literal to trail
    void assign( var_t x, bool v ) {
        asgn[x] = v;
        lit_t l = ( v ) ? x : -x;
        trail.push_back(l);
    }

    /* unit propagates the literals enqueued in trail
     * until index == trail.size() i.e. all literals 
     * have been propagated
     */
    bool unit_propagation() {

        while ( index < trail.size() ) {

            lit_t lit = trail[index++];
            for ( auto x : trail ) {
                std::cout << x << ", ";
            }

            std::cout << "\n\n";

            auto clause_indices = occurs.extract(-lit).mapped();

            // TODO: unlinking occurs[-lit] causes issues for some reason
            // adding this ad hoc solution for now 
            occurs[-lit] = {};

            for ( int i : clause_indices ) {
                clause& c = form[i];
                c.resolve_watched(i, -lit, asgn, occurs);

                // if c was made UNIT by prev UP, add the literal to trail 
                // and update reasons
                if ( c.status == clause::UNIT ) {
                    lit_t l = c.watched_lits().first; 
                    assign( std::abs(l), l > 0 );
                    reasons.push_back( i );
                } 

                // Conflict in UP -> some clause has -lit as the last literal
                else if ( c.status == clause::CONFLICT ) {
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

        // decisions.back() contains idx to trail of last decision
        // adjust trail accordingly
        trail.resize( decisions.back() + 1 );
        decisions.pop_back();

        // set literal to negation, fix assignment
        trail.back() *= -1;
        var_t var = std::abs(trail.back());
        asgn[var] = 1 - *asgn[var];

        // set head of propagation queue to last
        index = trail.size() - 1;
    }

    bool solve() {

        // first UP
        if ( !unit_propagation() ) { return false; } 

        while ( var_t var = asgn.get_unassigned() ) {

            decide(var, false);
            if ( !unit_propagation() ) {
                if ( decisions.empty() ) {
                    return false;
                }

                backtrack();
            }
        }

        std::cout << "Model:\n";
        for ( std::size_t i = 1; i < asgn.asgn.size(); i++ ) {
            std::cout << i << " : " << asgn.asgn[i].value() << "\n";
        }

        return true;
    }

    
};
