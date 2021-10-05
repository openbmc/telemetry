import json
import sys
import subprocess
import argparse
import os
from mako.template import Template
from schema_parser import SchemaParser


class Settings:
    def __init__(self):
        self.formatter_cmd = []
        self.include_map = {}
        self.type_map = {}


def generate(template, output_name, output_dir, formatter_cmd, **mako_args):
    out_path = os.path.join(output_dir, output_name)
    with open(out_path, 'w+') as ff:
        render_data = Template(filename=template).render(**mako_args)
        ff.write(render_data)
    formatter_cmd = [part.replace('%file%', f'{out_path}') for part in
                     formatter_cmd]
    if formatter_cmd:
        subprocess.check_call(formatter_cmd)


def get_settings():
    res = Settings()
    if os.path.exists('settings.json'):
        with open('settings.json') as sf:
            data = json.load(sf)
            if type(data) is dict:
                res.formatter_cmd = data.get('formatter_cmd', [])
                res.type_map = data.get('type_map', {})
                res.include_map = data.get('include_map', {})
    return res


def main():
    parser = argparse.ArgumentParser(
        description='Script which generates c++ files from json schema.')
    parser.add_argument('schema',
                        help='path to json schema file (input)')
    parser.add_argument('-out', dest='out_dir', default='out',
                        help='directory to hold output files')
    parser.add_argument('-no_format', action='store_true',
                        help='If passed, formatter script will be run on output'
                             ' files. Check settings.json')

    args = parser.parse_args()
    settings = get_settings()

    if not os.path.exists(args.schema):
        print(f"Error: File {args.schema} does not exist!")
        return 1

    if not os.path.isdir(args.out_dir):
        os.makedirs(args.out_dir, exist_ok=True)

    formatter_cmd = [] if args.no_format else settings.formatter_cmd

    with open(args.schema) as ff:
        data = json.load(ff)
        parsed = SchemaParser(data)

        output_filename = os.path.basename(args.schema).rsplit('.json', 1)[
                              0] + '.hpp'
        generate('templates/types_hpp.mako', output_filename, args.out_dir,
                 formatter_cmd=formatter_cmd, object_list=parsed.object_list,
                 includes=parsed.includes_list, enum_map=parsed.enums_map)

        if parsed.has_enum:
            template_filename, output_filename = parsed.dependency_enum
            generate(template_filename, output_filename, args.out_dir,
                     formatter_cmd=formatter_cmd)

        if parsed.has_variant:
            template_filename, output_filename = parsed.dependency_variant
            generate(template_filename, output_filename, args.out_dir,
                     formatter_cmd=formatter_cmd)

    return 0


if __name__ == '__main__':
    sys.exit(main())
