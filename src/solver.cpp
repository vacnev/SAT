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
    } else {
        // init occurs vecs
        occurs[l1].push_back( clref );
        occurs[l2].push_back( clref );
    }

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
    log.log() << "[ ";
    for ( auto x : decisions ) { log.log() << x << ", "; }
    log.log() << " ]\n\n";

    log.log() << "LEVELS:\n";
    log.log() << "[ ";
    for ( auto &[k, v] : levels ) { log.log() << k << " - " << v << "; "; }
    log.log() << " ]\n\n";

    log.log() << "REASONS:\n";
    log.log() << "[ ";
    for ( auto x : reasons ) { log.log() << x << ", "; }
    log.log() << " ]\n\n";

    log.log() << "REASON CLAUSES:\n";
    for ( auto x : reasons ) { 
        if ( x != -1 ) {
            log_clause(form[x], "Reason - " + std::to_string(x));
        }
    }

    log.log() << "------------------------------------" << std::endl;
}


void solver::decide( var_t x, bool v ) {
    assign(x, v);
    levels[x]++;
    decisions.push_back(trail.size() - 1);
    reasons.push_back(-1);
}

void solver::assign( var_t x, bool v ) {
    asgn[x] = v;
    lit_t l = ( v ) ? x : -x;
    trail.push_back(l);
    levels[x] = decisions.size();
}

bool solver::unit_propagation() {

    while ( index < trail.size() ) {

        lit_t lit = trail[index++];

        auto& clause_indices = occurs[-lit];
        auto og_size = clause_indices.size();

        for ( int curr_entry = 0; curr_entry < og_size; ++curr_entry ) {
            int i = clause_indices[curr_entry];
            clause& c = form[i];
            clause::clause_status status = c.resolve_watched(i, -lit, asgn, occurs);

            if ( status == clause::UNIT ) {
                lit_t l = c.watched_lits().second; 
                assign( l.var(), l.pol() );
                reasons.push_back( i );
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
        asgn.unassign( trail[i].var() );
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

void solver::backjump( int level, clause& learnt ) {
    std::size_t j = decisions[level - 1];
    for ( auto i = j ; i < trail.size(); ++i ) {
        asgn.unassign( trail[i].var() );
    }

    // adjust trail accordingly
    decisions.resize( level - 1 );
    trail.resize( j );
    reasons.resize( j );
    initialize_clause( learnt, form.size() );

    // set head of propagation queue to last
    index = trail.size() - 1;
}

std::pair< clause, int > solver::analyze_conflict( clause confl ) {

    std::vector< lit_t > learnt_clause{ 0 };
    int index = trail.size() - 1;
    lit_t uip = 0;
    std::vector< int > reasons_learnt;

    log_solver_state("conflict analysis");
    log_clause( confl, "conflict clause" );
    log.log() << std::endl;

    int lits_remaining = 0;
    bool finished = false;

    do {

        log.log() << "Index: " << index << "\n";
        log_clause( confl, "currently resolved clause" );
        log.log() << std::endl;
        auto [l1, l2] = confl.watched_lits();

        for ( lit_t l : confl.data ) {

            if ( ( uip == 0 || l != l2 ) && levels[l.var()] > 0 && !seen[l.var()] ) {
                seen[l.var()] = 1;

                if ( levels[l.var()] < current_level() ) {
                    learnt_clause.push_back( l );
                    reasons_learnt.push_back( reasons[index] );
                }
                else {
                    lits_remaining++;
                }
            }
        }

        log_clause( clause( learnt_clause ), "ahoj" );

        // find next clause to resolve with
        while ( !seen[trail[index].var()] ) { index--; };
        log.log() << "index:" << index << std::endl;
        log.log() << "lits:" << lits_remaining << std::endl;

        // last step - this will be equal to -1
        if ( reasons[index] != -1)
            confl = form[reasons[index]];

        uip = trail[index];
        seen[uip.var()] = 0;
        lits_remaining--;

    } while (lits_remaining > 0);

    learnt_clause[0] = -uip;
    log_clause(learnt_clause, "learnt_clause");
    auto to_clear = learnt_clause;

    // simplify learnt clause
    /*
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
    */

    // find backjump level
    int backjump_level = 1;
    if ( learnt_clause.size() > 1 ) {
        int max_i = 1;
        for ( int i = 2; i < learnt_clause.size(); ++i ) {
            if ( levels[learnt_clause[i].var()] > levels[learnt_clause[max_i].var()] ) {
                max_i = i;
            }
        }

        std::swap(learnt_clause[1], learnt_clause[max_i]);
        backjump_level = levels[learnt_clause[1].var()];
    }

    clause learnt( std::move(learnt_clause), true );
    
    // clear seen
    for ( lit_t l : to_clear ) {
        seen[l.var()] = 0;
    }

    return { learnt, backjump_level };
}

bool solver::solve() {

    log.set_log_level( log_level::TRACE );

    if ( unsat ) {
        return false;
    }

    // first UP
    if ( !unit_propagation() ) { return false; }

    while ( var_t var = asgn.get_unassigned() ) {

        decide(var, false);

        while ( !unit_propagation() ) {
            if ( decisions.empty() ) {
                return false;
            }
        
            assert( conflict_idx != -1 );
            auto [learnt, level] = analyze_conflict( form[conflict_idx] );
            if ( level == 0 ) {
                return false;
            }

            backjump(level, learnt);
            add_learnt_clause(std::move(learnt));
        }
    }

    return true;
};
