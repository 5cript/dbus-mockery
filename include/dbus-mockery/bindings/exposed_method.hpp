#pragma once

#include "sdbus_core.hpp"
#include "detail/dissect.hpp"
#include "types.hpp"
#include "message.hpp"

#include <string>
#include <vector>

#include <iostream>

extern "C" {
    int dbus_mock_exposed_method_handler(sd_bus_message *m, void *userdata, [[maybe_unused]] sd_bus_error *error);
}

namespace DBusMock
{
    namespace detail
	{
	    template <typename Tuple, template <typename...> class Function>
	    struct tuple_apply
		{ };

		template <template <typename...> class Function, typename... List>
		struct tuple_apply <std::tuple <List...>, Function>
		    : Function <List...>
		{
		};
	}

	class basic_exposed_method
	{
	public:
		virtual sd_bus_vtable make_vtable_entry(std::size_t offset) const	= 0;
		virtual void call(message& msg) = 0;
		virtual ~basic_exposed_method() = default;
	};

	template <typename FunctionT>
	struct exposed_method : public basic_exposed_method
	{
	private:
		// will be generated, but must be stored to survive interface registration.
		mutable std::string signature_;
		mutable std::string result_signature_;
		mutable std::string io_name_combined_;
		mutable std::size_t offset_;

	public:
		std::string method_name;
		std::string out_name;
		std::vector <std::string> in_names;
		uint64_t flags;
		FunctionT func;

		exposed_method() = default;
		~exposed_method() = default;

		exposed_method(exposed_method const&) = default;
		exposed_method& operator=(exposed_method const&) = default;
		exposed_method(exposed_method&&) = default;
		exposed_method& operator=(exposed_method&&) = default;

		template <typename... StringTypes>
		exposed_method
		(
		    std::string method_name,
		    std::string out_name,
		    StringTypes&&... in_names
		)
		    : method_name{std::move(method_name)}
		    , out_name{std::move(out_name)}
		    , in_names{std::forward <StringTypes&&>(in_names)...} // move on ptr is copy
		    , func{nullptr}
		{
		}

		void call(message& msg) override
		{
			auto resTuple = typename detail::method_dissect <FunctionT>::parameters{};

			std::apply([&msg](auto&... args) {
				(msg.read(args), ...);
			}, resTuple);

			if constexpr (!std::is_same_v <typename detail::method_dissect <FunctionT>::return_type, void>)
			{
				sd_bus_reply_method_return
				(
				    msg.handle(),
				    result_signature_.c_str(),
				    std::apply(func, resTuple)
				);
			}
		}

	private:
		void prepare_for_expose() const
		{
			io_name_combined_.clear();
			result_signature_.clear();
			signature_.clear();

			signature_ = detail::vector_flatten(detail::tuple_apply <
			    typename detail::method_dissect <FunctionT>::parameters,
			    detail::argument_signature_factory
			>::build());
			result_signature_ = detail::vector_flatten(detail::tuple_apply <
			    std::tuple <typename detail::method_dissect <FunctionT>::return_type>,
			    detail::argument_signature_factory
			>::build());

			for (auto const& i : in_names)
			{
				io_name_combined_ += i;
				io_name_combined_.push_back('\0');
			}
			if constexpr (!std::is_same_v <typename detail::method_dissect <FunctionT>::return_type, void>)
			{
				io_name_combined_ += out_name;
				io_name_combined_.push_back('\0');
			}
		}

	public:
		sd_bus_vtable make_vtable_entry(std::size_t offset) const
		{
			prepare_for_expose();
			offset_ = offset;

			return SD_BUS_METHOD_WITH_NAMES_OFFSET(
			    method_name.c_str(),
			    signature_.c_str(), ,
			    result_signature_.c_str(), io_name_combined_.data(),
			    &dbus_mock_exposed_method_handler,
			    offset_,
			    flags
			);
			/*
			// intentionally disregarding plea to use macros.
			sd_bus_vtable vt;
			memset(&vt, 0, sizeof(vt));
			vt.type = static_cast <decltype(vt.type)> (_SD_BUS_VTABLE_METHOD);
			vt.flags = flags;
			decltype(vt.x.method) method;
			method.member = method_name.c_str();

			method.signature = signature.c_str();
			method.result = result_signature.c_str();
			method.handler = &dbus_mock_exposed_method_handler;
			method.offset = 0;
			method.names = io_name_combined.c_str();
			vt.x.method = method;

			return vt;
			*/
		}
	};
}
