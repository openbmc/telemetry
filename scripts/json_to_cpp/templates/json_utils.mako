#pragma once

#include <variant>
#include <optional>

#include <nlohmann/json.hpp>

namespace json_convert_utils {
    using nlohmann::json;

    template<typename T>
    std::optional<T> get_strict(const json& j) {
        bool is_type_correct = true;
        if constexpr (std::is_same<T, bool>::value) {
            is_type_correct = j.is_boolean();
        }
        else if constexpr (std::is_floating_point<T>::value) {
            is_type_correct = j.is_number_float();
        }
        else if constexpr (std::is_integral<T>::value) {
            if constexpr (std::is_unsigned<T>::value) {
                is_type_correct = j.is_number_unsigned();
            }
            else {
                is_type_correct = j.is_number_integer();
            }
        }
        if (is_type_correct) {
            try {
                return std::make_optional(std::move(j.get<T>()));
            }
            catch (...) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }


    template <typename T, std::size_t I = 0>
    T parse(const json& j)
    {
        if constexpr (I < std::variant_size_v<T>)
        {
            auto result = get_strict<std::variant_alternative_t<I, T>>(j);

            return result ? std::move(*result) : parse<T, I + 1>(j);
        }
        throw std::exception("Can't parse");
    }

}

namespace nlohmann {
    template <typename ...Args>
    struct adl_serializer<std::variant<Args...>> {
        static void from_json(const json& j, std::variant<Args...>& value) {
            value = json_convert_utils::parse<std::variant<Args...>>(j);
        }
    };
}
