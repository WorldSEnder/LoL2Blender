"""
Created on 31.03.2014

@author: Carbon
"""
from ..util import MODE_FILE, MODE_INTERNAL

class SklBone(object): # pylint: disable=too-few-public-methods
    """
    represents a bone in the skeleton
    """
    kNameLength = 0x20
    kSizeWithoutMatrix = 0x28
    kSizeInFile = 0x58

    def __init__(self, name=""):
        """
        Constructor
        """
        self.name = name
        self.parent = -1
        self.scale = 0.1
        self.transform = [[0.0] * 4 for _ in range(4)]

class SklData(object): # pylint: disable=too-few-public-methods
    """
    Represents a read-in state of a skeleton
    """
    kMaxBones = 0x44
    def __init__(self, mode=MODE_INTERNAL):
        """
        Constructor
        """
        self._mode = mode
        self.version = 0
        self.num_bones = 0
        self.bones = []
        self.num_indices = 0
        self.skn_indices = []
        self.bones_internal = []

    def switch_to_blend_mode(self):
        """
        Switches to internal mode
        """
        if self._mode == MODE_INTERNAL:
            return
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
        Dumps the currently read data with print(###, file=stream)
        """
        # TODO: dump data
        pass
