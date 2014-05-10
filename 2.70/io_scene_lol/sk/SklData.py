"""
Created on 31.03.2014

@author: Carbon
"""
from ..util import MODE_FILE, MODE_INTERNAL, quats_to_matrix, split, unpack
from .util import hash_name
from sys import stdout

class SklBone(object): # pylint: disable=too-few-public-methods
    """
    represents a bone in the skeleton
    """
    kNameLength = 0x20
    kSizeWithoutMatrix = 0x28
    kSizeInFile = 0x58

    def __init__(self):
        """
        Constructor
        """
        self.name = ""
        self.namehash = 0
        self.parent = -1
        self.scale = 0.1
        self.transform = [[0.0] * 4 for _ in range(4)]
        self.transform[3][3] = 1.0

    def __str__(self):
        """A good-looking representation of the bone"""
        rstr = "namehash: %s, Name: %s" % (self.namehash, self.name)
        return rstr + ", Parent: %s, Scale: %s, Transformation: %s" % (self.parent, self.scale, self.transform)

    def unpack(self, fistream, raw=False):
        """
        Reads this bone from the inputstream
        """
        own_id = -1 # where to put the bone in the list
        if raw:
            (# uk,              # short    # ??, always \x00
             own_id,            # short    # id of this bone
             parent_id,         # short    # id of parent bone
             # uk2,             # short    # ??, always \x00
             self.namehash,     # int      # namehash
             # unused,          # float    # ?? always \x66\x66\x06\x40
             t_x, t_y, t_z,     # float[3] # translation
             # d__1, d__2, d__3,# float[3] # ?? [~1.0] * 3
             q_x, q_y, q_z, q_w,# float[4] # quaternions
             # ctx, cty, ctz    # float[3] # ??
             # d__1, d__2, d__3,# float[3] # ?? [~1.0] * 3
             # cqx, cqy, cqz, cqw,# float[4] # ?? quaternions
             # some_offset      # int      # ??
                                # 0x8 + 0x8 + 0x15 * 4 = 0x64 bytes
             ) = unpack("<2x2H2xI4x3f12x4f44x", fistream)
            self.parent = -1 if parent_id == 0xFFFF else parent_id
            self.transform = quats_to_matrix(q_x, q_y, q_z, q_w, t_x, t_y, t_z)
        else:
            self.name = fistream.read(SklBone.kNameLength).decode("latin-1").split('\x00')[0]
            self.namehash = hash_name(self.name)
            self.parent, self.scale = unpack("<if", fistream)
            major_col = split(unpack("12f", fistream), 4, 8)
            self.transform = [list(a) + [0.0] for a in zip(*major_col)]
            self.transform[3][3] = 1.0
        return own_id, self

class SklData(object): # pylint: disable=too-few-public-methods
    """
    Represents a read-in state of a skeleton
    """
    kMaxBones = 0x44
    def __init__(self, mode=MODE_INTERNAL):
        """
        Constructor
        """
        # MODE_INTERNAL, MODE_FILE
        self._mode = mode
        # 1, 2 or 3
        self.version = 0
        # all bones, type SklBone
        self.bones = []
        self.skn_indices = []

    def __str__(self):
        """Beautiful, just beautiful"""
        rstr = "SklData: version %s, %s bones, %s animated bones" % (self.version, len(self.bones), len(self.skn_indices))
        for i, bone in enumerate(self.bones):
            rstr += "\nBone #%s%s: %s" % (i, "(animated)" if i in self.skn_indices else "", bone)
        return rstr

    def switch_mode(self, mode=None):
        """
        Switches to internal mode
        """
        if self._mode == mode:
            return
        # Whatever is necessary for
        # Blender <==> File

class RawHeader(object): # pylint: disable=too-many-instance-attributes
    """
    A version 3 file-header
    """
    def __init__(self):
        """
        Constructor, defaults values
        """
        self.size = 0               # int   # filesize?
        self.magic = 0              # int   # magicnumber
        # self.unk = 0              # int   # ?? padding
        # self.unk2 = 0             # short # ?? padding
        self.nbr_skl_bones = 0      # short # number of bones in file
        self.nbr_anm_bones = 0      # short # number of animated bones
        # self.uk2                  # short # padding
        self.header_size = 0x40     # int   # size, always 0x40?
        self.offset_namehashes = 0  # int   # namehashes in file... again
        self.offset_skn_indices = 0 # int   # animated bones
        self.offset_ukn = 0         # int   # ?? offset to smth before bone_names with \x00\x00\x00\x00
        self.offset_ukn2 = 0        # int   # ?? offset to smth before bone_names with \x00\x00\x00\x00
        self.offset_bone_names = 0  # int   # names are SklBone.kNameLength max or \x00-terminated and padded to 4 bytes
                                    # 44 bytes + 20 padding = 64 bytes
    def unpack(self, fistream):
        """
        Reads the header from the input-stream
        """
        (self.size, self.magic, self.nbr_skl_bones, self.nbr_anm_bones, self.header_size,
         self.offset_namehashes, self.offset_skn_indices, self.offset_ukn, self.offset_ukn2,
         self.offset_bone_names) = unpack("<2I6x2H2x6I", fistream) # 8I + 8x + 2H = 44bytes
        return self

    def dump_data(self, line_prefix="", ostream=stdout):
        """
        Dumps the data into the given stream
        """
        # TODO: print the data
        print(line_prefix + "Filesize: %s Headersize: %s, Nbr. of bones: %s, Nbr. of animated bones: %s" % (self.size,
                                                                                                            self.header_size,
                                                                                                            self.nbr_skl_bones,
                                                                                                            self.nbr_anm_bones),
              file=ostream)
        print(line_prefix + "Namehashoffset: %s, Animated bones indices offset: %s, Bonenameoffset: %s" % (self.offset_namehashes,
                                                                                                           self.offset_skn_indices,
                                                                                                           self.offset_bone_names),
              file=ostream)
