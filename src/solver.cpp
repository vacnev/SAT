#include "solver.hpp"

void solver::initialize_clause( const clause& cl, int clref ) {
    auto [l1, l2] = cl.watched_lits();

    // unit clause, setup trail for first UP
    if ( cl.status == clause::UNIT ) {
        if ( !asgn.lit_unassigned(l1) && !asgn.satisfies_literal(l1) ) {
            unsat = true;
            return;
        }

        assign( l1.var(), l1.pol() );
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


void solver::decide( var_t x, bool v ) {
    assign(x, v);
    decisions.push_back(trail.size() - 1);
    reasons.push_back(-1);
}

void solver::assign( var_t x, bool v ) {
    asgn[x] = v;
    lit_t l = ( v ) ? x : -x;
    trail.push_back(l);
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
    decisions.pop_back();

    // set literal to negation, fix assignment
    trail.back().flip();
    var_t var = trail.back().var();
    asgn[var] = 1 - *asgn[var];

    // set head of propagation queue to last
    index = trail.size() - 1;
}

bool solver::solve() {

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
        
            backtrack();
        }
    }

    return true;
}

