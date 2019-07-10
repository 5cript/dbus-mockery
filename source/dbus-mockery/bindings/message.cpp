#include <dbus-mockery/bindings/message.hpp>
#include <dbus-mockery/bindings/types.hpp>
#include <dbus-mockery/bindings/sdbus_core.hpp>

#include <stdexcept>

using namespace std::string_literals;

namespace DBusMock::Bindings
{
//#####################################################################################################################
    message::message(sd_bus_message* messagePointer)
        : msg{messagePointer}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    message::~message()
    {
        sd_bus_message_unref(msg);
    }
//---------------------------------------------------------------------------------------------------------------------
    message::operator sd_bus_message*()
    {
        return msg;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string message::comprehensible_type() const
    {
        auto descriptor = type();

        std::string result;
        if (descriptor.type == '\0')
        {
            return "void";
        }
        if (descriptor.type == 'a')
        {
            result = "array of "s;
        }
        for (char const* c = descriptor.contained.data(); c != nullptr && *c != 0; ++c)
        {
            result += typeToComprehensible(*c);
            if (*(c+1) != 0)
                result += ',';
        }

        return result;
    }
//---------------------------------------------------------------------------------------------------------------------
    type_descriptor message::type() const
    {
        char typeSig;
        const char* contentsSig;
        auto r = sd_bus_message_peek_type(msg, &typeSig, &contentsSig);
        if (r < 0)
            std::runtime_error("Failed to peek message type: "s + strerror(-r));

        return {typeSig, contentsSig};
    }
//#####################################################################################################################
}
