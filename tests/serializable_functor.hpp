#pragma once

namespace taskloaf {

struct SerializableFunctor {
    std::vector<int> vs;

    SerializableFunctor() = default;

    int operator()(int x) const {
        int out = x;
        for (auto& v: vs) {
            out *= v; 
        }
        return out;
    }

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(vs);
    }
};

inline Closure<int(int)> get_serializable_functor() {
    SerializableFunctor s;
    s.vs = {1,2,3,4};
    return {
        [] (std::vector<Data>& args, int a) {
            return args[0].get_as<SerializableFunctor>()(a);
        },
        {make_data(s)}
    };
}

} //end namespace taskloaf