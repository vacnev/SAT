#include "solver_types.hpp"
#include "logger.hpp"
#include <fstream>

struct solver {

    logger log;

    // solved formula
    formula form;

    /**
     * SOLVER STATE
     */
    assignment asgn;

    // index to confl clause
    int confl_clause;

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

    
    // current index of conflict
    int conflict_idx = -1;

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
     * seen literals, used for resolution in CDCL
    */
    std::unordered_map< var_t, int > seen;

    /**
     * levels of variables, used for CDCL
     */
    std::unordered_map< var_t, int > levels;

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

    void log_solver_state( const std::string &title );
    void log_clause( const clause &c, const std::string &title );



    /**
     * CORE
     */

    /* current decision level */
    int current_level() const {
        return decisions.size();
    }

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
