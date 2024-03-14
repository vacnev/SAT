#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <optional>
#include <utility>

using var_t = int;
using lit_t = int;
using lbool = std::optional< bool >;

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
    void resolve_watched(int clause_index, lit_t lit, std::map< var_t, lbool >& asgn, std::map< lit_t, std::vector< int > >& occurs) {
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

    formula(std::vector< clause > _base) : base(std::move(_base)) {}

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