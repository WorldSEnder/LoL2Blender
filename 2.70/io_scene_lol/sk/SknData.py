"""
Created on 31.03.2014

@author: Carbon
"""
from ..util import MODE_FILE, MODE_INTERNAL, split, unpack

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

    def __str__(self):
        """A nice representation is always nice"""
        return "SknMaterial: Name: %s, Verts-start: %s, Vertsnum: %s, Indices-start: %s, Indicesnum: %s" % \
            (self.name, self.start_vertex, self.num_vertices, self.start_index, self.num_indices)

    def unpack(self, fistream):
        """
        Reads the material from the input stream
        """
        self.name = fistream.read(SknMaterial.kNameLen).decode("latin-1").split('\x00')[0]
        (self.start_vertex, self.num_vertices,
         self.start_index, self.num_indices) = unpack("<4I", fistream)
        return self

class SknVtx(object): # pylint: disable=too-few-public-methods
    """
    an sknVertex. You know, the things making up a model
    """
    kSizeInFile = 0x34
    def __init__(self):
        """
        Constructor
        """
        self.pos = [0.0] * 3
        self.skn_indices = [-1] * 4 # char
        self.weights = [0.0] * 4
        self.normal = [0.0] * 3
        self.uv = [0] * 2
#         int skl_indices[4];
#         int uv_index;
#         int dupe_data_index;

    def __str__(self):
        """Such beauty, much nice"""
        return "Pos: %s, Uv: %s, Normal: %s, Bone-Bindings: %s" % \
            (self.pos, self.uv, self.normal, list(zip(self.skn_indices, self.weights)))

    def unpack(self, fistream):
        """
        Reads a single vertex from the file
        """
        (self.pos, self.skn_indices,
         self.weights, self.normal, self.uv) = split(unpack("<3f4B9f", fistream), 3, 7, 11, 14)
        return self

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
        # no need for the following in python
        # self.num_vtxs = 0
        # self.num_indices = 0
        self.faces = []
        self.materials = []
        self.vertices = []
        self.end_tab = b'' # some trash data whatever/debug

    def __str__(self):
        """Beautiful representation"""
        rstr = "SknData:\nVersion %s, %s materials, %s vertices, %s faces" % \
                (self.version, len(self.materials), len(self.vertices), len(self.faces))
        for i, mat in enumerate(self.materials):
            rstr += "\n    Material #%s: %s" % (i, mat)
        for i, vert in enumerate(self.vertices):
            rstr += "\n    Vertex #%s: %s" % (i, vert)
        for i, face in enumerate(face):
            rstr += "\n    Face #%s: %s" % (i, face)
        if self.version == 2:
            rstr += "\n    Some bytes: %s" % (self.end_tab,)
        return rstr

    def switch_mode(self, mode=None):
        """
        Switches to internal mode
        mode: the mode to switch to.
            If None given just swaps to the other
        """
        if self._mode == mode:
            return
        for vert in self.vertices:
            vert.uv = (vert.uv[0], 1 - vert.uv[1]) # Swap y-co
        # TODO: whatever is necessary to adjust Blender <-> LoL conversion
