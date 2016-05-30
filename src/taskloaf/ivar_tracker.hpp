#pragma once

#include "ivar.hpp"
#include "fnc.hpp"
#include "address.hpp"
#include "closure.hpp"

namespace taskloaf {

struct ID;
struct Data;
struct Comm;

struct IVarTrackerInternals;

struct IVarTracker {
    std::unique_ptr<IVarTrackerInternals> impl;

    IVarTracker(Comm& comm);
    IVarTracker(IVarTracker&&);
    ~IVarTracker();

    void introduce(Address addr);
    void fulfill(const IVarRef& ivar, std::vector<Data> vals);
    void add_trigger(const IVarRef& ivar, TriggerT trigger);
    void dec_ref(const IVarRef& ivar);

    bool is_fulfilled_here(const IVarRef& ivar) const;
    std::vector<Data> get_vals(const IVarRef& ivar) const;

    size_t n_owned() const;
    size_t n_triggers_here() const;
    size_t n_vals_here() const;
    const std::vector<ID>& get_ring_locs() const;
    std::vector<Address> ring_members() const;
};

} //end namespace taskloaf
