#include "solver_types.hpp"
#include "logger.hpp"
#include <fstream>
#include <random>
#include <unordered_set>

struct solver {

    // rng with fixed seed
    std::mt19937 rng{ 42 };

    logger log;

    // solved formula
    formula form;


    // index to confl clause
    int confl_clause;

    /*
     * a vector of watched literals in the same order as the clauses in the formula
     */
    std::vector< std::pair< lit_t, lit_t > > watches;

    /**
     * SOLVER STATE
     */
    assignment asgn;

    /**
     * signals that solver is in an unsatisfiable state before the first unit
     * propagation, (contradictory unit clauses / empty clause)
     **/
    bool unsat = false;


    /**
     * stores index of first literal in _trail_ to be investigated in
     * unit_propagate(), i.e. the head of the queue of assignments
     **/
    std::size_t index;

    /*
     * current index of conflict
     */
    int conflict_idx = -1;

    /**
     * stores indices into _trail_ corresponding to decisions made during
     * search that can be backtracked to 
     */
    std::vector< int > decisions; 

    /*
     * stores the trail (assigned literals)
     */
    std::vector< lit_t > trail;

    /* *
     * tracks watched literals accros clauses, maps literals -> indices of
     * clauses in which they are currently watched
     */
    lit_map occurs;

    /**
     * seen literals, used for resolution in CDCL
    */
    std::vector< int > seen;

    /**
     * stores reason (clause) for each literal in _trail_ 
     */
    std::vector< int > reasons;

    /**
     * levels of variables, used for CDCL
     */
    std::vector< int > levels;

    /**
     * VARIABLE SELECTION
     */

    /**
     * stores the EVSIDS max heap structure containing all variables 
     */
    evsids_heap heap;

    // increment for evsids
    double inc = 1;

    // decay factor used to multiply the increment
    double var_decay = 1.01;

    void decay_var_priority();
    void increase_var_priority( var_t v );

    // select next branching variable
    var_t get_unassigned( bool &polarity );



    /* RESTARTS */

    /* number of conflicts */
    int conflicts = 0;

    /* restart limit */
    int restart_limit = 100;

    /* compute next limit with Luby sequence */
    int max_limit = 100;

    void change_restart_limit() {
        restart_limit *= 2;

        if (restart_limit > max_limit) {
            max_limit *= 2;
            restart_limit = 100;
        }
    }

    /* restart */
    void restart();

    /* FORGETTING CLAUSES */

    /* local forgetting period */
    int forget_period = 15000;

    /* mid demotion check period */
    int demote_period = 10000;

    /* conflict ctr (max. 30k) */
    int conflict_ctr = 1;

    void inc_conflict_ctr() {
        if (++conflict_ctr > 30000) {
            conflict_ctr = 1;
        }
        ++conflicts;

        if ( conflicts % demote_period == 0 ) {
            form.learnt.demote_clauses( conflict_ctr, demote_period, reasons );
        } else if ( conflict_ctr % forget_period == 0 ) {
            form.learnt.forget_clauses();
        }
    }

    /**
     * CONSTRUCTORS
     */
    solver(formula _form) : form(std::move(_form))
                          , watches( form.clause_count )
                          , asgn(form.var_count)
                          , heap( form.var_count )
                          , occurs( form.var_count )
                          , seen( form.var_count + 1 )
                          , levels( form.var_count + 1 ) 
    {
        initialize_structures();
    }



    /**
     * INIT FUNCTIONS
     */

    // initialize _occurs_, check empty / unit clauses before solve()
    void initialize_clause( clause& cl, int clref );
    void initialize_structures();

    void add_base_clause(clause c);
    void add_learnt_clause(clause c);

    /**
     * MODEL OUTPUT/TESTING functions
     */
    std::vector< bool > get_model();
    std::string get_model_string();
    void output_model( const std::string &filename );

    void log_solver_state( const std::string &title, bool all_clauses );
    void log_clause( const clause &c, const std::string &title );



    /**
     * CORE
     */

    /* current decision level */
    int current_level() const {
        return decisions.size();
    }

    /* choose random polarity */
    bool rand_pol() {
        return rng() % 2;
    }

    int compute_lbd( const std::vector< lit_t >& lits );

    // assigns val v to variable x, adds new decision level to _decisions_
    void decide( var_t x, bool v );

    // assigns, but without the new DL
    void assign( var_t x, bool v );

    // unassigns variable, inserting back into evsids heap and updating asgn struct
    void unassign( var_t x );
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

    /**
     * performs conflict analysis, returning a new learnt clause
     * and backjump index
     */
    std::pair< clause, int > analyze_conflict();

    /**
     * backjumps to the level of the last UIP
    */
    void backjump( int level, clause learnt );

    /*
     * solves the formula _form_, returning true if it is SAT
     */
    bool solve();
};
