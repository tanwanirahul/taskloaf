#pragma once

#include "fnc_registry.hpp"
#include "data.hpp"

#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/tuple.hpp>

namespace taskloaf {

struct Closure {
    using CallerT = Data (*)(Closure&, Data&);
    using SerializerT = void (*)(cereal::BinaryOutputArchive&);
    using DeserializerT = void (*)(Closure&, cereal::BinaryInputArchive&);

    Data f;
    Data d;
    CallerT caller;
    SerializerT serializer;

    Closure() = default;

    template <typename F, typename... Ts>
    Closure(F fnc, Ts... val):
        f(ensure_data(fnc)),
        d(ensure_data(std::move(val))...)
    {
        static_assert(
            std::is_trivially_copyable<std::decay_t<F>>::value,
            "F is not trivially copyable"
        );

        setup_policy<F>();
    }

    //TODO: Static asserts for F being trivially copyable, etc.
    // static_assert(
    //     !is_serializable<std::decay_t<F>>::value,
    //     "The function type for Closure cannot be serializable"
    // );

    template <typename F>
    void setup_policy() {
        caller = &closure_caller<F>;
        serializer = &closure_serializer<F>;
    }

    template <typename F>
    static void closure_serializer(cereal::BinaryOutputArchive& ar) 
    {
        auto f = [] (Closure& c, cereal::BinaryInputArchive& ar) {
            ar(c.f);
            ar(c.d);
            c.caller = &closure_caller<F>;
            c.serializer = &closure_serializer<F>;
        };
        auto deserializer_id = RegisterFnc<
            decltype(f),void,Closure&,cereal::BinaryInputArchive&
        >::add_to_registry();
        ar(deserializer_id);
    }

    template <typename F>
    static Data closure_caller(Closure& c, Data& arg) {
        return ensure_data(c.f.template unchecked_get_as<F>()(c.d, arg));
    }

    bool operator==(std::nullptr_t) const;
    bool operator!=(std::nullptr_t) const;

    Data operator()(Data& arg);
    Data operator()(Data&& arg);
    Data operator()();

    void save(cereal::BinaryOutputArchive& ar) const;
    void load(cereal::BinaryInputArchive& ar);
};

} //end namespace taskloaf
