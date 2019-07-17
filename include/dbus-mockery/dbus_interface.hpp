#pragma once

#include "methods.hpp"
#include "properties.hpp"
#include "signals.hpp"

#include <iostream>
#include <boost/type_index.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#include <tuple>
#include <type_traits>

namespace DBusMock::Mocks
{
    namespace detail
    {
        template <typename>
        struct method_dissect
        {
        };

        template <typename R, typename IFace, typename... Parameters>
        struct method_dissect <R(IFace::*)(Parameters...)>
        {
            using interface_type = IFace;
            using return_type = R;
            using parameters = std::tuple <Parameters...>;
        };
    }

    struct interface_mock_n_dummy
    {
        virtual ~interface_mock_n_dummy() = default;
    };

    template <typename InterfaceT, std::size_t Index>
    struct interface_mock_n{};

    template <typename InterfaceT>
    struct interface_mock{};
}

#define DBUS_MOCK_SEQUENCE_FACTORY_0(...) \
     ((__VA_ARGS__)) DBUS_MOCK_SEQUENCE_FACTORY_1
#define DBUS_MOCK_SEQUENCE_FACTORY_1(...) \
     ((__VA_ARGS__)) DBUS_MOCK_SEQUENCE_FACTORY_0
#define DBUS_MOCK_SEQUENCE_FACTORY_0_END
#define DBUS_MOCK_SEQUENCE_FACTORY_1_END

#define DBUS_MOCK_METHOD_DISSECT(IFace, Method) method_dissect<decltype(&IFace::Method)>

#define DBUS_MOCK_METHOD_RETURN(IFace, Method) DBUS_MOCK_METHOD_DISSECT(IFace, Method)::return_type

#define DBUS_MOCK_METHODS(...) BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)

#define DBUS_MOCK_PROPERTIES(...) BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)

#define DBUS_MOCK_SIGNALS(...) BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)

#define DBUS_MOCK_METHOD_SINGLE(r, IFace, Method) \
    namespace DBusMock::Mocks::detail \
    { \
        template <typename Owner, typename> \
        struct BOOST_PP_CAT(_mock_help ## _ ## IFace ## _, Method) \
        { \
        };\
        \
        template <typename Owner, typename IFace, typename... Parameters> \
        struct BOOST_PP_CAT(_mock_help ## _ ## IFace ## _, Method) <Owner, void(IFace::*)(Parameters...)> \
            : public virtual interface_method_base \
        { \
            using interface_method_base::interface_method_base;\
            \
            auto Method([[maybe_unused]] Parameters... params) -> void \
            { \
                dynamic_cast <interface_method_base*> (this)->call_method_no_reply(BOOST_PP_STRINGIZE(Method), params...); \
            } \
        };\
        \
        template <typename Owner, typename R, typename IFace, typename... Parameters> \
        struct BOOST_PP_CAT(_mock_help ## _ ## IFace ## _, Method) <Owner, R(IFace::*)(Parameters...)> \
            : public virtual interface_method_base \
        { \
            using interface_method_base::interface_method_base;\
            \
            auto Method([[maybe_unused]] Parameters... params) -> R \
            { \
                return static_cast <interface_method_base*> (this)->call_method <R, Parameters...> (BOOST_PP_STRINGIZE(Method), params...); \
            } \
        };\
        \
        template <typename Owner, typename R, typename IFace, typename... Parameters> \
        struct BOOST_PP_CAT(_mock_help ## _ ## IFace ## _, Method) <Owner, R(IFace::*)(Parameters...) const> \
            : public virtual interface_method_base \
        { \
            using interface_method_base::interface_method_base;\
            \
            auto Method([[maybe_unused]] Parameters... params) const -> R \
            { \
                return dynamic_cast <interface_method_base*> (this)->call_method <R, Parameters...> (BOOST_PP_STRINGIZE(Method), params...); \
            } \
        };\
        \
        template <typename Owner, typename IFace, typename... Parameters> \
        struct BOOST_PP_CAT(_mock_help ## _ ## IFace ## _, Method) <Owner, void(IFace::*)(Parameters...) const> \
            : public virtual interface_method_base \
        { \
            using interface_method_base::interface_method_base;\
            \
            auto Method([[maybe_unused]] Parameters... params) const -> void \
            { \
                dynamic_cast <interface_method_base*> (this)->call_method_no_reply(BOOST_PP_STRINGIZE(Method), params...); \
            } \
        };\
    }

#define DBUS_MOCK_METHOD_HELPER(IFace, Methods) \
    BOOST_PP_SEQ_FOR_EACH(DBUS_MOCK_METHOD_SINGLE, IFace, Methods)

#define DBUS_MOCK_METHOD_TYPE_FORGE(Data, Method) \
    BOOST_PP_CAT( \
        BOOST_PP_CAT( \
            BOOST_PP_CAT( \
                BOOST_PP_CAT(detail::_mock_help, _), BOOST_PP_TUPLE_ELEM(0, Data) \
            ), _ \
        ), Method \
    ) \
    <BOOST_PP_TUPLE_ELEM(1, Data), BOOST_PP_TUPLE_ELEM(2, Data), decltype(&BOOST_PP_TUPLE_ELEM(0, Data)::Method)>

// Data = (IFace, OwnerPart1, OwnerPart2)
#define DBUS_MOCK_METHOD_DERIVE(r, Data, Method) \
    , public DBUS_MOCK_METHOD_TYPE_FORGE(Data, Method)

#define DBUS_MOCK_PROPERTY_ROLL(r, IFace, Property) \
    std::decay_t <decltype(IFace::Property)> Property;

#define DBUS_MOCK_PROPERTY_CTOR(r, IFace, Property) \
    , Property{this, BOOST_PP_STRINGIZE(Property)}

#define DBUS_MOCK_METHOD_CTOR(r, Data, Method) \
    , DBUS_MOCK_METHOD_TYPE_FORGE(Data, Method){bus, service, path, interface}

// Numerator => some interfaces are very large, and i will hit boost preprocessor limits there :/
// This is why there is this numeration workaround.
#define DBUS_MOCK_IMPL(NUMERATOR, IFace, Methods, Properties, Signals) \
    DBUS_MOCK_METHOD_HELPER(IFace, Methods) \
    \
    namespace DBusMock::Mocks \
    { \
        template <> \
        struct interface_mock_n <IFace, NUMERATOR> \
            : virtual interface_mock_base \
              BOOST_PP_SEQ_FOR_EACH(DBUS_MOCK_METHOD_DERIVE, (IFace, interface_mock_n <IFace, NUMERATOR>), Methods) \
        { \
        public: \
            BOOST_PP_SEQ_FOR_EACH(DBUS_MOCK_PROPERTY_ROLL, IFace, Properties) \
        \
        public: \
            interface_mock_n( \
                Bindings::Bus& bus, \
                std::string const& service, \
                std::string const& path, \
                std::string const& interface \
            ) \
                : interface_mock_base{bus, service, path, interface} \
                , interface_method_base{bus, service, path, interface} \
                  BOOST_PP_SEQ_FOR_EACH(DBUS_MOCK_METHOD_CTOR, (IFace, interface_mock_n <IFace, NUMERATOR>), Methods) \
                  BOOST_PP_SEQ_FOR_EACH(DBUS_MOCK_PROPERTY_CTOR, IFace, Properties) \
            { \
            } \
            virtual ~interface_mock_n() = default; \
        }; \
    }

#define DBUS_MOCK_DERIVE_ZIP_SEQ_EACH(r, data, elem) \
    interface_mock_n <data, elem>,

#define DBUS_MOCK_CTOR_ZIP_SEQ_EACH(r, data, elem) \
    interface_mock_n <data, elem>{bus, service, path, interface},

/**
 * Used to merge all numerated mocks
 * @param IFace the Interface to mock
 * @param ... All numbers that were used for DBUS_MOCK_N
 */
#define DBUS_MOCK_ZIP_IMPL(IFace, SEQ) \
namespace DBusMock::Mocks \
{ \
    template <> \
    struct interface_mock <IFace> \
        : BOOST_PP_SEQ_FOR_EACH(DBUS_MOCK_DERIVE_ZIP_SEQ_EACH, IFace, SEQ) interface_mock_n_dummy \
    { \
        interface_mock( \
            Bindings::Bus& bus, \
            std::string const& service, \
            std::string const& path, \
            std::string const& interface \
        ) \
            : interface_mock_base{bus, service, path, interface} \
            , interface_method_base{bus, service, path, interface} \
            , BOOST_PP_SEQ_FOR_EACH(DBUS_MOCK_CTOR_ZIP_SEQ_EACH, IFace, SEQ) interface_mock_n_dummy{} \
        { \
        } \
    }; \
}

#define DBUS_MOCK_ZIP_IMPL_2(IFace, SEQ) \
    DBUS_MOCK_ZIP_IMPL(IFace, SEQ)

#define DBUS_MOCK_ZIP(IFace, ...) \
    DBUS_MOCK_ZIP_IMPL_2(IFace, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define DBUS_MOCK(IFace, Methods, Properties, Signals) \
    DBUS_MOCK_IMPL(0, IFace, Methods, Properties, Signals) \
    DBUS_MOCK_ZIP(IFace, 0)

#define DBUS_MOCK_N(N, IFace, Methods, Properties, Signals) \
    DBUS_MOCK_IMPL(N, IFace, Methods, Properties, Signals)

namespace DBusMock
{
    template <typename T>
    auto create_interface
    (
        Bindings::Bus& bus,
        std::string const& service,
        std::string const& path,
        std::string const& interface
    )
    {
        return Mocks::interface_mock <T>{bus, service, path, interface};
    }
}
