"""
Created on 01.04.2014

@author: Carbon
"""
from ..util import MODE_FILE, seek
from .SklData import SklData
from os import path

def import_from_file(filepath, context, **options):
    """
    Imports a skeleton from the file into blender
    """
    if path.isfile(filepath):
        file = open(filepath, mode='rb')
        reader = SklReader()
        if not SklReader.is_skl_file(file):
            file.close()
            return {'CANCELLED'}
        reader.read_from_file(file)
        try:
            if options["dumpData"]:
                # this is debug
                # pylint: disable=protected-access
                steam = None
                try:
                    steam = options["ostream"]
                except KeyError:
                    pass
                reader._data.dump_data(stream=steam)
        except KeyError:
            pass # do nothing
        return reader.to_scene(context)

class SklReader(object):
    """
    Imports a skeleton from a file
    """
    @classmethod
    def is_skl_file(cls, fistream, reset=True):
        """
        Reads from the fistream and tells if the stream (supposedly) describes an animation file
        fistream - the stream to read from
        reset - whether to reset the filestream to the original position
        """
        is_skl = False
        first8 = fistream.read(8)
        if first8 == b'r3d2sklt':
            bit8 = fistream.read(1)
            if bit8 in (1, 2):
                is_skl = True
            if reset:
                seek(fistream, -1, 1)
        elif int(first8[4:]) = 
        if reset:
            seek(fistream, -8, 1)
        return is_skl

    def __init__(self):
        """
        Constructor
        """
        self._data = SklData(MODE_FILE)

    def read_from_file(self, fistream):
        """
        Reads skl-data from the file.
        fistream - the stream to read from
        """

    def to_scene(self, context):
        """
        Updates the scene to fit the read data
        """
