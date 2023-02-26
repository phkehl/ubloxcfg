#
# gdbnv - a natvis implementation for GDB debugger.
# Copyright (C) 2019 Rokas Kupstys
# Copyright (C) 2018-2019 asarium
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

import re
import sys
import traceback
from contextlib import suppress
from xml.etree import ElementTree

import gdb


def gdb_log(msg):
    gdb.write(f'{msg}\n', gdb.STDLOG)


def get_basic_type(gdb_type):
    """Return the "basic" type of a type.

    Arguments:
        gdb_type: The type to reduce to its basic type.

    Returns:
        type_ with const/volatile is stripped away,
        and typedefs/references converted to the underlying type.
    """

    while (gdb_type.code == gdb.TYPE_CODE_REF or
           gdb_type.code == gdb.TYPE_CODE_RVALUE_REF or
           gdb_type.code == gdb.TYPE_CODE_TYPEDEF or
           gdb_type.code == gdb.TYPE_CODE_PTR):
        if (gdb_type.code == gdb.TYPE_CODE_REF or
                gdb_type.code == gdb.TYPE_CODE_RVALUE_REF or gdb_type.code == gdb.TYPE_CODE_PTR):
            gdb_type = gdb_type.target()
        else:
            gdb_type = gdb_type.strip_typedefs()
    return gdb_type.unqualified()


def get_struct_type(gdb_type):
    """Return the "basic" type of a type.

    Arguments:
        gdb_type: The type to reduce to its basic type.

    Returns:
        type_ with const/volatile is stripped away,
        and typedefs/references converted to the underlying type.
    """
    gdb_type = gdb_type.strip_typedefs()
    while True:
        if gdb_type.code == gdb.TYPE_CODE_PTR or gdb_type.code == gdb.TYPE_CODE_REF or gdb_type.code == gdb.TYPE_CODE_RVALUE_REF:
            gdb_type = gdb_type.target()
        elif gdb_type.code == gdb.TYPE_CODE_ARRAY:
            gdb_type = gdb_type.target()
        elif gdb_type.code == gdb.TYPE_CODE_TYPEDEF:
            gdb_type = gdb_type.target()
        else:
            break

    if gdb_type.code != gdb.TYPE_CODE_STRUCT and gdb_type.code != gdb.TYPE_CODE_UNION:
        return None
    else:
        return gdb_type

    while (type_.code == gdb.TYPE_CODE_REF or
           type_.code == gdb.TYPE_CODE_RVALUE_REF or
           type_.code == gdb.TYPE_CODE_TYPEDEF or
           type_.code == gdb.TYPE_CODE_PTR):
        if (type_.code == gdb.TYPE_CODE_REF or
                type_.code == gdb.TYPE_CODE_RVALUE_REF or type_.code == gdb.TYPE_CODE_PTR):
            type_ = type_.target()
        else:
            type_ = type_.strip_typedefs()
    return type_.unqualified()


def get_type_name_or_tag(t):
    if t.name is not None:
        return t.name
    if t.tag is not None:
        return t.tag
    return None


def is_void_ptr(gdb_type):
    if gdb_type.code != gdb.TYPE_CODE_PTR:
        return False
    target = gdb_type.target()
    return target.code == gdb.TYPE_CODE_VOID


def strip_references(value):
    """
    Removes all types of references off of val. The result is a value with a non-pointer or reference type and a value of
    the same type
    :param value: The value to process
    :return: The basic value, the unqualified type of that value
    """
    while value.type.code == gdb.TYPE_CODE_REF or value.type.code == gdb.TYPE_CODE_RVALUE_REF or value.type.code == gdb.TYPE_CODE_PTR:
        if is_void_ptr(value.type):
            return None, None
        value = value.referenced_value()
    return value, value.type.unqualified()


def is_pointer(t):
    return t.code == gdb.TYPE_CODE_PTR


def is_natvis_taget(value):
    t = value.type.strip_typedefs()

    if is_pointer(t):
        target_t = t.target().strip_typedefs()
        if is_pointer(target_t):
            # We do not handle Pointer to pointer types since they probably are not meant to be used that way
            # The remaining code of the pretty printer would strip any pointer references away from the value so we need
            # to check for that here
            return False
        else:
            return True
    else:
        # Just assume that everything else is fine
        return True


class TemplateException(Exception):
    def __init__(self, input: str, pos: int, message: str) -> None:
        super().__init__('"{}":{}: {}'.format(input, pos, message))
        self.input = input
        self.pos = pos


class NvType(object):
    TEMPLATE_LIST_REGEX = re.compile("[<>,]")

    def __init__(self, name, args=None):
        super().__init__()

        if args is None:
            args = []
        self.name = name
        self.args = args
        self.full_name = ''

    @property
    def is_wildcard(self):
        return self.name == "*"

    def __repr__(self):
        return '<{}: {!r} [{}]>'.format(self.__class__.__name__, self.name, ",".join(repr(x) for x in self.args))

    def __str__(self):
        if len(self.args) <= 0:
            return self.name
        else:
            return '{}<{}>'.format(self.name, ", ".join(str(x) for x in self.args))

    def __eq__(self, other):
        if other is None:
            return False

        if self.is_wildcard:
            return True

        if not(len(self.args) == 1 and self.args[0].name == '*'):
            if len(self.args) != len(other.args):
                return False

            for left, right in zip(self.args, other.args):
                if left != right:
                    return False

        return self.name == other.name

    def __ne__(self, other):
        return not self.__eq__(other)

    @staticmethod
    def _skip_whitespace(input_text, start):
        while start < len(input_text) and input_text[start].isspace():
            start += 1
        return start

    # This pretty much implements a recursive descent parser without all the nice things a parser generator provides
    # Maybe this could be improved with a "real" parser generator...
    @staticmethod
    def _template_type_parse_runner(input, start):
        match = NvType.TEMPLATE_LIST_REGEX.search(input, start)
        if match is None:
            # No terminating character found
            return NvType(input[start:].rstrip()), len(input)
        name_end = match.start(0)
        if name_end == -1:
            # main match group not found but we still got a matcher?
            return NvType(input[start:].rstrip()), len(input)

        char = match.group(0)
        if char == ">" or char == ",":
            # The next template character is the end so we do not have a template argument list
            return NvType(input[start:name_end].rstrip()), name_end

        assert char == "<"  # The regex should not match anything else
        # We have a template list so we need to parse that before we can return our type
        arg_start = NvType._skip_whitespace(input, match.end(0))
        args = []
        while arg_start < len(input) and input[arg_start] != ">":
            arg_type, arg_end = NvType._template_type_parse_runner(input, arg_start)
            args.append(arg_type)
            arg_start = NvType._skip_whitespace(input, arg_end)

            if input[arg_start] == ",":
                # Skip over the comma
                arg_start = NvType._skip_whitespace(input, arg_start + 1)

                if input[arg_start] == "<" or input[arg_start] == ">" or input[arg_start] == ",":
                    raise TemplateException(input, arg_start + 1,
                                            'Expected a type name, got "{}" instead.'.format(input[arg_start]))
        if len(args) <= 0:
            raise TemplateException(input, arg_start + 1, 'Found type with template list but no parameters!')

        if arg_start >= len(input) or input[arg_start] != ">":
            raise TemplateException(input, arg_start + 1, 'Expected ">" but found "{}" instead!'
                                    .format("<EOF>" if arg_start >= len(input) else input[arg_start]))
        arg_start += 1  # Consume the '>'

        return NvType(input[start:name_end], args), arg_start

    @staticmethod
    def parse_type(template_str):
        template_type, end = NvType._template_type_parse_runner(template_str, 0)
        template_type.full_name = template_str
        if end < len(template_str):
            raise TemplateException(
                template_str, end + 1,
                f'Input remained after reading entire type! Additional string: {template_str[end:]}')
        return template_type


class NvPrettyPrinter(object):
    # Match elements between curly braces ignoring double curly braces.
    re_expression = re.compile(r'(?<!\{)\{([^\{\}]+)\}', re.IGNORECASE)
    # Match identifiers that are not preceeded by '.' or ':'.
    re_topmost_identifiers = re.compile(r'(?<![\.:\w\d,])([\w][\w\d_]+)', re.IGNORECASE)
    # Convert variables preceeded with $ to format specification.
    re_var_to_format = re.compile(r'\$([a-z0-9_]+)', re.IGNORECASE)
    # Expression,format
    re_expr_format = re.compile(r'^([^,]*)(,(d|o|x|h|hr|wc|wm|X|H|c|s|sa|sb|s8b|su|bstr|sub|s32|s32b|en|hv|na|nd))?$')

    def __init__(self, value, nv_type, el):
        self.value = value
        self.address = int(value.address)
        self.nv_type = nv_type
        self.el = el
        self.el_display_strings = el.findall('DisplayString')
        self.el_expand = el.find('Expand')
        self._display_string = None

    def display_hint(self):
        return 'array'

    def to_string(self):
        if not self._display_string:
            for el in self.el_display_strings:
                condition = el.attrib.get('Condition', None)
                if condition and not self._evaluate(condition, bool):
                    continue
                expression = el.text
                if (expression.startswith('"') and expression.endswith('"')) or \
                   expression.startswith("'") and expression.endswith("'"):
                    # Quoted strings may look good in VS, but not in CLion.
                    expression = expression[1:-1]
                self._display_string = self._evaluate_template(expression)
                break

        return self._display_string

    def __getattribute__(self, attr):
        # GDB is stupid. Presence of .children() method makes it think that we are printing something expandable. This
        # prevents making a mega-printer that handles all kinds of types.
        if attr == 'children' and self.display_hint() == 'string':
            raise AttributeError()
        return super().__getattribute__(attr)

    def children(self):
        # Temporary. Remove when  https://sourceware.org/bugzilla/show_bug.cgi?id=11335 is fixed.
        yield ('[Display String]', self._to_value_str(gdb.Value(self.to_string())))
        # Meant and bones of natvis renderer. Here we handle xml tags and output appropriate values
        for el in self.el_expand:
            if el.tag == 'Item':
                # Evaluate key:value pair where key is a static name.
                condition = el.attrib.get('Condition', None)
                if condition is not None and not self._evaluate(condition, bool):
                    continue
                yield (el.attrib.get('Name', 'Unnamed'), self._evaluate(el.text))
            elif el.tag == 'ArrayItems':
                # Loop `Size` times and yield all array elements.
                try:
                    size = self._evaluate(el.find('Size').text)
                    value_ptr = el.find('ValuePointer').text
                except Exception as e:
                    raise StopIteration() from e
                for i in range(size):
                    yield (f'[{i}]', self._evaluate(f'({value_ptr}) + {i}').dereference())

    def _to_value_str(self, value):
        return value.cast(gdb.lookup_type("char").pointer())

    def _evaluate_template(self, expression):
        result = expression[:]
        expressions = NvPrettyPrinter.re_expression.findall(expression)
        for expression in expressions:
            value = self._evaluate(expression)
            result = result.replace(f'{{{expression}}}', str(value))
        result = result.replace('{{', '{')
        result = result.replace('}}', '}')
        return result

    def _evaluate(self, expression, cast=None):
        """Evaluates a text expression within a context of current value. This is a hack :("""

        # Separate format specifier if such exists
        with suppress(AttributeError):
            expression, _, display_format = NvPrettyPrinter.re_expr_format.match(expression).groups()

        # Replace $Tn with actual template values
        expression = NvPrettyPrinter.re_var_to_format.sub(r'{\1}', expression)
        expression = expression.format(**{f'T{i + 1}': str(val) for i, val in enumerate(self.nv_type.args or [])})

        # Patch member field access
        identifiers = self.re_topmost_identifiers.findall(expression)
        field_names = [field for field, _ in self.value.type.items()]
        # Replace all field names with pointer access
        for field in identifiers:
            if field in field_names:
                # A hack to access fields
                expression = re.sub(fr'(?<![\.:\w]){field}', fr'(({self.nv_type.full_name}*){self.address})->{field}', expression)
            else:
                # A hack to access subtypes
                expression = re.sub(fr'(?<![\.:\w]){field}', f'{self.nv_type.full_name}::{field}', expression)

        result = gdb.parse_and_eval(expression)

        if display_format:
            result = self._format(result, display_format)

        if cast is not None:
            result = cast(result)
        return result

    def _format(self, value, display_format):
        if display_format == 'sb':
            value = self._to_value_str(value).string()
        elif display_format == 'd':
            value = int(value)
        return value


class AddNatvis(gdb.Command):
    def __init__(self, router):
        super().__init__("add-natvis", gdb.COMMAND_USER)
        self.router = router

    def invoke(self, argument: str, from_tty):
        args = gdb.string_to_argv(argument)

        if len(args) <= 0:
            print("Usage: add-nativs filename...")
            return

        for file in args:
            global db
            self.router.load_natvis(file)

    def dont_repeat(self):
        return True

    def complete(self, text: str, work: str):
        return gdb.COMPLETE_FILENAME


class NvPrettyPrinterRouter(gdb.printing.PrettyPrinter):
    def __init__(self, name):
        super().__init__(name)
        self._natvis = []
        self._command = AddNatvis(self)

    def load_natvis(self, file_name):
        dom = ElementTree.parse(file_name)
        root = dom.getroot()
        namespace = '{http://schemas.microsoft.com/vstudio/debugger/natvis/2010}'
        if root.tag != f'{namespace}AutoVisualizer':
            gdb_log(f'{file_name} is not a valid natvis file.')
            return False

        # Strip namespace from tag names
        namespace_len = len(namespace)

        def strip_ns(el):
            if el.tag.startswith(namespace):
                el.tag = el.tag[namespace_len:]
            for c in el:
                strip_ns(c)

        strip_ns(root)

        for typeTag in root:
            if typeTag.tag == 'Type':
                self._natvis.append((NvType.parse_type(typeTag.attrib['Name']), typeTag))

        return True

    def _get_nv_type(self, val):
        if not is_natvis_taget(val):
            return None

        val, val_type = strip_references(val)
        if val is None:
            # Probably a void ptr
            return None

        val_type = val_type.strip_typedefs()
        if not val_type:
            return None

        val_type = get_basic_type(val_type)

        if get_type_name_or_tag(val_type) is None:
            # We can't handle unnamed types
            return None

        if val_type.code != gdb.TYPE_CODE_UNION and val_type.code != gdb.TYPE_CODE_STRUCT:
            # Non-structs are not handled by this printer
            return None

        return NvType.parse_type(val_type.name)

    def __call__(self, value):
        try:
            if value is None or value.address is None:
                return

            # try:
            requested_type = self._get_nv_type(value)
            for nv_type, el in self._natvis:
                if nv_type == requested_type:
                    return NvPrettyPrinter(value, requested_type, el)

        except Exception as e:
            exc_type, exc_value, exc_tb = sys.exc_info()
            gdb_log("".join(traceback.format_exception(type(e), e, exc_tb)))


def register(debug=False):
    printer = NvPrettyPrinterRouter('Natvis')
    gdb.printing.register_pretty_printer(None, printer)

    if debug:
        import pydevd_pycharm
        pydevd_pycharm.settrace('localhost', port=41879, stdoutToServer=True, stderrToServer=True, suspend=False)
