import copy
import json
from jsonschema import validate


class InvalidSchema(Exception):
    def __init__(self, json_path, msg):
        super(InvalidSchema, self).__init__(
            f'Invalid json at {json_path}: {msg}')


class CppSchemaProperties:
    fields = 'cpp_fields'
    type = 'cpp_type'


class SchemaParser:
    default_type_map = {
        'integer': 'int64_t',
        'number': 'double',
        'string': 'std::string',
        'boolean': 'bool',
        'array': 'std::vector',
        'tuple': 'std::tuple',
        'variant': 'std::variant'
    }

    default_include_map = {
        'std::string': '<string>',
        'std::vector': '<vector>',
        'std::tuple': '<tuple>',
        'std::variant': '<variant>'
    }

    dependency_enum = ('templates/enum_conversion.mako', 'enum_conversion.hpp')
    dependency_variant = ('templates/json_utils.mako', 'json_utils.hpp')

    def __init__(self, json_data, type_map=None, include_map=None):
        self.type_map = self.default_type_map
        self.include_map = self.default_include_map
        if type(type_map) is dict:
            self.type_map |= type_map
        if type(include_map) is dict:
            self.include_map |= include_map
        self.dependency_templates = set()
        self.has_enum = False
        self.has_variant = False
        self.object_list = []
        self.includes_set = set()
        self.enums_map = {}
        self.objects_data = copy.deepcopy(json_data)
        self._validate_schema()
        self._get_cpp_types(self.objects_data)
        self._get_objects(self.objects_data)
        self.includes_list = list(sorted(self.includes_set))

    def _include_enum(self):
        self.includes_set.add(f'"{self.dependency_enum[1]}"')
        self.has_enum = True

    def _include_variant(self):
        self.includes_set.add(f'"{self.dependency_variant[1]}"')
        self.has_variant = True

    @staticmethod
    def obj_to_cpp_json(json_obj):
        json_type = type(json_obj)
        if json_type is dict:
            res = '{'
            for name, sub_obj in json_obj.items():
                res += f'{{"{name}",{SchemaParser.obj_to_cpp_json(sub_obj)}}},'
            res = res[:-1] + '}'
        elif json_type is list:
            array_str = ",".join(
                SchemaParser.obj_to_cpp_json(sub_obj) for sub_obj in json_obj)
            res = f'json::array({{{array_str}}})'
        elif json_type is str:
            res = f'"{json_obj}"'
        elif json_type is float or json_type is int:
            res = f'{json_obj}'
        elif json_type is bool:
            if json_obj:
                res = 'true'
            else:
                res = 'false'
        elif json_obj is None:
            res = 'nullptr'
        else:
            raise TypeError(f"Invalid json type: {str(json_type)}")
        return res

    def _get_cpp_types(self, json_object):
        props = CppSchemaProperties
        json_type = json_object['type']
        if 'cpp_type' not in json_object:
            if json_type == 'array':
                array_items = json_object['items']
                if array_items:
                    self._get_cpp_types(array_items)
                    json_object[
                        props.type] = \
                        f'{self.type_map[json_type]}<{array_items[props.type]}>'
                else:
                    tuple_items = json_object['prefixItems']
                    tuple_types = []
                    for tuple_item in tuple_items:
                        self._get_cpp_types(tuple_item)
                        tuple_types.append(tuple_item[props.type])
                    json_object[
                        props.type] = \
                        f'{self.type_map["tuple"]}<{", ".join(tuple_types)}>'
            elif isinstance(json_type, list):
                variant_types_str = ", ".join(
                    [self.type_map[j_type] for j_type in json_type])
                json_object[props.type] = \
                    f'{self.type_map["variant"]}<{variant_types_str}>'
                self._include_variant()
            elif json_type == 'object' and 'oneOf' in json_object:
                for idx, variant_object in enumerate(json_object['oneOf']):
                    self._get_cpp_types(variant_object)
                variant_types_str = ", ".join(
                    [variant[props.type] for variant in json_object["oneOf"]])
                json_object[props.type] = \
                    f'{self.type_map["variant"]}<{variant_types_str}>'
                self._include_variant()
            else:
                json_object[props.type] = self.type_map[json_type]
        if 'enum' in json_object and json_type == 'string':
            self.enums_map[json_object[props.type]] = json_object['enum']
            self._include_enum()
        for object_property in json_object.get('properties', []):
            self._get_cpp_types(json_object['properties'][object_property])
        for cpp_type, include in self.include_map.items():
            if cpp_type in json_object[props.type]:
                self.includes_set.add(include)

    def _get_objects(self, json_object, json_path='$'):
        props = CppSchemaProperties
        json_default_value = json_object.get('default', None)
        if json_default_value is not None:
            json_object['default_parsed'] = self.obj_to_cpp_json(
                json_default_value)
        json_type = json_object['type']
        if json_type == 'object':
            for idx, variant_object in enumerate(json_object.get('oneOf', [])):
                self._get_objects(variant_object, f'{json_path}[{idx}]')
            cpp_fields = json_object.get(props.fields, False)
            if cpp_fields:
                for object_property in json_object.get('properties', []):
                    if object_property not in cpp_fields:
                        raise InvalidSchema(json_path,
                                            f"Json property "
                                            f"'{object_property}' "
                                            f"is not contained in "
                                            f"'{props.fields}: "
                                            f"[{', '.join(cpp_fields)}]")
                    self._get_objects(
                        json_object['properties'][object_property],
                        json_path + f'[\'{object_property}\']')
                self.object_list.append(json_object)
        elif json_type == 'array':
            array_items = json_object.get('items')
            if array_items:
                self._get_objects(array_items, json_path + '[\'items\']')
            else:
                for idx, tuple_field in enumerate(json_object['prefixItems']):
                    self._get_objects(tuple_field,
                                      json_path + f'[\'prefixItems\'][{idx}]')

    def _validate_schema(self):
        with open('meta_schema/cpp_json_schema.json') as ff:
            schema = json.load(ff)
            validate(self.objects_data, schema)
