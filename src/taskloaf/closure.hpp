#pragma once

#include "debug.hpp"
#include "fnc_registry.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/tuple.hpp>

namespace taskloaf {

struct data {
    virtual void save(cereal::BinaryOutputArchive& ar) const = 0;
    virtual void* get_storage() = 0;
    virtual const std::type_info& type() = 0;
};

struct data_ptr {
    std::unique_ptr<data> ptr;

    void save(cereal::BinaryOutputArchive& ar) const {
        (void)ar;
    }

    void load(cereal::BinaryInputArchive& ar) { 
        (void)ar;
    }

    template <typename T>
    T& get() {
        TLASSERT(ptr->type() == typeid(T));
        return *reinterpret_cast<T*>(ptr->get_storage());
    }

    template <typename T>
    operator T&() {
        return get<T>();
    }
};

template <typename T>
struct typed_data: public data {
    T v;

    typed_data(T& v): v(v) {}
    typed_data(T&& v): v(std::move(v)) {}

    void* get_storage() {
        return &v;
    }

    const std::type_info& type() {
        return typeid(T);
    }

    void save(cereal::BinaryOutputArchive& ar) const {
        ar(v);
    }
};

template <typename T>
data_ptr ensure_data(T&& v) {
    return {std::make_unique<typed_data<std::decay_t<T>>>(std::forward<T>(v))};
}

inline data_ptr ensure_data() {
    return {std::make_unique<typed_data<bool>>(false)};
}

struct closure {
    using caller_type = data_ptr(*)(closure&,data_ptr&);
    using serializer_type = void(*)(cereal::BinaryOutputArchive&);
    using deserializer_type = void(*)(closure&,cereal::BinaryInputArchive&);

    data_ptr f;
    data_ptr d;
    caller_type caller;
    serializer_type serializer;

    closure() = default;

    template <typename F>
    static void serializer_fnc(cereal::BinaryOutputArchive& ar) 
    {
        auto f = [] (closure& c, cereal::BinaryInputArchive& ar) {
            ar(c.f);
            ar(c.d);
            c.caller = &caller_fnc<F>;
            c.serializer = &serializer_fnc<F>;
        };
        auto deserializer_id = register_fnc<
            decltype(f),void,closure&,cereal::BinaryInputArchive&
        >::add_to_registry();
        ar(deserializer_id);
    }

    template <typename F>
    static data_ptr caller_fnc(closure& c, data_ptr& arg) {
        return ensure_data(c.f.template get<F>()(c.d, arg));
    }

    bool operator==(std::nullptr_t) const {
        return caller == nullptr; 
    }
    bool operator!=(std::nullptr_t) const {
        return !(*this == nullptr);
    }

    data_ptr operator()(data_ptr& arg) {
        return caller(*this, arg);
    }

    data_ptr operator()(data_ptr&& arg) {
        return caller(*this, arg);
    }

    data_ptr operator()() {
        auto d = ensure_data();
        return caller(*this, d);
    }
// 
//     void save(cereal::BinaryOutputArchive& ar) const {
//         serializer(ar);
//         ar(f);
//         ar(d);
//     }
// 
//     void load(cereal::BinaryInputArchive& ar) {
//         std::pair<size_t,size_t> deserializer_id;
//         ar(deserializer_id);
//         auto deserializer = reinterpret_cast<deserializer_type>(
//             get_fnc_registry().get_function(deserializer_id)
//         );
//         deserializer(*this, ar);
//     }
};

template <typename F, typename T>
auto make_closure(F fnc, T val) {
    auto f = ensure_data(std::move(fnc));
    auto d = ensure_data(std::move(val));

    return closure{
        std::move(f), std::move(d), 
        closure::template caller_fnc<F>,
        closure::template serializer_fnc<F>
    };
}

template <typename F>
auto make_closure(F fnc) {
    return make_closure(std::move(fnc), ensure_data());
}

} //end namespace taskloaf
