"""
Created on 05.05.2014

This work is based on https://github.com/Met48/inibin
   -> last fork 05.05.2014

@author: Carbon
"""
from .KEYTABLES import KEY_TABLE
from io_scene_lol.util import unpack

def default_sel_filter_func(d__key, d__value, d__typ):
    """The default selection filter. Variables can be used by provider"""
    return True

def default_format_func(key, value, typ):
    """The default format function will call the relevant element in the KEY_TABLE"""
    formatted = KEY_TABLE[key](value)
    return ("%s %s" % (typ, formatted) if formatted.matches('\xFF.*') else formatted, formatted)

def default_sort_func(iterable):
    """The default sorting function if none provided"""
    return sorted(iterable, key=lambda x: x[1])

class InibinData(object): # pylint: disable=too-few-public-methods
    """
    The overall data of the inibinfile
    """
    def __init__(self):
        """
        Initializes the data
        """
        self.header = InibinHeader()
        self.data_map = {}

    def str_data(self, sel_filter_func=default_sel_filter_func,
                         format_func=default_format_func,
                         sort_func=default_sort_func):
        """
        Stringifies the data in a format of the user's choice
        sel_filter_func: takes three arguments - the key, the value and the type of the value
        format_func: takes three arguments - the key, the value and the type of the value
                    -> a return value of None will not include the formatted string
                    -> you have to return a tuple, the second argument may be used for sorting
        sort_func: takes one argument - the list produced by calling the format_func
                                        on all items produced by the sel:filter_func
        """
        r_str_reps = []
        for key, val in self.data_map.items():
            value, typ = val
            if not sel_filter_func(key, value, typ):
                continue
            formatted, sorting = format_func(key, value, typ)
            if formatted is not None:
                r_str_reps.append((formatted, sorting))
        r_str_reps = sort_func(r_str_reps)
        r_str = "Inibin-File:\n%s\n" % (str(self.header))
        r_str += '\n'.join([str(i[0]) for i in r_str_reps])
        return r_str

    def __str__(self):
        return self.str_data()

class InibinHeader(object): # pylint: disable=too-few-public-methods
    """
    An inibin header. Describes used datatypes
    """
    def __init__(self):
        """
        Constructor
        """
        self.version = 2
        self.string_table_length = 0
        """
        Better described at the type_flag_table
        """
        self.set_type_flags = 0

    def unpack(self, fistream):
        """
        Reads the header from the instream
        """
        self.version, self.string_table_length, self.set_type_flags = unpack("<B2H", fistream)
        return self

    def __str__(self):
        """
        Stringify!
        """
        return "Version {0}\nFlags: {1:016b}\n----".format(self.version, self.set_type_flags)
