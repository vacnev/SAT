#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <optional>
#include <pair>

void ahoj();

using var_t = int;
using lit_t = int;
using lbool = std::optional< bool >;

struct clause {
    
    enum clause_status {
        undetermined = 0,
        satisfied = 1,
        conflict = 2,
        unit = 3
    };

    bool learnt;
    clause_status status;

    // two watched lits
    std::pair< int, int > watched;

    std::vector< lit_t > data;

    auto size() const {
        return data.size();
    }

    std::pair< lit_t, lit_t > watched_lits() {
        return { data[watched.first], data[watched.second] };
    }

    // move watch
    void resolve_watched(lit_t lit, auto& asgn, auto& occurs); // change status
};

struct formula {
    std::vector< clause > base;
    std::vector< clause > learnt;

    clause& operator[]( std::size_t index ) {
        if ( index >= base.size() ) {
            return learnt[index - base.size()];
        } else {
            return base[index];
        }
    }
};

struct solver {
    formula form;
    std::map< var_t, lbool > asgn;
    
    std::vector< lit_t > trail;
    std::vector< int > decisions; // indices into trail
    std::vector< int > reasons; // indices into form, -1 if dec
    std::map< lit_t, std::vector< int > > occurs;

    void decide( var_t x, bool v ) {
        assign(x, v);
        decisions.push_back(trail.back());
        reasons.push_back(-1);
    }

    void assign( var_t x, bool v ) {
        asgn[x] = v;
        lit_t l = ( v ) ? x : -x;
        trail.push_back(l);
    }

    void unit_propagation() {
        int index = trail.size() - 1;

        while ( index < trail.size() ) {
            lit_t lit = trail[index];

            for ( int i : occurs[-lit] ) {
                clause& c = form[i];
                c.resolve_watched(lit, asgn, occurs);
                if ( c.status == clause::unit ) {
                    lit_t l = c.watched_lits().first; 
                    assign( std::abs(l), l > 0 );
                    reasons.push_back(i);
                }
            }
        }
    }
};