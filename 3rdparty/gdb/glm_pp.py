import gdb.printing

_type_letters = {
    gdb.TYPE_CODE_FLT: "d", # or "f"
    gdb.TYPE_CODE_INT: "i",
    gdb.TYPE_CODE_BOOL: "b"
}

def _vec_info(v):
    # vec contains either a union of structs or a struct of unions, depending on
    # configuration. gdb can't properly access the named members, and in some
    # cases the names are wrong.
    # It would be simple to cast to an array, similarly to how operator[] is
    # implemented, but values returned by functions called from gdb don't have
    # an address.
    # Instead, recursively find all fields of required type and sort by offset.
    typ = v.type

    # Dereference reference types automatically
    if typ.code == gdb.TYPE_CODE_REF:
      typ = typ.target()
      v = v.referenced_value()

    T = v.type.template_argument(1) 
    letter = _type_letters.get(T.code, "t")
    length = v.type.sizeof // T.sizeof
    items = {}
    def find(v, bitpos):
        t = v.type.strip_typedefs()
        if t.code in (gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION):
            for f in t.fields():
                if hasattr(f, "bitpos"): # not static
                    find(v[f], bitpos + f.bitpos)
        elif t == T:
            items[bitpos] = v
    find(v, 0)
    assert len(items) == length
    items = [str(f) for k, f in sorted(items.items())]
    return letter, length, items

class VecPrinter:
    def __init__(self, v):
        self.v = v

    def to_string(self):
        letter, length, items = _vec_info(self.v)
        return "{}vec{}({})".format(letter, length, ", ".join(items))

class MatPrinter:
    def __init__(self, v):
        self.v = v

    def to_string(self):
        V = self.v["value"]
        columns = []
        for i in range(V.type.range()[1] + 1):
            letter, length, items = _vec_info(V[i])
            columns.append("({})".format(", ".join(items)))
        return "{}mat{}x{}({})".format(
            letter, len(columns), length, ", ".join(columns))

def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("glm_pp")
    pp.add_printer(
        "glm::vec", r"^glm::vec<\d+, \w+, \(glm::qualifier\)0>$", VecPrinter)
    pp.add_printer(
        "glm::vec", r"^glm::mat<\d+, \d+, \w+, \(glm::qualifier\)0>$", MatPrinter)

    return pp

gdb.printing.register_pretty_printer(gdb.current_objfile(),
                                     build_pretty_printer())
