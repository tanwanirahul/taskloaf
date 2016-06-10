#pragma once
#include "address.hpp"

#include <cereal/archives/binary.hpp>

namespace taskloaf {

enum class Loc: int {
    anywhere = -2,
    here = -1
};

struct InternalLoc {
    bool anywhere;
    Address where;

    void save(cereal::BinaryOutputArchive& ar) const;
    void load(cereal::BinaryInputArchive& ar);
};

InternalLoc internal_loc(int loc);

struct closure;
void schedule(const InternalLoc& iloc, closure t);

} //end namespace taskloaf
