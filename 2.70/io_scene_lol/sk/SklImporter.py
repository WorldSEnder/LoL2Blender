"""
Created on 01.04.2014

@author: Carbon
"""
from ..util import AbstractReader, MODE_FILE, length_check, read_from_file, seek, \
    tell_f_length, unpack
from .SklData import RawHeader, SklBone, SklData
import re

def import_from_file(filepath, context, **options):
    """
    Imports a skeleton from the given filepath
    """
    return read_from_file(SklReader, filepath, context, **options)

class SklReader(AbstractReader):
    """
    Imports a skeleton from a file
    """
    @classmethod
    def is_my_file(cls, fistream, reset=True):
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
        elif first8[4:] == b'\xC3\x4F\xFD\x22':
            is_skl = True
        if reset:
            seek(fistream, -8, 1)
        return is_skl

    def __init__(self):
        """
        Constructor
        """
        self._data = None

    def __str__(self):
        """A nice representation of this object"""
        return "SklImporter:\n%s" % self._data

    def read_from_file(self, fistream):
        """
        Reads skl-data from the file.
        fistream - the stream to read from
        """
        f_length = tell_f_length(fistream, prefix="SklReader")
        length_check(20, f_length, "SklReader")
        byts = fistream.read(8)
        if byts == "r3d2sklt".encode():
            self._read_version12(fistream, f_length)
        else:
            seek(fistream, -8, 1)
            self._read_version3(fistream, f_length)
        return [] # no warnings

    def _read_version12(self, fistream, f_length):
        """
        Internal reader for file version 1&2
        """
        data = SklData(MODE_FILE)
        (version,
         # designer_id,
         num_bones) = unpack("<I4xI", fistream)
        if version not in (1, 2):
            raise TypeError("SklReader: Version %s not supported. Report this to WorldSEnder" % version)
        data.version = version
        min_length = 20 + num_bones * SklBone.kSizeInFile + (4 if version == 2 else 0)
        length_check(min_length, f_length, "SklReader")
        # they appear in order, no need to worry
        data.bones = [a for _, a in (SklBone().unpack(fistream) for _ in range(num_bones))]
        if version == 2:
            num_indices = unpack("<I", fistream)
            min_length += num_indices * 4
            length_check(min_length, f_length, "SklReader")
            # if num_indices > data_.max_indices: raise TypeError("") # removed because cass fails the check
            data.skn_indices = list(unpack("<%sI" % num_indices, fistream))
        else:
            data.skn_indices = [i for i in range(num_bones)]
        self._data = data

    def _read_version3(self, fistream, f_length):
        """
        Internal reader for file version 3
        """
        data = SklData(MODE_FILE)
        data.version = 3
        pos = fistream.tell()
        header = RawHeader().unpack(fistream)
        # already checked
        # if header.magic != 0xC34FFD22:
        #     raise TypeError("SklReader: Stream doesn't describe a skl-file, magic is wrong")
        length_check(header.size, f_length, "SklReader:")
        data.bones = [None] * header.nbr_skl_bones
        for _ in range(header.nbr_skl_bones):
            bone_id, bone = SklBone().unpack(fistream, True)
            data.bones[bone_id] = bone
        seek(fistream, pos + header.offset_bone_names, 0)
        # because this is the last in file we can read as far as we want
        ensured_string = fistream.read(SklBone.kNameLength * header.nbr_skl_bones).decode()
        # the regex matches multiple, 4-byte charsequences without \0-byte followed by a sequence with one or more \0-bytes
        bin_names = re.findall('(([^\x00]{4})*(\x00...|.\x00..|..\x00.|...\x00))', ensured_string)
        # the first matched group (everything) is taken and the leftmost splutter of split(\0) is taken as the bone's name
        names = [bname[0].split('\x00')[0] for bname in bin_names]
        for bone, name in zip(data.bones, names):
            bone.name = name
        seek(fistream, pos + header.offset_skn_indices, 0)
        data.skn_indices = list(unpack("<%sH" % header.nbr_anm_bones, fistream))
        self._data = data

    def to_scene(self, d__context):
        """
        Updates the scene to fit the read data
        """
        self._data.switch_to_blend_mode()
        # TODO: SklImporter.to_scene
        raise NotImplementedError()
