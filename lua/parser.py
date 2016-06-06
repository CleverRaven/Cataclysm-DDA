
import clang.cindex
clang.cindex.Config.set_library_path('/usr/lib')

import re

'''
Thrown by the translate functions below to indicate the C++ type does not
have a matching Lua type and can therefor not be used in the Lua wrapper.
'''
class TypeTranslationError:
    def __init__(self, message):
        self.message = message
    def __str__(self):
        return self.message

'''
Raised when an object from C++ is not exported to Lua, for whatever reason.
'''
class SkippedObjectError:
    def __init__(self, message):
        self.message = message
    def __str__(self):
        return self.message

def is_public(cursor):
    return cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC

class Parser:
    def __init__(self):
        '''
        C++ types that should be exported by-value. They must support copy-construction
        and copy-assignment. Use the plain C++ name of their type. Note: enumerations
        and string_id and int_id are automatically exported as value, they don't need to
        be listed here.
        '''
        self.types_exported_by_value = [ ]
        '''
        C++ types that should be exported by-reference. They don't need to support
        copy-construction nor copy-assignment. Types may be exported by-value and
        by-reference.
        '''
        self.types_exported_by_reference = [ ]
        '''
        All the C++ types that should be exported to Lua. This should contain the
        real type, not a type alias. It can contain enumeration or class types.
        Enumerations are automatically added to types_exported_by_value. Class types
        must be added manually to types_exported_by_value or types_exported_by_reference
        or to both.
        '''
        self.types_to_export = [ ]

        self.string_ids = { }
        self.int_ids = { }

        self.enums = { }
        self.classes = { }
        self.index = clang.cindex.Index.create()

        self.build_in_typedefs = {
        }

    numeric_fixed_points = [
        clang.cindex.TypeKind.CHAR_U, clang.cindex.TypeKind.UCHAR, clang.cindex.TypeKind.CHAR16,
        clang.cindex.TypeKind.CHAR32, clang.cindex.TypeKind.USHORT, clang.cindex.TypeKind.UINT,
        clang.cindex.TypeKind.ULONG, clang.cindex.TypeKind.ULONGLONG, clang.cindex.TypeKind.UINT128,
        clang.cindex.TypeKind.CHAR_S, clang.cindex.TypeKind.SCHAR, clang.cindex.TypeKind.WCHAR,
        clang.cindex.TypeKind.SHORT, clang.cindex.TypeKind.INT, clang.cindex.TypeKind.LONG,
        clang.cindex.TypeKind.LONGLONG, clang.cindex.TypeKind.INT128
    ]
    numeric_floating_pints = [
        clang.cindex.TypeKind.FLOAT, clang.cindex.TypeKind.DOUBLE, clang.cindex.TypeKind.LONGDOUBLE,
    ]



    def add_export_by_value(self, cpp_name):
        self.types_exported_by_value.append(cpp_name)
        self.types_to_export.append(cpp_name)
    def add_export_by_reference(self, cpp_name):
        self.types_exported_by_reference.append(cpp_name)
        self.types_to_export.append(cpp_name)
    def add_export_by_value_and_reference(self, cpp_name):
        self.types_exported_by_value.append(cpp_name)
        self.types_exported_by_reference.append(cpp_name)
        self.types_to_export.append(cpp_name)
    def add_export_enumeration(self, cpp_name):
        self.add_export_by_value(cpp_name)



    def derived_class(self, t):
        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return t.get_declaration().underlying_typedef_type
        if t.kind == clang.cindex.TypeKind.RECORD or t.kind == clang.cindex.TypeKind.ENUM:
            return re.sub('^const ', '', t.spelling)
        return t.spelling

    def export_enabled(self, name):
        if type(name) is str:
            return name in self.types_to_export
        return self.export_enabled(self.derived_class(name))
    def export_by_value(self, name):
        if type(name) is str:
            return name in self.types_exported_by_value
        elif self.export_by_value(re.sub('^const ', '', name.spelling)):
            return True
        return self.export_by_value(self.derived_class(name))
    def export_by_reference(self, name):
        if type(name) is str:
            return name in self.types_exported_by_reference
        elif self.export_by_reference(re.sub('^const ', '', name.spelling)):
            return True
        return self.export_by_reference(self.derived_class(name))




    def get_string_id_for(self, name):
        for sid, base_type in self.string_ids.iteritems():
            if base_type == name:
                return sid
        return None
    def get_int_id_for(self, name):
        for iid, base_type in self.int_ids.iteritems():
            if base_type == name:
                return iid
        return None




    def parse_enum(self, cursor):
        if not self.export_enabled(cursor.type):
            return
        # Just a declaration, skip the actual parsing as it requires a proper definition.
        if not cursor.is_definition():
            return
        e = CppEnum(self, cursor);
        self.enums.setdefault(e.cpp_name, e)

    def parse_class(self, cursor):
        sp = cursor.spelling
        if not self.export_enabled(cursor.type):
            return
        # Just a declaration, skip the actual parsing as it requires a proper definition.
        if not cursor.is_definition():
            return
        if not self.export_by_reference(cursor.type) and not self.export_by_value(cursor.type):
            raise RuntimeError("Class %s should be exported, but is not marked as by-value nor by-reference!" % sp)
        c = CppClass.from_cursor(self, cursor)
        self.classes.setdefault(c.cpp_name, c)
    '''
    Parse the cursor under the assumption it's a typedef for a string/int id.
    If the cursor is no such typedef or if it is an id type, but
    the base type is not exported, return false. Otherwise return true.
    id_type must be 'string_id' or 'int_id' and ids_map is either self.string_ids
    or self.int_ids.
    '''
    def parse_id_typedef(self, cursor, id_type, ids_map):
        t = cursor.underlying_typedef_type

        # TODO: parse the template data of the underlying type via clang.
        m = re.match('^' + id_type + '<([a-zA-Z][_a-zA-Z0-9]*)>$', re.sub('^const ', '', t.spelling))
        if not m:
            return False

        typedef_name = re.sub('^const ', '', cursor.spelling)
        base_type = m.group(1)
        if typedef_name in ids_map:
            return True

        if not self.export_enabled(base_type):
            return False

        ids_map[typedef_name] = base_type
        # An id itself is always handled by value. It's basically a std::string/int.
        self.types_exported_by_value.append(typedef_name)
        return True



    def parse_typedef(self, cursor):
        # string_id and int_id typedefs are handled separately so we can include the typedef
        # name in the definition of the class. This allows Lua code to use the typedef name
        # instead of the underlying type `string_id<T>`.
        if self.parse_id_typedef(cursor, 'string_id', self.string_ids):
            return
        if self.parse_id_typedef(cursor, 'int_id', self.int_ids):
            return

        bt = self.build_in_lua_type(cursor.underlying_typedef_type)
        if bt:
            a = re.sub('^const ', '', cursor.spelling)
            self.build_in_typedefs[a] = bt


    def parse(self, header):
        try:
            opts = clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES | clang.cindex.TranslationUnit.PARSE_INCOMPLETE
            foo = ['-x', 'c++', '-std=c++11', '-isystem', '/usr/lib/clang/4.0.0/include']
            tu = self.index.parse(header, foo, options=opts)
            for d in tu.diagnostics:
                print("Diagnostic at %s:%s: %s" % (d.location.file.name, d.location.line, d.spelling))
            self.parse_cursor(tu.cursor)
        except clang.cindex.TranslationUnitLoadError as e:
            print("Failed to parse %s: %s" % (header, str(e)))



    def export(self, lua_file):
        # Export "dummy" class definitions for classes that are used in string/int ids.
        for c in self.string_ids:
            base_type = self.string_ids[c]
            if not base_type in self.classes:
                self.classes[base_type] = CppClass.from_name(self, base_type)
                self.types_to_export.append(base_type)
        for c in self.int_ids:
            base_type = self.int_ids[c]
            if not base_type in self.classes:
                self.classes[base_type] = CppClass.from_name(self, base_type)
                self.types_to_export.append(base_type)

        handled_types = [ ]
        with open(lua_file, 'wb') as f:
            f.write("classes = {\n")
            for c in self.types_to_export:
                if c in self.classes:
                    f.write(self.classes[c].export() + ",\n")
                    handled_types.append(c)
            f.write("}\n")

            f.write("\n" + "enums = {\n")
            for e in self.types_to_export:
                if e in self.enums:
                    f.write(self.enums[e].export() + ",\n")
                    handled_types.append(e)
            f.write("}\n")

        for t in self.types_to_export:
            if not t in handled_types:
                print("Type %s not found in any input source file" % (t))



    def parse_cursor(self, cursor):
        k = cursor.kind
        if k == clang.cindex.CursorKind.STRUCT_DECL or k == clang.cindex.CursorKind.CLASS_DECL or k == clang.cindex.CursorKind.CLASS_TEMPLATE:
            self.parse_class(cursor)
            return

        if k == clang.cindex.CursorKind.ENUM_DECL:
            self.parse_enum(cursor)
            return

        if k == clang.cindex.CursorKind.TYPEDEF_DECL or k == clang.cindex.CursorKind.TYPE_ALIAS_DECL:
            self.parse_typedef(cursor)
            return

        for c in cursor.get_children():
            self.parse_cursor(c)



    def register_id_type(self, t, ids_map, id_type):
        typedef = t
        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            t = t.get_declaration().underlying_typedef_type

        m = re.match('^' + id_type + '<([a-zA-Z][_a-zA-Z0-9]*)>$', t.spelling)
        if not m:
            m = re.match('^const ' + id_type + '<([a-zA-Z][_a-zA-Z0-9]*)>$', t.spelling)
            if not m:
                return None

        base_type = m.group(1)
        name = re.sub('^const ', '', typedef.spelling)
        if not name in ids_map:
            if not self.export_enabled(base_type):
                return None
            ids_map[name] = base_type
            # A id itself is always handled by value. It's basically a std::string/int.
            self.types_exported_by_value.append(name)

        return '"%s"' % name

    '''
    Returns the build-in Lua type of the given C++ type, if there is one. It returns
    None if there is no matching build-in type.
    Input can either be a string or a clang.cindex.Type.
    '''
    def build_in_lua_type(self, t):
        if type(t) is str:
            # Lua has build in support for strings and the bindings generator translates
            # std::string to Lua strings.
            if t == 'std::string':
                return 'std::string'
            # Typedefs encountered earlier that have been resolved already.
            elif t in self.build_in_typedefs:
                return self.build_in_typedefs[t]
            # If the input is a string, we can't really do anything more as we don't have
            # any information about the base type itself.
            else:
                return None

        # First try the canonical type as reported by clang. This will handle all common
        # typedefs correctly.
        ct = t.get_canonical()
        if ct.kind in Parser.numeric_fixed_points:
            return 'int'
        elif ct.kind in Parser.numeric_floating_pints:
            return 'float'
        elif ct.kind == clang.cindex.TypeKind.BOOL:
            return 'bool'

        # It's not a build-in C++ type that we can handle, so look at its actual name.
        # This removes the const because we export 'const int' and 'const std::string' the
        # same as non-const counterparts.
        # This calls the function with a string argument, and thereby handles std::string
        res = self.build_in_lua_type(re.sub('^const ', '', t.spelling))
        if res: return res

        # Not a known type, but if it's a typedef, maybe the underlying type is known.
        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return self.build_in_lua_type(t.get_declaration().underlying_typedef_type)

        return None



    '''
    Translate a return type (of a function call) to the wrapped type, suitable for
    the class definition.
    '''
    def translate_result_type(self, t):
        res = self.build_in_lua_type(t)
        if res: return '"%s"' % res

        if self.export_by_value(t):
            return '"%s"' % t.spelling

        # Only allowed as result type, therefor hard coded here.
        if t.get_canonical().kind == clang.cindex.TypeKind.VOID:
            return "nil"

        if t.kind == clang.cindex.TypeKind.LVALUEREFERENCE:
            pt = t.get_pointee()
            spt = pt.spelling
            if pt.is_const_qualified():
                # A const reference. Export it like a value (which means Lua will get a copy)
                res = self.build_in_lua_type(pt)
                if res: return '"%s"' % res

                if self.export_by_value(pt):
                    return '"%s"' % re.sub('^const ', '', spt)

            # const and non-const reference:
            if self.export_by_reference(pt):
                return '"%s&"' % re.sub('^const ', '', spt)

        if t.kind == clang.cindex.TypeKind.POINTER:
            pt = t.get_pointee()
            spt = pt.spelling
            # "const foo *" and "foo *" can be satisfied by an by-reference object.
            if self.export_by_reference(pt):
                return '"%s&"' % re.sub('^const ', '', spt)

        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return self.translate_result_type(t.get_declaration().underlying_typedef_type)

        ct = t.get_canonical()
        raise TypeTranslationError("unhandled type %s[%s] as result (%s[%s])" % (t.spelling, str(t.kind), ct.spelling, str(ct.kind)))



    '''
    Translate a function argument type to the wrapped type, suitable for
    the class definition.
    '''
    def translate_argument_type(self, t):
        res = self.build_in_lua_type(t)
        if res: return '"%s"' % res

        if self.export_by_value(t):
            return '"%s"' % re.sub('^const ', '', t.spelling)

        if t.kind == clang.cindex.TypeKind.LVALUEREFERENCE:
            pt = t.get_pointee()
            spt = pt.spelling
            if pt.is_const_qualified():
                # "const int &" can be satisfied by the build-in Lua "int".
                # But it won't work correctly with "int &" as the value passed to
                # the C++ functions is a temporary.
                res = self.build_in_lua_type(pt)
                if res: return '"%s"' % res

            # "const foo &" and "foo &" can be satisfied by an exported object.
            if self.export_by_value(pt):
                return '"%s"' % re.sub('^const ', '', spt)
            if self.export_by_reference(pt):
                return '"%s"' % re.sub('^const ', '', spt)

        if t.kind == clang.cindex.TypeKind.POINTER:
            pt = t.get_pointee()
            spt = pt.spelling
            # "const foo *" and "foo *" can be satisfied with by-reference values, they
            # have an overload that provides the pointer. It does not work for by-value objects.
            if self.export_by_reference(pt):
                return '"%s"' % re.sub('^const ', '', spt)

        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return self.translate_argument_type(t.get_declaration().underlying_typedef_type)

        ct = t.get_canonical()
        raise TypeTranslationError("unhandled type %s[%s] as argument (%s[%s])" % (t.spelling, str(t.kind), ct.spelling, str(ct.kind)))

    '''
    Translate a class member type to the wrapped type.
    '''
    def translate_member_type(self, t):
        res = self.build_in_lua_type(t)
        if res: return '"%s"' % res

        if self.export_by_reference(t):
            return '"%s"' % re.sub('^const ', '', t.spelling)
        if self.export_by_value(t):
            return '"%s"' % re.sub('^const ', '', t.spelling)

        if t.kind == clang.cindex.TypeKind.LVALUEREFERENCE:
            pt = t.get_pointee()
            # We can return a reference to the member in Lua, therefor allow types that
            # support by-reference semantic.
            if self.export_by_reference(pt):
                return '"%s"' % re.sub('^const ', '', pt.spelling)

        if t.kind == clang.cindex.TypeKind.POINTER:
            pt = t.get_pointee()
            if self.export_by_reference(pt):
                return '"%s"' % re.sub('^const ', '', pt.spelling)

        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return self.translate_member_type(t.get_declaration().underlying_typedef_type)

        ct = t.get_canonical()
        raise TypeTranslationError("unhandled type %s[%s] as member (%s[%s])" % (t.spelling, str(t.kind), ct.spelling, str(ct.kind)))



class CppClass:
    ''' Generic callable thing, it contains an argument list and can export the list. '''
    class CppCallable:
        def __init__(self, parent, cursor):
            self.parent = parent
            self.cursor = cursor
            # The min/max number of arguments. May differ if some parameters are optional.
            self.min_arguments = 0
            self.max_arguments = 0
            for a in self.cursor.get_arguments():
                self.max_arguments = self.max_arguments + 1
                if not self.has_default_value(a):
                    self.min_arguments = self.min_arguments + 1

        def has_default_value(self, a):
            # Sadly, this is not directly exposed by clang, one has to "parse" the parameter
            # declaration oneself.
            for t in a.get_tokens():
                if t.spelling == '=':
                    return True
            return False

        def export_argument_list(self, m):
            if m == 0:
                return '{ }'
            args = list(self.cursor.get_arguments())[0:m]
            args = [ self.parent.parser.translate_argument_type(a.type) for a in args]
            return "{ " + ", ".join(args) + " }"

        # Export all versions of the callable (multiple versions if there are optional parameters).
        # Result is an array. Each entry is created by calling the callback with exported
        # list of arguments (as string, ready to be printed) as parameter.
        def export_cb(self, callback):
            result = [ ]
            try:
                m = self.min_arguments
                while m <= self.max_arguments:
                    args = callback(self.export_argument_list(m))
                    if args:
                        result.append(args)
                    m = m + 1
            except TypeTranslationError as e:
                result.append("-- %s ignored because: %s" % (self.pretty_name(), e))
            except SkippedObjectError as e:
                result.append("-- %s ignored because: %s" % (self.pretty_name(), e))
            return result

        def pretty_name(self):
            return ('static ' if self.cursor.is_static_method() else '') + self.parent.cpp_name + '::' + self.cursor.spelling

    class CppFunction(CppCallable):
        def cb(self, args):
            # TODO: add support for *some* operators
            if re.match('^operator[^a-zA-Z0-9_]', self.cursor.spelling):
                raise SkippedObjectError("operator")
            if self.cursor.is_static_method():
                raise SkippedObjectError("static")

            result = self.parent.parser.translate_result_type(self.cursor.result_type)
            line = ""
            line = line + "{ "
            line = line + "name = \"" + self.cursor.spelling + "\", "
            line = line + "rval = " + result + ", "
            line = line + "args = " + args
            line = line + " }"
            return line

        def export(self):
            return self.export_cb(lambda x : self.cb(x))

    class CppConstructor(CppCallable):
        def export(self):
            return self.export_cb(lambda x : x)

    class CppAttribute:
        def __init__(self, parent, cursor):
            self.parent = parent
            self.cursor = cursor
            self.cpp_name = cursor.spelling
        def export(self):
            try:
                line = ""
                line = line + self.cpp_name + " = { "
                line = line + "type = " + self.parent.parser.translate_member_type(self.cursor.type)
                if not self.cursor.type.is_const_qualified():
                    line = line + ", writable = true"
                line = line + " }"
                return [ line ]
            except TypeTranslationError as e:
                return [ "-- %s::%s ignored because: %s" % (self.parent.cpp_name, self.cpp_name, e) ]
            except SkippedObjectError as e:
                return [ "-- %s::%s ignored because: %s" % (self.parent.cpp_name, self.cpp_name, e) ]

    def __init__(self, parser, cpp_name):
        self.functions = [ ]
        self.attributes = [ ]
        self.constructors = [ ]
        self.parser = parser
        self.cpp_name = cpp_name
        self.parents = [ ]
        self.has_equal = False

    @staticmethod
    def from_cursor(parser, cursor):
        result = CppClass(parser, cursor.spelling)

        for c in cursor.get_children():
            result.parse(c)

        return result

    @staticmethod
    def from_name(parser, cpp_name):
        result = CppClass(parser, cpp_name)
        return result

    def parse(self, cursor):
        if not is_public(cursor):
            return

        k = cursor.kind
        if k == clang.cindex.CursorKind.STRUCT_DECL or k == clang.cindex.CursorKind.CLASS_DECL:
            pass # Inner classes. Skipped for now. Maybe later. TODO.
        elif k == clang.cindex.CursorKind.CXX_METHOD:
            self.functions.append(CppClass.CppFunction(self, cursor))
            if cursor.spelling == 'operator==':
                # Note: this assumes any `operator==` compares with the same class type,
                # and not with a different type. TODO: check the arguments.
                self.has_equal = True
        elif k == clang.cindex.CursorKind.FIELD_DECL:
            self.attributes.append(CppClass.CppAttribute(self, cursor))
        elif k == clang.cindex.CursorKind.CONSTRUCTOR:
            self.constructors.append(CppClass.CppConstructor(self, cursor))
        elif k == clang.cindex.CursorKind.CXX_BASE_SPECIFIER:
            self.parents.append(cursor)

    def print_objects(self, objects, prefix, postfix):
        lines = [ ]
        for o in objects:
            for line in o.export():
                if line.startswith('--'):
                    print(line)
                else:
                    lines.append(line)

        if len(lines) == 0:
            return ''
        r = prefix
        for l in sorted(lines):
            r = r + ' ' * 12 + l + ',\n'
        r = r + postfix
        return r

    def gather_parent(self, pc, functions, attributes):
        definition = pc.get_definition()
        # Special construct to find templated classes like `visitable<T>`
        for c in pc.get_children():
            if c.kind == clang.cindex.CursorKind.TEMPLATE_REF:
                definition = c.get_definition()
                break
        parent = CppClass.from_cursor(self.parser, definition)
        functions += parent.functions
        attributes += parent.attributes
        for pc in parent.parents:
            parent.gather_parent(pc, functions, attributes)

    def export(self):
        tab = ' ' * 8
        # Both lists will include the functions of parent classes
        functions = self.functions
        attributes = self.attributes

        r = ""
        r = r + "    " + self.cpp_name + " = {\n"

        for pc in self.parents:
            definition = pc.get_definition()
            if self.parser.export_enabled(definition.type):
                r = r + tab + "parent = \"" + definition.spelling + "\",\n"
            else:
                # Parent class is not exported directly, but we still have to include its
                # functions and attributes
                self.gather_parent(pc, functions, attributes)

        # Exporting the constructor only makes sense for types that can have by-value semantic
        if self.parser.export_by_value(self.cpp_name):
            r = r + self.print_objects(self.constructors, tab + 'new = {\n', tab + '},\n')

        sid = self.parser.get_string_id_for(self.cpp_name)
        if sid:
            r = r + tab + "string_id = \"" + sid + "\",\n"
        iid = self.parser.get_int_id_for(self.cpp_name)
        if iid:
            r = r + tab + "int_id = \"" + iid + "\",\n"

        if self.parser.export_by_value(self.cpp_name) and self.parser.export_by_reference(self.cpp_name):
            r = r + tab + "by_value_and_reference = true,\n"
        elif self.parser.export_by_value(self.cpp_name):
            r = r + tab + "by_value = true,\n"
        else:
            pass # by reference is the default.

        if self.has_equal:
            r = r + tab + "has_equal = true,\n"

        r = r + self.print_objects(attributes, tab + 'attributes = {\n', tab + '},\n')
        r = r + self.print_objects(functions, tab + 'functions = {\n', tab + '}\n')

        r = r + "    }"

        return r



class CppEnum:
    def __init__(self, parser, cursor):
        self.values = [ ]
        self.cpp_name = cursor.spelling

        for c in cursor.get_children():
            if c.kind == clang.cindex.CursorKind.ENUM_CONSTANT_DECL:
                self.values.append(c.spelling)

    def export(self):
        r = ""
        r = r + "    " + self.cpp_name + " = {\n"
        for a in self.values:
            r = r + "        \"" + a + "\",\n"
        r = r + "    }"
        return r
