"""
Created on 31.03.2014

@author: Carbon
"""
from ..util import MODE_INTERNAL
from .SklData import SklData
from .SknData import SknData
from os import path

def read_from_file(filepath_skn, filepath_skl, context, **options):
    """
    Writes the model to the specified location
    """
    if path.isfile(filepath_skn) and path.isfile(filepath_skl):
        file_skn = open(filepath_skn, mode='wb')
        file_skl = open(filepath_skl, mode='wb')
        writer = SkWriter()
        writer.from_context(context)
        try:
            if options["dumpData"]:
                # this is debug
                # pylint: disable=protected-access
                stream = None
                try:
                    stream = options["ostream"]
                except KeyError:
                    pass
                writer._data_skn.dump_data(stream=stream)
                writer._data_skl.dump_data(stream=stream)
        except KeyError:
            pass # do nothing
        writer.write_to_files(file_skn, file_skl)
        file_skn.flush()
        file_skn.close()
        file_skl.flush()
        file_skl.close()
        return {'FINISHED'}
    else:
        return {'CANCELLED'}

class SkWriter(object):
    """
    Reads data from a scene and writes it to a file
    """
    def __init__(self):
        """
        Constructor
        """
        self._data_skn = SknData(MODE_INTERNAL)
        self._data_skn.version = 1
        self._data_skl = SklData(MODE_INTERNAL)
        self._data_skl.version = 2

    def from_context(self, context):
        """
        Inititalizes the writer from the context given (normally bpy.context)
        """
        obj = context.object
        if obj.type != 'MESH':
            raise TypeError("Selected object is not a mesh.")
        # search for an armature modifier
        armature = None
        for mod in obj.modifiers:
            if mod.type == 'ARMATURE' and mod.object is not None:
                armature = mod.object
                break
        # all mods searched
        if armature is None:
            raise TypeError("Selected object does not have an armature bound to it.")


    def write_to_files(self, fostream_skn, fostream_skl):
        """
        Writes the data to the given outputstream
        """
        pass
