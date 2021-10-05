// This is generated file

#pragma once

% for include in includes:
#include ${include}
% endfor
#include <nlohmann/json.hpp>

namespace generated{

% for enum_name in enum_map:
enum class ${enum_name}{
%for field in enum_map[enum_name]:
    ${field},
%endfor
};

% endfor

% for object_data in object_list:
${make_struct(object_data)}
% endfor


% for enum_name, enum_fields in enum_map.items():
    ${make_enum_conversion_table(enum_name, enum_fields)}
%endfor

}

namespace nlohmann{
    % for enum_name, enum_fields in enum_map.items():
        ${make_custom_enum_serializer(enum_name, enum_fields)}
    %endfor


% for object_data in object_list:
    ${make_struct_serializer(object_data)}
%endfor
}



<%def name="make_struct(obj_data)">
struct ${obj_data['cpp_type']} {
% for name in obj_data['cpp_fields']:
        ${obj_data['properties'][name]['cpp_type']} ${name};
% endfor
};
</%def>

<%def name="make_custom_enum_serializer(enum_name, enum_fields)">
template <>
struct adl_serializer<${f'generated::{enum_name}'}> {
    static void from_json(const json& j, ${f'generated::{enum_name}'}& value) {
        value = generated::stringTo${enum_name}(j.get${'<std::string>'}());
    }
};
</%def>

<%def name="make_struct_serializer(obj_data)">
template <>
struct adl_serializer<${'generated::'}${obj_data['cpp_type']}> {
    static void from_json(const json& j, ${'generated::'}${obj_data['cpp_type']}& value) {
        % for name in obj_data['cpp_fields']:
            %if 'default_parsed' in obj_data['properties'][name]:
                if (auto prop = j.find("${name}"); prop != j.end()) {
                    prop->get_to(value.${name});
                }
                else {
                    json(${obj_data['properties'][name]['default_parsed']}).get_to(value.${name});
                }
            %else:
                j.at("${name}").get_to(value.${name});
            %endif
        % endfor
    }
};
</%def>

<%!
    def uncapitalize(text):
        if not text:
            return text
        if len(text) > 2:
            return text[0].lower() + text[1:]
        elif len(text) == 1:
            return text.lower()
%>

<%def name="make_enum_conversion_table(enum_name, enum_fields)">
namespace details{
constexpr std::array${f'<std::pair<std::string_view, {enum_name}>, {len(enum_fields)}>'} convData${enum_name} = {
    %for idx, field in enumerate(enum_fields):
        std::make_pair("${field}", ${enum_name}::${field})
            %if idx < len(enum_fields) - 1:
                ,
            %endif
    %endfor

};
}

inline ${enum_name} stringTo${enum_name}(const std::string& str)
{
    return details::stringToEnum(details::convData${enum_name}, str);
}

inline std::string ${uncapitalize(enum_name)}ToString(${enum_name} v)
{
    return std::string(details::enumToString(details::convData${enum_name}, v));
}
</%def>
