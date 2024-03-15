#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <optional>
#include <utility>
#include <queue>
#include <algorithm>

using var_t = int;
using lit_t = int;
using lbool = std::optional< bool >;

struct assignment {
    std::size_t vars_count;
    std::vector< lbool > asgn;

    // EVSIDS
    double inc = 1.01;
    std::vector< std::pair< double, var_t > > heap;

    // phase

    assignment(std::size_t count) : vars_count(count), asgn(count + 1) {
        for ( std::size_t i = 1; i <= count; ++i ) {
            heap.emplace_back( 1.0, i ); // add random init
            asgn.emplace_back(); 
            std::cout << asgn.back().has_value();
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
    //         var_t var = std::abs(lit);

    //     }
    // }

    lbool& operator[](var_t var) {
        return asgn[var];
    }

    void unassign(var_t var) {
        asgn[var] = std::nullopt;
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

    clause(std::vector< lit_t > _data) : learnt(false), data(std::move(_data)) {
        if ( data.size() == 1 ) {
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

    std::pair< lit_t, lit_t > watched_lits() {
        return { data[watched.first], data[watched.second] };
    }

    // move watch
    void resolve_watched(int clause_index, lit_t lit, assignment& asgn, std::map< lit_t, std::vector< int > >& occurs) {
        if ( status == UNIT ) {
            status = CONFLICT;
            return;
        }

        auto [l1, l2] = watched_lits();
        auto& [w1, w2] = watched;
        if ( lit != l1) {
            using std::swap;
            swap(w1, w2);
        }

        for ( std::size_t i = 0; i < data.size(); ++i ) {
            lit_t l = data[i];

            if (l == l1 || l == l2) {
                continue;
            }

            if ( !asgn[std::abs(l)] ) {
                w1 = i;
                occurs[l].push_back(clause_index);
                return;
            }
        }

        w1 = w2;
        lit_t l = data[w1];
        lbool v = asgn[std::abs(l)];
        if ( v ) {
            if ( *v && l > 0) {
                status = SATISFIED;
            } else {
                status = CONFLICT;
            }
            // occurs[l].erase(clause_index); // is needed?
        } else {
            status = UNIT;
        }
    }
};

struct formula {
    std::vector< clause > base;
    std::vector< clause > learnt;
    std::size_t var_count;

    formula(std::vector< clause > _base, std::size_t count) : base(std::move(_base)), var_count(count) {}

    clause& operator[]( std::size_t index ) {
        if ( index >= base.size() ) {
            return learnt[index - base.size()];
        } else {
            return base[index];
        }
    }

    void add_base_clause(clause c) {
        base.push_back(std::move(c));
    }

    void add_learnt_clause(clause c) {
        learnt.push_back(std::move(c));
    }
};
