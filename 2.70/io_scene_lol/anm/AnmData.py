"""
Created on 29.03.2014

@author: Carbon
"""
from ..util import MODE_FILE, MODE_INTERNAL, unpack

class AnmHeader(object): # pylint: disable=too-few-public-methods
    """
    A header for animations of version 4
    """
    def __init__(self):
        """Constructor"""
        self.data_length = 0
        self.num_bones = 0
        self.num_frames = 0
        self.fps = 0
        self.pos_off = 0
        self.quat_off = 0
        self.frame_off = 0

    def unpack(self, fistream):
        """
        Unpacks the header from the given datastream
        """
        (self.data_length,
        # magic_number
        # 8 unused bytes
        self.num_bones, self.num_frames, self.fps,
        self.pos_off, self.quat_off, self.frame_off) = unpack('<I12x2If12x3I', fistream)

class AnmTransformation(object): # pylint: disable=too-few-public-methods
    """
    represents a transformation
    """
    kSizeInFile = 0x1C

    def __init__(self, rot=(), loc=()):
        """
        Constructor
        rot - a 4-dim vector as rotation quaternions
        loc - a 3-dim vector as x,y,z coord
        """
        # sorted((0, len(rot), 4|3))[1] clamps the length inside 0 and 4|3
        self.rot = [0.0] * 4
        self.rot[:sorted((0, len(rot), 4))[1]] = rot[:4]
        self.loc = [0.0] * 3
        self.loc[:sorted((0, len(loc), 3))[1]] = loc[:3]

class AnmBone(object): # pylint: disable=too-few-public-methods
    """
    represents a bone
    """
    kHeaderSize = 0x24
    kNameLen = 0x20

    def __init__(self, is_root=True, name=''):
        """
        Constructor
        """
        self.is_root = is_root
        self.name = name
        self.poses = []
        self.name_hash = 0

class AnmData(object):
    """"
    Represents full animation data
    """
    def __init__(self, initMode=MODE_INTERNAL):
        """
        Constructor
        """
        self.version = -1
        self.num_frames = 0
        self.fps = 0.0
        self.bones = []
        self._mode = initMode

    def switch_to_blend_mode(self):
        """
        Switches to internal mode
        """
        if self._mode == MODE_INTERNAL:
            return
#             for bone in self.bones:
#                 for transform in bone.poses:
#                     transform.rot[1] = -transform.rot[1]
#                     transform.rot[2] = -transform.rot[2]
#                     transform.x = -transform.x
        # TODO: whatever is necessary to adjust Blender <-> LoL conversion

    def switch_to_file_mode(self):
        """
        Switches to file/external mode
        """
        if self._mode == MODE_FILE:
            return
        # TODO: whatever is necessary to adjust Blender <-> LoL conversion

    def __str__(self):
        """
        Returns the data in a nice format
        """
        rstr = "Version: %s, fps: %s, Nbr. of bones: %s, Framecount: %s" % (self.version, self.fps,
                                                                                 len(self.bones), self.num_frames)
        for i, bone in enumerate(self.bones):
            rstr += ("\nAnmBone Nbr: %s, %s: %s, isRoot: %s" % ((i, "name", bone.name, bone.is_root)
                            if self.version == 3 else (i, "namehash", bone.name_hash, bone.is_root)))
            for j, pose in enumerate(bone.poses):
                rstr += '\n    Frame %s: Pos %s Rot %s' % (j, pose.loc, pose.rot)
            # all frames
        # all bones
        return rstr
