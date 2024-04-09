#include "solver.hpp"
#include <cassert>

void solver::initialize_clause( const clause& cl, int clref ) {
    auto [l1, l2] = cl.watched_lits();

    // unit clause, setup trail for first UP
    if ( cl.status == clause::UNIT ) {
        if ( !asgn.lit_unassigned(l2) && !asgn.satisfies_literal(l2) ) {
            unsat = true;
            return;
        }

        assign( l2.var(), l2.pol() );
        reasons.push_back( clref );
    }

    // init occurs vecs
    occurs[l1].push_back( clref );
    occurs[l2].push_back( clref );
}

void solver::initialize_structures() {
    for ( std::size_t i = 0; i < form.clause_count; i++ ){
        initialize_clause( form[i], i );

        if ( unsat ) {
            return;
        }
    }

    index = 0;
}

std::vector< bool > solver::get_model() {
    std::vector< bool > res( asgn.vars_count );

    for ( int var = 1; var <= asgn.vars_count; ++var ) {
        res[var-1] = asgn.satisfies_literal( var );
    }
    return res;
}

std::string solver::get_model_string() {
    auto model = get_model();
    std::string model_str;
    for ( int i = 1; i <= model.size(); ++i ){
        model_str += std::to_string( i ) + " : " + std::to_string( model[i-1] ) + "\n";
        
    }
    return model_str;        
}

void solver::output_model( const std::string &filename ) {
    std::string str = get_model_string();
    std::ofstream out( filename, std::ios::out );
    out << str;
}

void solver::log_clause( const clause& c, const std::string &title ) {
    if ( !log.enabled() ) return;
    
    log.log() << "   " << title << " clause - {";
    for ( lit_t x : c.data ) {
        log.log() << x << ", "; 
    }

    log.log() << "}";
    auto x = c.watched_lits();
    log.log() << "Watches - " << x.first << ", " << x.second << "\n";

}

void solver::log_solver_state( const std::string &title ) {
    if ( !log.enabled() ) return;


    /*
     * bad idea
    for ( int i = 0; i < form.clause_count; i++ ) {
        log_clause( form[i] , "Clause " + std::to_string(i) );
    }
    */

    log.log() << title << "\n";
    log.log() << "\n\nEVSIDS:\n";
    for ( auto x : heap.heap ) {
        log.log() << x << " - " << heap.priorities[x] << "\n";
    }

    
    for ( auto &[k, v] : heap.indices ) {
        log.log() << "Var " << k << " has index " << v << "\n";
    }

    log.log() << "STATE:\n\n\n";

    log.log() << "index: " << index << "\n";
    log.log() << "TRAIL:\n";
    log.log() << "[ ";
    for ( auto x : trail ) { log.log() << x << ", ";}
    log.log() << " ]\n\n";

    log.log() << "ASGN:\n";
    log.log() << "[ ";
    for ( int i = 1; i < asgn.asgn.size(); i++ ) { 
        if ( asgn.var_unassigned( i ) ) { log.log() << " none ; "; }
        else { log.log() << asgn.satisfies_literal( i ) << " ; "; }
    }
    log.log() << " ]\n\n";

    log.log() << "DECISIONS:\n";
    log.log() << "[ ";;
    for ( auto x : decisions ) { log.log() << x << ", "; }
    log.log() << " ]\n\n";

    log.log() << "LEVELS:\n";
    log.log() << "[ ";
    for ( const auto &[k, v] : levels ) { log.log() << k << " - " << v << "; "; }
    log.log() << " ]\n\n";

    log.log() << "REASONS:\n";
    log.log() << "[ ";
    for ( auto x : reasons ) { log.log() << x << ", "; }
    log.log() << " ]\n\n";

    // log.log() << "OCCURS:\n";
    // for ( const auto &[k, v] : occurs ) {
    //     log.log() << k << " - [ ";
    //     for ( auto x : v ) { log_clause(form[x], "Occurs" + std::to_string(k)); }
    //     log.log() << " ]\n";
    // }

    log.log() << "------------------------------------" << std::endl;
}

void solver::decide( var_t x, bool v ) {
    assign(x, v);
    levels[x]++;
    decisions.push_back(trail.size() - 1);
    reasons.push_back(-1);
}

void solver::assign( var_t x, bool v ) {
    asgn.assign( x, v );
    lit_t l = ( v ) ? x : -x;
    trail.push_back(l);
    levels[x] = decisions.size();
}

void solver::unassign( var_t x ){
    asgn.unassign( x );

    // potentially return x back to the evsids heap
    heap.insert( x );
}

void solver::decay_var_priority() {
    inc *= var_decay;
}

void solver::increase_var_priority( var_t v ) {
    heap.increase_priority( v, inc );
}

/* iff all assigned then 0 */
std::pair< var_t, bool > solver::get_unassigned() {
    var_t v_max = 0;

    // FIXME: surely a better way to avoid doing this in the heap somehow - spis ne
    do {
        v_max = heap.extract_max();
        log_solver_state("after extract");
    }
    while ( !asgn.var_unassigned( v_max ) );

    bool pol = false;
    lbool& saved = asgn.saved_phase(v_max);
    if ( saved ) {

        switch ( asgn.decision_count % 100 ) {
            case 0:
                pol = *saved;
                break;
            case 1:
                pol = !*saved;
                break;
            case 3:
                pol = rand_pol();
                break;
            default:
                break;
        }
    }

    saved = pol;

    if ( ++asgn.decision_count >= asgn.period ) {
        asgn.decision_count = 0;
    }

    return { v_max, pol };
}

bool solver::unit_propagation() {

    while ( index < trail.size() ) {

        lit_t lit = trail[index++];

        auto& clause_indices = occurs[-lit];
        auto og_size = clause_indices.size();

        // log.log() << "UP LIT: " << lit << '\n';

        for ( int curr_entry = 0; curr_entry < og_size; ++curr_entry ) {
            int i = clause_indices[curr_entry];
            clause& c = form[i];

            // log_clause(c, "UP CLAUSE ");

            clause::clause_status status = c.resolve_watched(i, -lit, asgn, occurs);

            if ( status == clause::UNIT ) {
                lit_t l = c.watched_lits().second; 
                assign( l.var(), l.pol() );
                reasons.push_back( i );

                // log.log() << "resovled lit: " << lit.var() << '\n';
                // log_clause(c, "UNIT PROPAGATION ");
                // log_solver_state("UNIT PROPAGATION ");
                
            } 
            // Conflict in UP -> some clause has -lit as the last literal
            else if ( status == clause::CONFLICT ) {

                // return clause indices that would be dropped
                clause_indices.erase(clause_indices.begin(), std::next(clause_indices.begin(), curr_entry + 1));

                // save index of conflict clause
                conflict_idx = i;

                return false; // UNSAT
            }
        }

        clause_indices.erase(clause_indices.begin(), std::next(clause_indices.begin(), og_size));
    }

    return true;
}

void solver::add_base_clause(clause c) {
    form.add_base_clause(std::move(c));
}

void solver::add_learnt_clause(clause c) {
    form.add_learnt_clause(std::move(c));
}

void solver::backtrack() {
    for ( std::size_t i = decisions.back() + 1; i < trail.size(); ++i ) {
        unassign( trail[i].var() );
    }

    // decisions.back() contains idx to trail of last decision
    // adjust trail accordingly
    trail.resize( decisions.back() + 1 );
    reasons.resize( decisions.back() + 1 );
    decisions.pop_back();

    // set literal to negation, fix assignment
    trail.back().flip();
    var_t var = trail.back().var();
    asgn[var] = 1 - *asgn[var];

    // set head of propagation queue to last
    index = trail.size() - 1;
}


/* invariant - learnt has two watched literals set, the second of which is the
 * UIP and the first one is the literal with second highest DL after UIP */
void solver::backjump( int level, clause learnt ) {

    assert( level < decisions.size() );

    // index of next decision level that is to be removed, i.e. all entries in
    // trail after decisions[level] will be deleted. In case the _learnt_
    // clause is unit, all decisions will be deleted;
    int next_level = ( level > 0 ) ? decisions[level] : decisions[0];

    for ( int k = next_level ; k < trail.size(); ++k ) {
        unassign( trail[k].var() );
    }

    // adjust trail accordingly
    decisions.resize( level );
    trail.resize( next_level );
    reasons.resize( next_level );

    // unit propagate learnt clause
    initialize_clause( learnt, form.size() );
    add_learnt_clause( std::move( learnt ) ) ;

    // set head of propagation queue to last
    index = trail.size() - 1;
}

std::pair< clause, int > solver::analyze_conflict() {

    std::vector< lit_t > learnt_clause{ 0 };
    int ind = trail.size() - 1;
    lit_t uip = 0;
    int lits_remaining = 0;
    std::vector< int > reasons_learnt;
    // clause confl = form[conflict_idx];
    int confl_idx = conflict_idx;

    log_solver_state("conflict analysis");
    // log_clause( confl, "conflict clause" );
    // log.log() << std::endl;

    do {

        // log.log() << "Index: " << ind << "\n";
        // log.log() << "lits:" << lits_remaining << std::endl;
        // log_clause( confl, "currently resolved clause" );
        // if (confl.learnt)
        //     log.log() << "LERNAT" << std::endl;
        // log.log() << std::endl;
        // log_solver_state("conflict analysis");
        
        for ( lit_t& l : form[confl_idx].data ) {

            if ( ( l != uip ) && levels[l.var()] > 0 && !seen[l.var()] ) {
                seen[l.var()] = 1;

                increase_var_priority( l.var() );

                if ( levels[l.var()] < current_level() ) {
                    learnt_clause.push_back( l );
                    reasons_learnt.push_back( reasons[ind] );
                }
                else {
                    // log.log() << "lit: " << l << " level: " << levels[l.var()] << " current level: " << current_level() << std::endl;
                    lits_remaining++;
                }
            }
        }

        // log_clause( clause( learnt_clause ), "ahoj" );

        // find next clause to resolve with
        while ( !seen[trail[ind].var()] ) { ind--; };
        


        uip = trail[ind];
        seen[uip.var()] = 0;
        lits_remaining--;

        // last step - this will be equal to -1
        if ( lits_remaining > 0) {
            
            // if ( reasons[ind] == -1 ) {
            //     log_solver_state("BAD SOLVER STATE");
            // }

            // confl = form[reasons[ind]];
            confl_idx = reasons[ind];
        }

    } while (lits_remaining > 0);

    learnt_clause[0] = -uip;
    auto to_clear = learnt_clause;

    // simplify learnt clause
    int i, j;
    for ( i = j = 1; i < learnt_clause.size(); ++i) {
        if ( reasons_learnt[i - 1] == -1) {
            learnt_clause[j++] = learnt_clause[i];
        } else {
            clause& confl = form[reasons_learnt[i - 1]];
            auto [l1, l2] = confl.watched_lits();

            for ( lit_t l : confl.data ) {
                if ( l != l2 && levels[l.var()] > 0 && !seen[l.var()] ) {
                    learnt_clause[j++] = learnt_clause[i];
                    break;
                }
            }
        }
    }

    learnt_clause.resize(j);
    learnt_clause.shrink_to_fit();

    // find backjump level
    int backjump_level = -1;
    if ( learnt_clause.size() > 1 ) {
        int max_i = 1;
        for ( int i = 2; i < learnt_clause.size(); ++i ) {
            if ( levels[learnt_clause[i].var()] > levels[learnt_clause[max_i].var()] ) {
                max_i = i;
            }
        }

        // swap highest DL literal
        std::swap( learnt_clause[1], learnt_clause[max_i] );
        backjump_level = levels[learnt_clause[1].var()];
    }

    // clear seen
    for ( const lit_t &l : to_clear ) {
        seen[l.var()] = 0;
    }

    // construct clause ( init watches to UIP & highest DL literal )
    clause learnt( std::move(learnt_clause), true );
    
    return { learnt, backjump_level };
}


bool solver::solve() {

    // log.set_log_level( log_level::TRACE );

    if ( unsat ) {
        return false;
    }

    // first UP
    if ( !unit_propagation() ) { return false; }

    var_t var;
    bool pol;

    while ( true ) {

        std::tie( var, pol ) = get_unassigned();

        if ( var == 0 ) {
            break;
        }

        decide(var, false);
        log_solver_state( "ahojky" );

        while ( !unit_propagation() ) {
            if ( decisions.empty() ) {
                return false;
            }
        
            assert( conflict_idx != -1 );
            auto [learnt, level] = analyze_conflict();
            decay_var_priority();

            if ( level == 0 ) {
                return false;
            }

            else if ( level == -1 ) {
                level = 0;
            }

            backjump(level, std::move( learnt ) );
            log_solver_state( "after_conflict" );
        }
    }

    log_solver_state( "final" );
    return true;
};
