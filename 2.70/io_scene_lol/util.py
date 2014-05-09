"""
Utility module
Created on 31.03.2014

@author: Carbon
"""
MODE_FILE = 0
MODE_INTERNAL = 1

DESIGNER_ID = 0x43211234

from io import UnsupportedOperation
# from mathutils import Quaternion, Vector
from os import path
import functools
import struct

def debug_func(func):
    """Annotation for debugging a function"""
    @functools.wraps(func)
    def wrapped(*args, **wargs):
        """
        Wrapper function that outputs the result to stdout
        """
        res = func(*args, **wargs)
        print(func.__name__, res)
        return res
    return wrapped

def get_arm_from_context(context):
    """
    Finds a suitable armature-obj in the context
    """
    # TODO: get this out of here, get_arm_from_context
    armature = context.object
    if armature.type == 'MESH':
        # if selected object is a mesh (the model)
        for modifier in (mod for mod in armature.modifiers if mod.type == 'ARMATURE'):
            # extract armature of the first armature modifier if not None
            obj = modifier.object
            armature = armature if obj is None else obj
            break
    if armature.type == 'ARMATURE':
        return armature
    # we didn't find any valid armature modifiers or object was not mesh
    for obj in context.scene.objects:
        if obj.type == 'ARMATURE':
            # return first armature in scene
            return obj
    # no armature object in the scene
    raise TypeError("Context object can't be resolved to an armature object")

def split(rand_acc, *args):
    """
    Splits a list into several parts on the indices given.
    Example: split([1,2,3,4,5,6,7], 1,3,5,6) == (1, [2,3], [4,5], 6, 7)
    If no arguments were given the function still beatufies the list given
    Example: split([item]) == item
    """
    def beautify(liszt):
        """
        Beautifies the list. More exactly flattens it if length is 1
        """
        return liszt[0] if len(liszt) == 1 else liszt
    return beautify([beautify(rand_acc[x:y]) for x, y in zip((0,) + args, args + (len(rand_acc),))])

def seek(fstream, offset, mode=0):
    """
    Seeks for the given position in the file with the given filestream and the given mode.
    mode
    * 0 -- start of stream (the default); offset should be zero or positive
    * 1 -- current stream position; offset may be negative
    * 2 -- end of stream; offset is usually negative
    """
    try:
        return fstream.seek(offset, mode)
    except UnsupportedOperation as uop:
        if mode == 1:
            cur = fstream.tell()
            return fstream.seek(cur + offset, 0)
        raise uop

def tell_f_length(fistream, prefix="", error_pattern="%s: Can't tell length of stream"):
    """
    Gets the length of the filestream given and resets the stream to
    its original position
    """
    pos = fistream.tell()
    f_length = seek(fistream, 0, 2)
    if seek(fistream, pos, 0) != pos:
        raise TypeError(error_pattern % prefix)
    return f_length

def length_check(min_length, f_length, prefix="", error_pattern="%s: Stream not long enough"):
    """
    With a Preprocessor I would just #define that but in this case...
    """
    if f_length < min_length:
        raise TypeError(error_pattern % prefix)

def quats_to_matrix(qx, qy, qz, qw, tx, ty, tz): # pylint: disable=invalid-name
    """
    Converts the quaternions and the translation into a 4-dimensional matrix
    """
    # this is straight up math, nothing to "graps" or "understand". Var names are practical
    # pylint: disable=invalid-name
    mat = Quaternion((qx, qy, qz, qw)).to_matrix().to_4x4()
    mat.translation = Vector((tx, ty, tz))
    return mat

def unpack(fmt, buffer):
    """
    Wrapper for struct.unpack(fmt). Determines size automatically.
    Also automatically unpacks one-value-tupples
    """
    size = struct.calcsize(fmt)
    items = struct.unpack(fmt, buffer.read(size))
    return items[0] if len(items) == 1 else items

def read_from_file(reader_type, filepath, context, **options):
    """
    Imports from a file_stream with the given reader
    """
    result = {'CANCELLED'}
    if path.isfile(filepath):
        file_stream = open(filepath, mode='rb')
        reader = reader_type()
        if not reader_type.is_my_file(file_stream, True):
            file_stream.close()
            return {'CANCELLED'}
        try:
            warnings = reader.read_from_file(file_stream)
            if warnings is None:
                warnings = []
            for warn in warnings:
                print(warn)
        except:
            print("An exception occured. Current state of the reader:\n%s" % reader)
            raise
        try:
            if options["dumpData"]:
                stream = None
                try:
                    stream = options["ostream"]
                except KeyError:
                    pass # to stdout
                print(reader, file=stream)
        except KeyError:
            pass # option not specified
        try:
            result = reader.to_scene(context)
            if result is None:
                result = {'FINISHED'}
        except NotImplementedError:
            pass
    return result

class AbstractReader(object):
    """
    Represents an abstact reader to extend
    """
    @classmethod
    def is_my_file(cls, fistream, reset=True):
        """
        Reads from the fistream and tells if the stream (supposedly) describes an animation file
        fistream - the stream to read from
        reset - whether to reset the filestream to the original position
        """
        raise NotImplementedError()

    def read_from_file(self, fistream):
        """
        Reads the model from the given fistream
        """
        raise NotImplementedError()

    def to_scene(self, d__context):
        """
        Insert data into the scene
        """
        raise NotImplementedError()

    def __str__(self):
        """
        Gives the data
        """
