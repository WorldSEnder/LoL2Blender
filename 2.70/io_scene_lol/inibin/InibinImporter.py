"""
Created on 05.05.2014

This work is based on https://github.com/Met48/inibin
   -> last fork 05.05.2014

@author: Carbon
"""
from ..util import AbstractReader, read_from_file, seek, tell_f_length, unpack
from .InibinData import InibinData, default_format_func, default_sel_filter_func, \
    default_sort_func
from .KEYTABLES import KEY_TABLE, TYPEFLAG_TABLE, TYPE_OF_FLAG

# the default filter filters outdated keys
DEFAULT_FILTER = lambda key, key_type: (not KEY_TABLE[key].matches(r'zzz\.*') and key_type)

def import_from_file(filepath, context, **options):
    """
    Imports an inibin from the given filepath
    """
    return read_from_file(InibinReader, filepath, context, **options)

class InibinReader(AbstractReader):
    """
    An inibin-reader
    """
    @classmethod
    def is_my_file(cls, fistream, reset=True):
        """
        Checks if the fistream describes an inibin-object
        """
        is_my = False
        length = tell_f_length(fistream)
        if length > 5:
            read = fistream.read(1)
            if read == b'\x02':
                is_my = True
            if reset:
                seek(fistream, -1, 1)
        return is_my

    @classmethod
    def _flags_in_order(cls):
        """
        Yields all possible flags in order meaning all one-bit-set shorts
        """
        flag = 0b1
        while flag <= 0x8000:
            yield flag
            flag <<= 1

    def __init__(self, filter_func=DEFAULT_FILTER):
        """
        Constructor
        """
        self._data = None
        self.filter_func = (filter_func, DEFAULT_FILTER)[filter_func is None]

    def _handle_flag(self, flag, fistream, data):
        """
        Handles a specific flag description with the given fistream
        """
        # Read count of and keys
        unpack_func = TYPEFLAG_TABLE[flag]
        flag_type = TYPE_OF_FLAG[unpack_func]
        count = unpack("<H", fistream)
        keys = unpack("<%si" % count, fistream)
        try:
            read_items = unpack_func(count, fistream, data)
            return [i for i in zip(keys, ((item, flag_type) for item in read_items))
                        if self.filter_func(i[0], flag_type)
                    ]
        except NotImplementedError:
            pass
        return []

    def __str__(self):
        """A nice representation of this object"""
        return "InibinImporter:\n%s" % self._data

    def print_data(self, sel_filter_func=default_sel_filter_func,
                         format_func=default_format_func,
                         sort_func=default_sort_func,
                         stream=None):
        """
        Outputs the data carried with the InibinReader in a human-readable format
        """
        print(self._data.str_data(sel_filter_func, format_func, sort_func), file=stream)

    def read_from_file(self, fistream):
        """
        Actually reads the from the fistream
        fistream: the file to read from
        [filter_func]: a function with three input parameters(enum mode, int key, type key_type)
            - a mode -> get the right KEYTABLE with KEYTABLES#mode_to_keytable(mode)
            - a key value -> KEYTABLES#KEY_TABLE
            - a key_type like INT, SHORT, often not used at all
        """
        data = InibinData()
        errs = []
        data.header.unpack(fistream)
        for flag in InibinReader._flags_in_order():
            if not flag & data.header.set_type_flags:
                # flag not in file
                continue
            new_mappings = self._handle_flag(flag, fistream, data)
            for mapping in new_mappings :
                key, value = mapping
                if key in data.data_map:
                    errs.append("Key '%s' found multiple times" % key)
                data.data_map[key] = value
        self._data = data
        return errs

    def to_scene(self, d__context):
        """
        Imports the data into Blender. Doing nothing so far
        # TODO: InibinReader.to_scene: decide if this should cause changes in Blender
        """
        pass
