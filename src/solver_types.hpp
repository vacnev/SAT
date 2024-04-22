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
#include <cstdint>

using var_t = int;
using lbool = std::optional< bool >;

struct lit_t {
    int lit;

    lit_t() : lit(0) { /*std::cout << "WARNING: default lit constr called\n";*/ };

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

    bool operator==( const lit_t &rhs ) const {
        return lit == rhs.lit;
    }

    inline int var() const {
        return std::abs( lit );
    }

    inline bool pol() const {
        return lit > 0;
    }

    inline void flip() {
        lit *= -1;
    }
};


/* occurs struct */
struct lit_map {
    std::vector< std::vector< int > > data;
    int var_count;

    lit_map( int count ) : data( 2 * count + 1 ), var_count( count ) {}

    std::vector< int >& operator[]( lit_t l ) {
        int lvar = l.var();
        int index = ( l.pol() ) ? lvar : lvar + var_count;
        return data[index];
    }
};


struct evsids_heap {

    /* priorities of elements in the heap */
    std::vector< double > priorities;

    /* indices of elements in the heap */
    std::vector< int > indices;

    /* stores the actual max heap of variables */
    std::vector< var_t > heap;

    int vars_count;

    evsids_heap( std::size_t count ) : vars_count( count ), indices( count + 1 ), priorities( count + 1 ) {
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

    void increase_priority( var_t v, double &inc ) {
        // if increased to larger val than 1e100, rescale prios/increment
        if ( ( priorities[v] += inc ) > 1e100 ) {
            for ( int i = 1; i <= vars_count; i++ ) {
                priorities[i] *= 1e-100;
            }
            inc *= 1e-100; 
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

        for ( int i = 1; i < indices.size(); i++) {
            int idx = indices[i];
            if ( idx == -1 ) {
                for ( auto y : heap ) { 
                    assert( y != i ); 
                }
            }

            else {
                assert( heap[idx] == i );
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

            if ( lt( heap[parent_idx], heap[idx] ) ) {
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
    const int period = 4 * 100;
    std::size_t vars_count;
    std::vector< lbool > asgn;
    int decision_count = 0;
    std::vector< lbool > last_phase;

    assignment(std::size_t count) : vars_count( count ), asgn( count + 1 ), last_phase( count + 1 ) { }

    lbool& operator[](var_t var) {
        return asgn[var];
    }

    lbool& saved_phase(var_t var) {
        return last_phase[var];
    }

    void assign( var_t var, bool v ) {
        asgn[var] = v;
        last_phase[var] = v;
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
        lbool& asgn_l = asgn[lit.var()];

        return asgn_l && asgn_l == lit.pol();
    }

};

struct clause {
    
    enum clause_status {
        UNDETERMINED = 0,
        SATISFIED = 1,
        CONFLICT = 2,
        UNIT = 3
    };

    enum learnt_type {
        CORE = 0,
        MID = 1,
        LOCAL = 2
    };

    bool learnt;
    clause_status status;
    int lbd;
    int_fast8_t reason_index = -1;
    learnt_type type;
    
    /* last conflict */
    int last_conflict;

    std::vector< lit_t > data;

    clause(std::vector< lit_t > _data, bool _learnt = false, int _lbd = 0, int conf_ctr = 0)
     : learnt(_learnt), data(std::move(_data)), lbd(_lbd), last_conflict(conf_ctr) {
        if ( learnt ) {
            status = UNIT;
            
            if ( lbd <= 3 ) {
                type = CORE;
            } else if ( lbd <= 6 ) {
                type = MID;
            } else {
                type = LOCAL;
            }
        }
        else if ( data.empty() ) {
            status = CONFLICT;
        }
        else if ( data.size() == 1 ) {
            status = UNIT;
        } else {
            status = UNDETERMINED;

            // UNIT cuz only 1 literal
            lit_t l = data[0];
            for ( lit_t lit : data ) {
                if ( l != lit ) {
                    return;
                }
            }

            status = UNIT;
        }
    }

    /* if smaller -> update */
    void update_lbd( int new_lbd ) {
        lbd = std::min( lbd, new_lbd );
        if ( lbd <= 3 ) {
            type = CORE;
        } else if ( lbd <= 6 ) {
            type = MID;
        } else {
            type = LOCAL;
        }
    }

    auto size() const {
        return data.size();
    }
};

struct formula {
    std::vector< clause > base;
    std::vector< clause > learnt;
    std::vector< uint_fast8_t > is_valid;
    std::vector< int > empty_indices;
    std::vector< double > activity;
    
    std::size_t clause_count;
    std::size_t var_count;
    int demote_limit = 30000;

    /* increment for forgetting */
    double inc = 1;

    /* decay */
    const double decay = 1.01;

    formula( std::vector< clause > _base, std::size_t count_c, std::size_t count_v ) : base(std::move( _base )), 
                                                                                       clause_count( count_c ),
                                                                                       var_count( count_v ) {}

    clause& operator[]( std::size_t index ) {
        if ( index < base.size() ) {
            return base[index];
        } else {
            return learnt[index - base.size()];
        }
    }

    const clause& operator[]( std::size_t index ) const {
        if ( index < base.size() ) {
            return base[index];
        } else {
            return learnt[index - base.size()];
        }
    }

    bool is_valid_clause( std::size_t index ) const {
        if ( index < base.size() ) {
            return 1;
        } else {
            return is_valid[index - base.size()];
        }
    }

    void add_base_clause(clause c) {
        base.push_back(std::move(c));
        clause_count++;
    }

    size_t next_index() {
        if ( empty_indices.size() > 0 ) {
            size_t idx = empty_indices.back();
            empty_indices.pop_back();
            return base.size() + idx;
        } else {
            return size();
        }
    }

    void add_learnt_clause(clause c, size_t idx) {
        idx -= base.size();
        if ( idx < learnt.size() ) {
            learnt[idx] = std::move(c);
            is_valid[idx] = 1;
            activity[idx] = 0;
        } else {
            learnt.push_back(std::move(c));
            is_valid.push_back(1);
            activity.push_back(0);
        }
        clause_count++;
    }

    size_t size() const {
        return base.size() + learnt.size();
    }

    /* move mid to local if not used in last 30k conflicts */
    void demote_clauses( int conflict_ctr, int demote_period ) {
        for ( int i = 0; i < learnt.size(); i++ ) {
            clause& c = learnt[i];
            if ( !is_valid_clause(i) || c.type != clause::MID ) {
                continue;
            }
            
            if ( c.last_conflict < conflict_ctr - demote_limit) {
                c.type = clause::LOCAL;
            }
            c.last_conflict -= demote_period;
        }
    }

    /* increase clause activity */
    void inc_activity( int idx ) {
        if ( idx < base.size() ) {
            return;
        }

        if ( (activity[idx - base.size()] + inc) > 1e100 ) {
            for ( double& act : activity ) {
                act *= 1e-100;
            }
            inc *= 1e-100;
        }
    }

    /* decay clause activity */
    void decay_activity() {
        inc *= decay;
    }

    /* forget bottom half of LOCAL clauses based on their activity */
    void forget_clauses( int conflict_idx ) {
        std::vector< std::pair< double, int > > act;
        for ( int i = 0; i < learnt.size(); i++ ) {
            if ( !is_valid_clause(i) || i == conflict_idx ) {
                continue;
            }

            clause& c = learnt[i];
            if ( c.type == clause::LOCAL && c.reason_index == -1 ) {
                act.emplace_back( activity[i], i );
            }
        }

        std::sort( act.begin(), act.end() );

        for ( int i = 0; i < act.size() / 2; i++ ) {
            int idx = act[i].second;
            empty_indices.push_back( idx );
            is_valid[idx] = 0;
        }
    }
};


// struct learnt_clauses {

//     /* demote req */
//     int demote_limit = 30000;

//     std::vector< clause > core; // LBD <= 3
//     std::vector< clause > mid; // 3 < LBD <= 6
//     std::vector< clause > local; // LBD > 6

//     /* demote if not used in last 30k conflicts
//      * invariant: reasons have been marked */
//     void demote_clauses( int conflict_ctr, int demote_period, std::vector< int >& reasons ) {
//         std::vector< clause > tmp_mid;
//         for ( int i = 0; i < mid.size(); i++ ) {
//             clause& c = mid[i];

//             if ( c.reason_index >= 0 && c.last_conflict < conflict_ctr - demote_limit ) {
//                 c.last_conflict -= demote_period;
//                 local.push_back( std::move(c) );
//             } else {
//                 c.last_conflict -= demote_period;
//                 tmp_mid.push_back( std::move(c) );
//             }
//         }

//         mid = std::move(tmp_mid);
//     }

//     /* adds clause based on its lbd */
//     void add_learnt_clause(clause c) {
//         if ( c.lbd <= 3 ) {
//             core.push_back(std::move(c));
//         } else if ( c.lbd <= 6 ) {
//             mid.push_back(std::move(c));
//         } else {
//             local.push_back(std::move(c));
//         }
//     }

//     clause& operator[]( std::size_t index ) {
//         if ( index < core.size() ) {
//             return core[index];
//         } else if ( index < core.size() + mid.size() ) {
//             return mid[index - core.size()];
//         } else {
//             return local[index - core.size() - mid.size()];
//         }
//     }

//     /* update lbd only if it is smaller */
//     void update_lbd( clause& cl, std::size_t index, int lbd ) {
//         if ( cl.lbd <= lbd ) {
//             return;
//         }
//         cl.lbd = lbd;

//         if ( index < core.size() ) {
//             return;
//         } else if ( index < core.size() + mid.size() ) {
//             index -= core.size();
//             if ( lbd <= 3 ) {
//                 core.push_back( std::move(mid[index]) );
//                 mid.erase( mid.begin() + index );
//             }
//         } else {
//             index -= core.size() - mid.size();
//             if ( lbd <= 3 ) {
//                 core.push_back( std::move(local[index]) );
//                 local.erase( local.begin() + index );
//             } else if ( lbd <= 6 ) {
//                 mid.push_back( std::move(local[index]) );
//                 local.erase( local.begin() + index );
//             }
//         }

//     }

//     auto size() const {
//         return core.size() + mid.size() + local.size();
//     }
// };