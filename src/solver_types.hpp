#pragma once

#include <algorithm>
#include <concepts>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <queue>
#include <vector>
#include <unordered_map>
#include <utility>

using var_t = int;
using lbool = std::optional< bool >;

struct lit_t {
    int lit;

    lit_t() : lit(0) { std::cout << "WARNING: default lit constr called\n"; };

    lit_t(std::convertible_to< int > auto&& l) {
        lit = l;
    }

    operator int() {
        return lit;
    }

    auto& operator=(std::convertible_to< int > auto&& l) {
        lit = l;
        return *this;
    }

    int var() const {
        return std::abs( lit );
    }

    bool pol() const {
        return lit > 0;
    }

    void flip() {
        lit *= -1;
    }
};

struct assignment {
    std::size_t vars_count;
    std::vector< lbool > asgn;

    // EVSIDS
    double inc = 1.01;
    std::vector< std::pair< double, var_t > > heap;

    // phase

    assignment(std::size_t count) : vars_count( count ), asgn( count + 1 ) {
        for ( std::size_t i = 1; i <= count; ++i ) {
            heap.emplace_back( 1.0, i ); // add random init
            std::make_heap( heap.begin(), heap.end() );
        }
    }


    /* iff all assigned then 0 */
    var_t get_unassigned() const {
        // EVSIDS
        // while ( asgn[heap.front().second] ) {
        //     std::pop_heap(heap.front(), heap.back());
        //     heap.pop_back();
        //     // kdy vracet, muzeme jen drzet back it misto pop_back
        // }

        // if ( !heap.empty() ) {
        //     var_t var = heap.front();
        //     std::pop_heap(heap.front(), heap.back());
        //     heap.pop_back();
        //     return var;
        // }

        // naive
        for ( std::size_t i = 1; i <= vars_count; ++i) {
            if ( !asgn[i] ) {
                return i;
            }
        }

        return 0;
    }

    // void increase_priority(const std::vector< lit_t >& cl) {
    //     for ( lit_t lit : cl ) {
    //         var_t var = lit.var();

    //     }
    // }

    lbool& operator[](var_t var) {
        return asgn[var];
    }

    void unassign(var_t var) {
        asgn[var] = std::nullopt;
    }

    bool var_unassigned( var_t var ) const {
        return !asgn[var];
    }

    bool lit_unassigned( lit_t lit ) const {
        return var_unassigned( lit.var() );
    }

    // input condition: only called on literals that are assigned
    // returns whether this literal is satisfied by assignment
    bool satisfies_literal( lit_t lit ) {
        if ( lit_unassigned( lit ) )
            return false;

        // ( lit > 0 ) true iff positive literal
        // asgn[ ... ] true iff variable is assigned true
        return asgn[ lit.var() ] == lit.pol();
    }

};

struct clause {
    
    enum clause_status {
        UNDETERMINED = 0,
        SATISFIED = 1,
        CONFLICT = 2,
        UNIT = 3
    };

    bool learnt;
    clause_status status;

    // two watched lits
    std::pair< int, int > watched;

    std::vector< lit_t > data;

    clause(std::vector< lit_t > _data, bool _learnt = false) : learnt(_learnt), data(std::move(_data)) {
        if ( learnt ) {
            status = UNIT;
            watched = { ( data.size() > 1 ), 0 };
        }
        else if ( data.empty() ) {
            status = CONFLICT;
            watched = { 0, 0 };
        }
        else if ( data.size() == 1 ) {
            status = UNIT;
            watched = { 0, 0 };
        } else {
            status = UNDETERMINED;
            watched = { 0 , 1 };
        }
    }

    auto size() const {
        return data.size();
    }

    std::pair< lit_t, lit_t > watched_lits() const {
        return { data[watched.first], data[watched.second] };
    }


    /** input condition:
     *      clause watched store indices of two unassigned literals
     *      (or one which is true)
     *          
     *      only called after literal l is assigned either through decide() or
     *      unit_propagation(), with arg lit equal to -l. 
     *
     *      only called on clauses where -l is one of the watched literals, i.e. clause is present
     *      in occurs[-l].
     *
     *      returns status signal:
     *          UNIT if unit propagation is needed for this clause, with the
     *          unit propped literal stored as the second entry of
     *          watched_lits()
     *
     *          CONFLICT - self explanatory
     *          UNDETERMINED - if nothing else needs to be done
     *          SATISFIED - if one of the watched literals is currently
     *          satisfied under asgn
     **/
     
    clause_status resolve_watched(int clause_index, lit_t lit, assignment& asgn, std::unordered_map< int, std::vector< int > >& occurs) {

        auto [l1, l2] = watched_lits();
        auto& [w1, w2] = watched;
        
        if (lit != l1) {
            using std::swap;
            swap( w1, w2 );
            swap( l1, l2 );
        }

        // try to avoid moving watch
        if ( asgn.satisfies_literal( l2 ) ) {
            occurs[ data[w1] ].push_back( clause_index );
            return SATISFIED;
        }

        // find unassigned literal
        for ( std::size_t i = 0; i < data.size(); ++i ) {
            lit_t l = data[i];

            if (l == l1 || l == l2) {
                continue;
            }

            // sat lit => SATISFIED
            if ( asgn.lit_unassigned( l ) || asgn.satisfies_literal( l ) ) {
                w1 = i;
                occurs[l].push_back( clause_index );
                return UNDETERMINED;
            }
        }

        /*
         * did not find new index for w1
         * w1 will stay at its current index, restore its watch
         */

        occurs[ data[w1] ].push_back( clause_index );

        lit_t l = data[w2];

        // if second watch is unassigned, unit prop
        if ( asgn.lit_unassigned( l ) ) {
            return UNIT;
        }

        // if the second watch is unsat, return conflict
        else if ( !asgn.satisfies_literal( l ) ) {
            return CONFLICT;
        }
        
        return SATISFIED;
    }
};

struct formula {
    std::vector< clause > base;
    std::vector< clause > learnt;
    
    std::size_t clause_count;
    std::size_t var_count;

    formula( std::vector< clause > _base, std::size_t count_c, std::size_t count_v ) : base(std::move( _base )), 
                                                                                       clause_count( count_c ),
                                                                                       var_count( count_v ) {}

    clause& operator[]( std::size_t index ) {
        if ( index >= base.size() ) {
            return learnt[index - base.size()];
        } else {
            return base[index];
        }
    }

    const clause& operator[]( std::size_t index ) const {
        if ( index >= base.size() ) {
            return learnt[index - base.size()];
        } else {
            return base[index];
        }
    }
    void add_base_clause(clause c) {
        base.push_back(std::move(c));
        clause_count++;
    }

    void add_learnt_clause(clause c) {
        learnt.push_back(std::move(c));
        clause_count++;
    }

    auto size() const {
        return base.size() + learnt.size();
    }
};
