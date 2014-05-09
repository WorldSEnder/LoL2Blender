"""
Created on 03.05.2014

@author: Carbon
"""
from ..util import AbstractReader, MODE_FILE, length_check, read_from_file, seek, \
    tell_f_length, unpack
from .SknData import SknData, SknMaterial, SknVtx

def import_from_file(filepath, context, **options):
    """
    Imports a mesh from the given filepath
    """
    return read_from_file(SknReader, filepath, context, **options)

class SknReader(AbstractReader):
    """
    classdocs
    """
    @classmethod
    def is_my_file(cls, fistream, reset=True):
        """
        Reads from the fistream and tells if the stream (supposedly) describes an animation file
        fistream - the stream to read from
        reset - whether to reset the filestream to the original position
        """
        is_skn = False
        magic = unpack("<I", fistream)[0]
        if magic == 0x00112233:
            is_skn = True
        if reset:
            seek(fistream, -4, 1)
        return is_skn

    def __init__(self):
        """
        Constructor
        """
        self._data = None

    def __str__(self):
        """A nice representation of this object"""
        return "SknImporter:\n%s" % self._data

    def read_from_file(self, fistream):
        """
        Reads the model from the given fistream
        """
        warns = []
        f_length = tell_f_length(fistream, "SknReader")
        length_check(8, f_length, "SknReader")
        (# magic,
        version, nbr_objects) = unpack("<4x2H", fistream)
        if version not in (1, 2):
            raise NotImplementedError("Skn-version %s is currently not supported. Report this to WorldSEnder" % version)
        if nbr_objects != 1:
            raise NotImplementedError("More than one object per file is currently not supported. Report this to WorldSEnder")
        if version == 1:
            warns = self._read_version12(fistream, f_length, False)
        elif version == 2:
            warns = self._read_version12(fistream, f_length, True)
        return warns

    def _read_version12(self, fistream, f_length, version2=True):
        """
        Reads both version 1 and 2 depending on data.version
        """
        data = SknData(MODE_FILE)
        warns = []
        min_length = 20 + 12 if version2 else 0
        length_check(min_length, f_length, "SknReader")
        num_materials = unpack("<I", fistream)[0]
        min_length += num_materials * SknMaterial.kSizeInFile
        length_check(min_length , f_length, "SknReader")
        data.materials = [SknMaterial().unpack(fistream) for _ in range(num_materials)]
        num_indices, num_verts = unpack("<2I", fistream)
        if num_indices % 3:
            raise TypeError("Invalid number of indices: %s Not divisible by 3." % num_indices)
        min_length += num_indices * 2 + num_verts * SknVtx.kSizeInFile
        length_check(min_length, f_length, "SknReader")
        for face_nbr in range(num_indices // 3):
            indx1, indx2, indx3 = unpack("<3H", fistream)
            if (indx1 == indx2 or
                indx1 == indx3 or
                indx2 == indx3):
                warns.append("SknReader: input mesh has a badly built face (nbr. %s)."
                        "Indx1 %s, indx2 %s, indx3 %s" % (face_nbr, indx1, indx2, indx3))
            wrong_indices = [i for i in [indx1, indx2, indx3] if i >= num_indices]
            for wrong_indx in wrong_indices:
                warns.append("SknReader: Face nbr. %s -> Index out of bounds: %s" % (face_nbr, wrong_indx))
            if not len(wrong_indices):
                data.indices += [indx1, indx2, indx3]
        data.vertices = [SknVtx().unpack(fistream) for _ in range(num_verts)]
        if version2:
            data.version = 2
            data.end_tab = fistream.read(12) # something
        else:
            data.version = 1
        self._data = data
        return warns

    def to_scene(self, d__context):
        """
        Updates the scene to fit the read data
        """
        self._data.switch_to_blend_mode()
        # TODO: SklImporter.to_scene
        raise NotImplementedError()