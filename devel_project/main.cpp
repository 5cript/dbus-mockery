#include <dbus-mockery/dbus_interface.hpp>
#include <dbus-mockery/bindings/bus.hpp>
#include <dbus-mockery/bindings/busy_loop.hpp>
#include <dbus-mockery/bindings/variant_helpers.hpp>
#include <dbus-mockery/generator/generator.hpp>

#include <dbus-mockery/bindings/exposable_interface.hpp>
#include <dbus-mockery/interface_builder.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include <thread>
#include <chrono>

#include <boost/type_index.hpp>

using namespace DBusMock;
using namespace std::chrono_literals;
using namespace std::string_literals;

class MyInterface : public DBusMock::exposable_interface
{
public:
    std::string path() const override
    {
        return "/bluetooth";
    }
    std::string service() const override
    {
        return "de.iwsmesstechnik.ela";
    }

public: // Methods
    auto DisplayText([[maybe_unused]] std::string const& text) -> void {
        std::cout << "message from bus: " << text << "\n";
        ++IsThisCool;
    }
    auto Multiply([[maybe_unused]] int lhs, [[maybe_unused]] int rhs) -> int {return lhs * rhs;}

public: // Properties
    int IsThisCool;

public: // Signals
    emitable <void(*)(int)> FireMe{this, "FireMe"};
    //using FireMe = void(*)(int);
};

int main()
{
    auto bus = open_user_bus();

    using namespace DBusMock;
    using namespace ExposeHelpers;

    std::cout << "exposing!\n";
    int r = 0;
    bool anyErr = false;

    auto exposed = make_interface <MyInterface> (
        DBusMock::exposable_method_factory{} <<
            name("Multiply") <<
            result("Product") <<
            parameter(0, "a") <<
            parameter(1, "b") <<
            as(&MyInterface::Multiply),
        DBusMock::exposable_method_factory{} <<
            name("DisplayText") <<
            result("Nothing") <<
            parameter("text") <<
            as(&MyInterface::DisplayText),
        DBusMock::exposable_property_factory{} <<
            name("IsThisCool") <<
            flags(property_change_behaviour::emits_change) <<
            writeable(true) <<
            as(&MyInterface::IsThisCool),
        DBusMock::exposable_signal_factory{} <<
            parameter("integral") <<
            as(&MyInterface::FireMe)
    );

    r = bus.expose_interface(exposed);
    anyErr = r < 0;
    if (r < 0)
        std::cout << "Could not expose interface: " << strerror(-r) << "\n";
    std::cout << r << "\n";

    std::cout << "requesting name: \n";
    r = sd_bus_request_name(static_cast <sd_bus*> (bus), "de.iwsmesstechnik.ela", 0);
    anyErr |= r < 0;
    if (r < 0)
        std::cout << "Exposed interface not found: " << strerror(-r) << "\n";

    if (!anyErr)
        std::cout << "interface exposed\n";

    make_busy_loop(&bus, 200ms);
    bus.loop <busy_loop>()->error_callback([](int r, std::string const& msg){
        std::cout << msg << std::endl;
        return true;
    });

    exposed->FireMe.emit(5);

    //while(true){std::this_thread::sleep_for(10ms);}
    std::cout << "exposition complete!" << std::endl;
    std::cin.get();

    /**
     * std::shared_ptr <MyInterface> iface;
     * bus->registerInterface(iface, "de.iwsmesstechnik.ela", "/ela/server");
     *
     *
     */
    // bus->registerInterface()

    return 0;
}
