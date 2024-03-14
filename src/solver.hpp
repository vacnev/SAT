#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <optional>
#include <utility>

void ahoj();

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
        auto& [m1, m2] = watched;
        if ( lit != l1) {
            using std::swap;
            swap(m1, m2);
        }

        for ( std::size_t i = 0; i < data.size(); ++i ) {
            lit_t l = data[i];

            if (l == l1 || l == l2) {
                continue;
            }

            if ( !asgn[std::abs(l)] ) {
                m1 = i;
                occurs[l].push_back(clause_index);
                return;
            }
        }

        m1 = m2;
        lit_t l = data[m1];
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

            for ( int i : occurs.extract(-lit).mapped() ) {
                clause& c = form[i];
                c.resolve_watched(i, -lit, asgn, occurs);
                if ( c.status == clause::UNIT ) {
                    lit_t l = c.watched_lits().first; 
                    assign( std::abs(l), l > 0 );
                    // c.status = clause::SATISFIED; // is needed?
                    reasons.push_back(i);
                }
            }
        }
    }
};