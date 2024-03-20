#pragma once

#include "solver_types.hpp"
#include <fstream>

/* mozne optim 
 * - wrapper na trail - drzel by current size, nebyl by potreba resize v backtrack 
 */

struct solver {

    // solved formula
    formula form;

    /**
     * SOLVER STATE
     */
    assignment asgn;

    /**
     * signals that solver is in an unsatisfiable state before the first unit
     * propagation, (contradictory unit clauses / empty clause)
     **/
    bool unsat = false;

    // stores the trail (assigned literals)
    std::vector< lit_t > trail;

    /**
     * stores index of first literal in _trail_ to be investigated in
     * unit_propagate(), i.e. the head of the queue of assignments
     **/
    std::size_t index;

    /**
     * stores indices into _trail_ corresponding to decisions made during
     * search that can be backtracked to 
     */
    std::vector< int > decisions; 

    /**
     * stores reason (clause) for each literal in _trail_ 
     */
    std::vector< int > reasons;

    /* *
     * tracks watched literals accros clauses, maps literals -> indices of
     * clauses in which they are currently watched
     */
    std::unordered_map< int, std::vector< int > > occurs;



    /**
     * CONSTRUCTORS
     */
    solver(formula _form) : form(std::move(_form)), asgn(form.var_count) {
        initialize_structures();
    }



    /**
     * INIT FUNCTIONS
     */

    // initialize _occurs_, check empty / unit clauses before solve()
    void initialize_clause( const clause& cl, int clref );
    void initialize_structures();

    void add_base_clause(clause c);
    void add_learnt_clause(clause c);

    /**
     * MODEL OUTPUT/TESTING functions
     */
    std::vector< bool > get_model();
    std::string get_model_string();
    void output_model( const std::string &filename );



    /**
     * CORE
     */

    // assigns val v to variable x, adds new decision level to _decisions_
    void decide( var_t x, bool v );

    // assigns, but without the new DL
    void assign( var_t x, bool v );

    /**
     * processes all currently enqueued assignments in trail, starting 
     * from the _index_ entry
     */
    bool unit_propagation();

    /**
     * backtracks to the previous DL, flipping the last decision,
     * as well as nullifying the resulting unit propagations,
     */
    void backtrack();

    /*
     * solves the formula _form_, returning true if it is SAT
     */
    bool solve();
};
