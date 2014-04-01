"""
Created on 29.03.2014

@author: Carbon
"""
from ..util import MODE_FILE, MODE_INTERNAL

class AnmTrans(object): # pylint: disable=too-few-public-methods
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

    def dump_data(self, stream=None):
        """
        Prints the data to the console
        """
        print('Version: %s, fps: %s, bonecount: %s, frames: %s' % (self.version, self.fps, len(self.bones), self.num_frames), file=stream)
        bone_caption = 'Bone Nbr: %s, {0}: %s, isRoot: %s'.format('name' if self.version == 3 else 'namehash')
        for i, bone in enumerate(self.bones):
            print(bone_caption % (i, bone.name if self.version == 3 else bone.name_hash, bone.is_root), file=stream)
            for j, pose in enumerate(bone.poses):
                print('    Frame %s: Pos %s Rot %s' % (j, pose.loc, pose.rot), file=stream)
            # all frames
        stream.flush()
        # all bones
