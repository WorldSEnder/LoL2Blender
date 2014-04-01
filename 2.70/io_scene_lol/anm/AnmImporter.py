"""
Created on 30.03.2014

@author: Carbon
"""
from ..util import split
from .AnmData import AnmBone, AnmData, AnmTrans, MODE_FILE
from io_scene_lol.util import seek
from os import path
import os
import struct

def read_from_file(filepath, context, **options):
    """
    Imports an animation from the file into blender
    """
    if path.isfile(filepath):
        file = open(filepath, mode='rb')
        reader = AnmReader()
        if not AnmReader.is_anm_file(file):
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

class AnmReader(object):
    """
    Convenience class that handles the import
    """

    @classmethod
    def is_anm_file(cls, fistream, reset=True):
        """
        Reads from the fistream and tells if the stream (supposedly) describes an animation file
        fistream - the stream to read from
        reset - whether to reset the filestream to the original position
        """
        is_anm = False
        if fistream.read(8) == b'r3d2anmd':
            is_anm = True
        if reset:
            seek(fistream, -8, 1)
        return is_anm

    def __init__(self):
        """
        Constructor
        """
        self._data = AnmData(MODE_FILE)

    def read_from_file(self, fistream):
        """
        Reads an animation from a file into this handler. Doesn't manipulate any scene/object
        """
        # get length of file we read
        f_length = os.stat(fistream.name).st_size
        if f_length < 28: # length of file header
            raise TypeError("File is too short for an animation file")
        if not AnmReader.is_anm_file(fistream, False):
            raise TypeError("Magic number of file incorrect. No animation file")
        version = struct.unpack("<I", fistream.read(4))[0]
        if version == 3:
            self._read_version3(fistream, f_length)
        elif version == 4:
            self._read_version4(fistream, f_length)
        else:
            raise NotImplementedError("Only file-versions 3 and 4 are currently implemented. Report this error to WorldSEnder.")

    def _read_version3(self, fistream, f_length):
        """
        Reads a version 3 file
        """
        self._data.version = 3
        # _ = designer Id
        _, num_bones, num_frames, fps = struct.unpack("<3If", fistream.read(16))
        if fps < 0.0: # the odds of the file....
            fps += 4294967296.0
        # add to minlength
        min_length = 28 + num_bones * AnmBone.kHeaderSize + num_bones * num_frames * AnmTrans.kSizeInFile
        if f_length < min_length:
            raise TypeError("File is too short (%s bytes). Expected " \
                            "with %s bones and %s frames: %s bytes" % (f_length, num_bones, num_frames, min_length))
        for _ in range(num_bones):
            bone = AnmBone()
            name_bytes, mode = split(struct.unpack("<32BI", fistream.read(AnmBone.kHeaderSize), 32))
            bone.name = bytes(name_bytes).rstrip('\x00').decode("ISO-8859-1")
            bone.is_root = mode == 2
            for _ in range(num_frames):
                rot, loc = split(struct.unpack("<7f", fistream.read(28)), 4)
                bone.poses.append(AnmTrans(rot, loc))
            # all frames
            self._data.bones.append(bone)
        # all bones

    def _read_version4(self, fistream, f_length):
        """
        Reads a file of version 4
        """
        self._data.version = 4
        min_length = 12 + struct.unpack('<I', fistream.read(4))[0]
        if f_length < min_length:
            raise TypeError("File is too short (Read filelength: %s)" % min_length + 12)
        magic_nbr = struct.unpack(">I", fistream.read(4))[0]
        if magic_nbr != 0xD39407BE:
            raise TypeError("Invalid magic number")
        seek(fistream, 8, 1) # 8 unused bytes, possibly was thought to be used as num_pos, num_quat or something
        num_bones, num_frames, fps = struct.unpack('<2If', fistream.read(12))
        if fps < 1:
            fps = 1 / fps
        self._data.num_frames = num_frames
        self._data.fps = fps
        seek(fistream, 12, 1) # 12 unused bytes
        pos_off, quat_off, frame_off = struct.unpack('<3I', fistream.read(12))
        num_pos = (quat_off - pos_off) // 12
        num_quat = (frame_off - quat_off) // 16
        seek(fistream, pos_off + 12) # place cursor to positions
        positions = split(struct.unpack('<%sf' % (num_pos * 3), fistream.read(12 * num_pos)), *[3 * i for i in range(1, num_pos)])
        quats = split(struct.unpack('<%sf' % (num_quat * 4), fistream.read(16 * num_quat)), *[4 * i for i in range(1, num_quat)])
        # init bones
        self._data.bones = [AnmBone() for _ in  range(num_bones)]
        if num_frames: # if there are any frames then we take the first one to set name hashes
            for bone in self._data.bones:
                name_hash, pos_id, quat_id = struct.unpack("<Ihxxhxx", fistream.read(12))
                bone.name_hash = name_hash
                bone.poses.append(AnmTrans(positions[pos_id], quats[quat_id]))
        for _ in range(num_frames - 1):
            for bone in self._data.bones:
                name_hash, pos_id, quat_id = struct.unpack("<Ihxxhxx", fistream.read(12))
                if bone.name_hash != name_hash:
                    raise TypeError('The file does not contain valid information. The same bone occurs with different name hashes.')
                bone.poses.append(AnmTrans(positions[pos_id], quats[quat_id]))
            # all bones
        # all frames

    def to_scene(self, d__context):
        """
        Sets up the scene with the previously read data
        """
        self._data.switch_to_blend_mode()
        raise NotImplementedError()
