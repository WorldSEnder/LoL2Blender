"""
Created on 31.03.2014

@author: Carbon
"""
from ..util import MODE_FILE, MODE_INTERNAL

class SknMaterial(object): # pylint: disable=too-few-public-methods
    """
    describes a texture used by skn
    """
    kSizeInFile = 0x50
    kNameLen = 0x40

    def __init__(self, name=""):
        """
        Constructor
        """
        self.name = name
        self.start_vertex = 0
        self.num_vertices = 0
        self.start_index = 0
        self.num_indices = 0

class SknVtx(object): # pylint: disable=too-few-public-methods
    """
    an sknVertex. You know, the things making up a model
    """
    def __init__(self):
        """
        Constructor
        """
        self.pos = [0.0] * 3
        self.skn_indices = [-1] * 4
        self.weights = [0.0] * 4
        self.normal = [0.0] * 3
        self.uv = [0] * 2
#         int skl_indices[4];
#         int uv_index;
#         int dupe_data_index;

class SknData(object): # pylint: disable=too-few-public-methods
    """
    contains all the data of one mesh
    """
    def __init__(self, mode=MODE_INTERNAL):
        """
        Set the mode to file if you'll read from a file and to internal if you init from blender
        """
        self._mode = mode
        self.version = -1
        self.num_vtxs = 0
        self.num_indices = 0
        self.num_final_vtxs = 0
        self.indices = []
        self.materials = []
        self.vertices = []
        self.end_tab = None # some trash data whatever

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
