"""
Created on 30.03.2014

@author: Carbon
"""
from .. import util
from ..util import AbstractReader, MODE_FILE, MODE_INTERNAL, length_check, seek, \
    unpack
from .AnmData import AnmBone, AnmData, AnmHeader, AnmTransformation

def import_from_file(filepath, context, **options):
    """
    Imports an animation from the file into blender
    """
    return util.read_from_file(AnmReader, filepath, context, **options)

class AnmReader(AbstractReader):
    """
    Convenience class that handles the import
    """

    @classmethod
    def is_my_file(cls, fistream, reset=True):
        """
        Reads from the fistream and tells if the stream (supposedly) describes an animation file
        fistream - the stream to read from
        reset - whether to reset the filestream to the original position
        """
        is_anm = False
        if fistream.read(8) == b'r3d2anmd':
            is_anm = True
            # if version 4
            if unpack("<I", fistream) == 4:
                if unpack("<4xI", fistream) == 0xBE0794D3:
                    is_anm = True
                if reset:
                    seek(fistream, -8, 1)
            if reset:
                seek(fistream, -4, 1)
        if reset:
            seek(fistream, -8, 1)
        return is_anm

    def __init__(self):
        """
        Constructor
        """
        self._data = None

    def __str__(self):
        """A nice representation of this object"""
        return "AnmImporter:\n%s" % str(self._data)

    def read_from_file(self, file_path):
        """
        Reads an animation from a file into this handler.
        Doesn't manipulate any scene/object
        """
        warns = []
        fistream = open(file_path, 'rb')
        # get length of file we read
        f_length = util.tell_f_length(fistream, prefix="AnmReader")
        length_check(12, f_length, "AnmReader")
        fistream.read(8) # == b'r3d2anmd'
        version = unpack("<I", fistream)
        if version == 3:
            length_check(28, f_length, "AnmReader")
            warns = self._read_version3(fistream, f_length)
        elif version == 4:
            length_check(64, f_length, "AnmReader")
            warns = self._read_version4(fistream, f_length)
        else:
            raise NotImplementedError("AnmReader: Only file-versions 3 and 4 are currently implemented. Report this error to WorldSEnder.")
        fistream.close()
        return warns

    def _read_version3(self, fistream, f_length):
        """
        Reads a version 3 file
        """
        data = AnmData(MODE_FILE)
        data.version = 3
        (# designerId,
        num_bones, num_frames, fps) = unpack("<4x2If", fistream)
        if fps < 0.0: # the odds of the file....
            fps += 4294967296.0
        min_length = 28 + num_bones * AnmBone.kHeaderSize + num_bones * num_frames * AnmTransformation.kSizeInFile
        length_check(min_length, f_length, "AnmReader")
#         raise TypeError("File is too short (%s bytes). Expected " \
#                         "with %s bones and %s frames: %s bytes" % (f_length, num_bones, num_frames, min_length))
        for _ in range(num_bones):
            bone = AnmBone()
            name_bytes, mode = util.split(unpack("<32BI", fistream), 32)
            bone.name = str(bytes(name_bytes), encoding="ISO-8859-1").rstrip('\x00')
            bone.is_root = mode == 2
            for _ in range(num_frames):
                rot, loc = util.split(unpack("<7f", fistream), 4)
                bone.poses.append(AnmTransformation(rot, loc))
            # all frames
            data.bones.append(bone)
        # all bones
        self._data = data
        return []

    def _read_version4(self, fistream, f_length):
        """
        Reads a file of version 4
        """
        data = AnmData(MODE_FILE)
        warns = []
        data.version = 4
        pos = fistream.tell()
        header = AnmHeader().unpack(fistream)
        length_check(12 + header.data_length, f_length, "AnmReader:")
        # no need for that is already checked
        # if magic_nbr != 0xD39407BE:
        #     raise TypeError("AnmReader: Invalid magic number")
        data.num_frames = header.num_frames
        data.fps = (1 / header.fps) if header.fps < 1 else (header.fps)
        num_pos = (header.quat_off - header.pos_off) // 12
        num_quat = (header.frame_off - header.quat_off) // 16
        seek(fistream, pos + header.pos_off, 0) # place cursor to positions
        positions = util.split(unpack('<%sf' % (num_pos * 3), fistream), *[3 * i for i in range(1, num_pos)])
        quats = util.split(unpack('<%sf' % (num_quat * 4), fistream), *[4 * i for i in range(1, num_quat)])
        # init bones
        data.bones = [AnmBone() for _ in  range(header.num_bones)]
        for _ in range(header.num_frames):
            for i, bone in enumerate(data.bones):
                name_hash, pos_id, quat_id = unpack("<Ihxxhxx", fistream)
                if not bone.name_hash:
                    bone.name_hash = name_hash
                elif bone.name_hash != name_hash:
                    warns.append("AnmReader: The file does not contain valid information."
                                " The same bone (nbr. %d) occurs with different name hashes (%s and %s)"
                                % (i, bone.name_hash, name_hash))
                bone.poses.append(AnmTransformation(positions[pos_id], quats[quat_id]))
            # all bones
        # all frames
        self._data = data
        return warns

    def to_scene(self, d__context):
        """
        Sets up the scene with the previously read data
        """
        self._data.switch_mode(MODE_INTERNAL)
        # TODO: anm.to_scene
        raise NotImplementedError()
