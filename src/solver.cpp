#include "solver.hpp"
#include <cassert>

void solver::initialize_clause( clause& cl, int clref ) {

    lit_t l1 = cl.data[0];
    lit_t l2 = cl.data[ ( cl.size() > 1 ) ];

    // add new entry to watches if the clause was learnt
    if ( cl.learnt ) {
        watches.push_back( { l1, l2 } );
    } else {
        watches[clref] = { l1, l2 };
    }

    // unit clause, setup trail for first UP
    if ( cl.status == clause::UNIT ) {
        if ( !asgn.lit_unassigned(l1) && !asgn.satisfies_literal(l1) ) {
            unsat = true;
            return;
        }

        assign( l1.var(), l1.pol() );
        reasons.push_back( clref );
        cl.reason_index = reasons.size() - 1;
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
    auto& x = watches[&c - &form[0]];
    log.log() << "Watches - " << x.first << ", " << x.second << "\n";

}

void solver::log_solver_state( const std::string &title, bool all_clauses=false ) {
    if ( !log.enabled() ) return;


    if ( all_clauses )
        for ( int i = 0; i < form.clause_count; i++ ) {
            log_clause( form[i] , "Clause " + std::to_string(i) );
        }

    log.log() << title << "\n";
    log.log() << "\n\nEVSIDS:\n";
    for ( auto x : heap.heap ) {
        log.log() << x << " - " << heap.priorities[x] << "\n";
    }

    log.log() << "INC:" << inc << "\n";

    
    int i = 0;
    for ( auto v : heap.indices ) {
        log.log() << "Var " << i++ << " has index " << v << "\n";
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
    for ( int i = 1; i < levels.size(); ++i ) { log.log() << i << " - " << levels[i] << "; "; }
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

void solver::restart() {

    change_restart_limit();
    conflicts = 0;
    index = decisions[0];
    decisions.clear();

    for ( int k = index ; k < trail.size(); ++k ) {
        unassign( trail[k].var() );
        form[reasons[k]].reason_index = -1;
    }

    trail.resize( index );
    reasons.resize( index );
}

/* iff all assigned then 0 */
var_t solver::get_unassigned( bool& polarity ) {
    var_t v_max = 0;

    do {
        v_max = heap.extract_max();
    }
    while ( !asgn.var_unassigned( v_max ) );


    bool pol = false;
    lbool saved = asgn.saved_phase(v_max);
    if ( saved ) {
        switch ( asgn.decision_count / 100 ) {
            case 0:
                pol = *saved;
                --asgn.decision_count;
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

    asgn.decision_count += 2;

    if ( asgn.decision_count >= asgn.period ) {
        asgn.decision_count = 0;
    }

    polarity = pol;
    return v_max;
}


bool solver::unit_propagation() {

    // repeatedly propagate enqueued literal
    while ( index < trail.size() ) {

        lit_t lit = trail[index++];
        lit.flip();

        // get indices of clauses where -lit occurs
        auto& clause_indices = occurs[lit];

        /* track two indices 
         * i - currently investigated index of occurs[-lit]
         * j - index of last element that will remain in occurs[-lit]
         *
         * i.e. swap and move elements to avoid erasing at the end
         */
        int j = 0;
        for ( int i = 0; i < clause_indices.size(); ++i ) {

            bool swapped = false;
            int clause_idx = clause_indices[i];
            auto [l1, l2] = watches[clause_idx];
            
            if ( lit != l1 ) {
                l2 = l1;
                l1 = lit;
                swapped = true;
            }

            // try to avoid moving watch
            if ( asgn.satisfies_literal( l2 ) ) {
                clause_indices[j++] = clause_idx;
                continue;
            }

            clause& c = form[clause_idx];

            if ( swapped ) {
                c.data[1] = c.data[0];
                c.data[0] = lit;
                std::swap( watches[clause_idx].first, watches[clause_idx].second );
            }

            /*
             * MOVE WATCH
             */
            
            bool found = false;

            // find unassigned literal
            for ( std::size_t k = 2; k < c.data.size(); ++k ) {

                lit_t l = c.data[k];

                // get ls assignment ( nullopt / bool )
                lbool& asgn_l = asgn[l.var()];

                // handle duplicit snd watch, TODO: can save this by preprocessing
                if (l == l2) {
                    continue;
                }

                // if the literal is unassigned or satisfied
                if ( !asgn_l || ( asgn_l == l.pol() ) ) {
                    // w1 = k;
                    std::swap( c.data[0], c.data[k] );
                    watches[clause_idx].first = c.data[0];
                    occurs[l].push_back( clause_idx );
                    found = true;
                    break;
                }
            }

            /* found new watch, do not increment j */
            if ( found ) {
                continue;
            }

            /* did not find new index for w1, the watch will remain in effect
             * swap the index entry and increment j*/
            clause_indices[j++] = clause_idx;
            // lit_t l = c.data[w2];

            // if second watch is unassigned, unit prop
            if ( asgn.lit_unassigned( l2 ) ) {
                assign( l2.var(), l2.pol() );
                reasons.push_back( clause_idx );
                c.reason_index = reasons.size() - 1;
            }

            /* if the second watch is unsat
             * copy the remaining watches and analyze_conflict() 
             */
            else if ( !asgn.satisfies_literal( l2 ) ) {

                // save index of conflict clause
                conflict_idx = clause_idx;
                i++;

                for ( ; i < clause_indices.size(); i++ ) {
                    clause_indices[j++] = clause_indices[i];
                }

                clause_indices.resize(j);
                return false;
            }

        }

        // adjust the occurs vector after watches have been moved
        clause_indices.resize(j);

    }
    return true;
}

void solver::add_base_clause(clause c) {
    form.add_base_clause(std::move(c));
}

void solver::add_learnt_clause(clause c) {
    form.add_learnt_clause(std::move(c));
}

void solver::backjump( int level, clause learnt ) {

    assert( level < decisions.size() );

    /* index of next decision level that is to be removed, i.e. all entries in
     * trail after decisions[level] will be deleted. In case if the _learnt_
     * clause is unit, all decisions will be deleted
     */
    int next_level = ( level > 0 ) ? decisions[level] : decisions[0];

    for ( int k = next_level ; k < trail.size(); ++k ) {
        unassign( trail[k].var() );
        form[reasons[k]].reason_index = -1;
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

int compute_lbd( const std::vector< lit_t > &lits ) {
    std::unordered_set< int > lvl_set;
    for ( lit_t l : lits ) {
        lvl_set.insert( l.var() );
    }

    return lvl_set.size();
}

std::pair< clause, int > solver::analyze_conflict() {

    std::vector< lit_t > learnt_clause{ 0 };
    int ind = trail.size() - 1;
    lit_t uip = 0;
    int lits_remaining = 0;

    std::vector< int > reasons_learnt;

    // stores index of currently resolved clause, starts with conflict clause
    int confl_idx = conflict_idx;


    /* repeatedly resolve away literals until first uip
     * the seen map stores literals that are present in the final clause
     */
    do {
        for ( lit_t& l : form[confl_idx].data ) {

            var_t lvar = l.var();
            if ( ( l != uip ) && levels[lvar] > 0 && !seen[lvar] ) {
                seen[l.var()] = 1;

                increase_var_priority( lvar );

                if ( levels[lvar] < current_level() ) {
                    learnt_clause.push_back( l );
                    reasons_learnt.push_back( reasons[ind] );
                }
                else {
                    lits_remaining++;
                }
            }
        }

        int new_lbd = compute_lbd( form[confl_idx].data );
        form[confl_idx].last_conflict = conflict_ctr;
        form.learnt.update_lbd( form[confl_idx], confl_idx - form.base.size(), new_lbd );

        // find next clause to resolve with
        while ( !seen[trail[ind].var()] ) { ind--; };

        uip = trail[ind];
        seen[uip.var()] = 0;
        lits_remaining--;


        confl_idx = reasons[ind];

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
            auto [l1, l2] = watches[reasons_learnt[i - 1]];

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

    // compute LBD
    int lbd = compute_lbd( learnt_clause );

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
    clause learnt( std::move(learnt_clause), true, lbd, conflict_ctr );

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

        var = get_unassigned( pol );

        if ( var == 0 ) {
            break;
        }

        decide(var, pol);
        log_solver_state( "ahojky" );

        while ( !unit_propagation() ) {
            if ( decisions.empty() ) {
                return false;
            }

            inc_conflict_ctr();
            if ( conflicts >= restart_limit ) {
                restart();
                break;
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

            backjump( level, std::move( learnt ) );
            log_solver_state( "after_conflict" );
        }
    }

    return true;
};
