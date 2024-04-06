#pragma once

#include <algorithm>
#include <cassert>
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


struct evsids_heap {

    /* priorities of elements in the heap */
    std::unordered_map< var_t, double > priorities;

    /* indices of elements in the heap */
    std::unordered_map< var_t, int > indices;

    /* stores the actual max heap of variables */
    std::vector< var_t > heap;

    int vars_count;


    evsids_heap( std::size_t count ) : vars_count( count ) {
        for ( std::size_t i = 1; i <= count; ++i ) {
            heap.emplace_back( i );
            priorities[i] = 1.0;
            indices[i] = i-1;
        }
    }


    bool lt( const var_t &l, const var_t &r ) {
        return priorities[l] < priorities[r];
    }

    int parent( int idx ) {
        return (idx-1) / 2;
        
    }

    int left( int idx ) {
        return 2 * idx + 1;
    }

    int right( int idx ) {
        return 2 * idx + 2;
    }

    void increase_priority( var_t v, float inc) {
        // if increased to larger val than 1e100, rescale prios/increment
        if ( ( priorities[v] += inc ) > 1e100 ) {
            for ( int i = 1; i < vars_count; i++ ) {
                priorities[v] *= 1e-100;
                inc *= 1e-100; 
            }
        }

        // if the variable is present in the heap
        if ( indices[v] != -1 ) {
            propagate( v );
        }
    }


    bool valid_heap ( int idx ) {
        bool valid = true;

        if ( left(idx) < heap.size() ) {
            valid = ( priorities[heap[idx]] >= priorities[heap[left(idx)]] ) && valid_heap( left(idx) );
        }

        if ( valid && ( right(idx) < heap.size()  ) )  {
            valid = ( priorities[heap[idx]] >= priorities[heap[right(idx)]] ) && valid_heap( right(idx) );
        }

        return valid;
    }
    void consistent_heap() {

        assert(valid_heap(0));

        for ( auto &[v, idx] : indices ) {
            if ( idx == -1 ) {
                for ( auto y : heap ) { 
                    assert( y != v ); 
                }
            }

            else {
                assert( heap[idx] == v );
            }
        }
    }
    
    // swap variables in the heap and their respective indices
    void heap_swap( int l, int r ) {
        std::swap( indices[heap[l]], indices[heap[r]] );
        std::swap( heap[l], heap[r] );
    }

    // propagate heap from variable v upward, i.e. adjust the heap to reflect the new
    // priority in priorities[v]
    void propagate( var_t v ) {

        int idx = indices[v];
        int parent_idx = 0;

        while ( idx != 0 ) {
            parent_idx = parent( idx );

            if ( lt( heap[parent_idx], heap[idx] ) ){
               heap_swap( idx, parent_idx );
               idx = parent_idx;
            }

            else { break; }
        }
    }
    
    void heapify( var_t v ) {

        int idx = indices[v];
        int child_idx = left(idx);
        int right_idx;

        while ( child_idx < heap.size() ) {
            right_idx = right( idx );

            // child idx stores index of child with larger prio
            if ( right_idx < heap.size() && lt( heap[child_idx], heap[right_idx] ) ) {
                child_idx = right_idx;
            }

            // if current has lower priority then the larger child, swap them &
            // continue
            if ( lt( heap[idx], heap[child_idx] ) ){
                heap_swap( child_idx, idx );

                idx = child_idx;
                child_idx = left( idx );

            }


            // else terminate
            else { break; }
        }
    }

    var_t extract_max(){
        // signal empty heap
        if ( heap.size() == 0 ) {
            return 0;
        }

        var_t v_max = heap[0];

        heap_swap( 0, heap.size() - 1 );
        heap.pop_back();


        indices[v_max] = -1;

        // heapify from root
        heapify( heap[0] );


        return v_max;
    }

    void insert( var_t v ) {
        // v is not in the heap
        if ( indices[v] == -1 ) {
            heap.push_back( v );
            indices[v] = heap.size() - 1;
            propagate( heap.back() );
        }
    }

};


struct assignment {
    std::size_t vars_count;
    std::vector< lbool > asgn;

    assignment(std::size_t count) : vars_count( count ), asgn( count + 1 ) { }

    lbool& operator[](var_t var) {
        return asgn[var];
    }


    void assign( var_t var, bool v ) {
        asgn[var] = v;
    }
    void unassign( var_t var ) {
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
