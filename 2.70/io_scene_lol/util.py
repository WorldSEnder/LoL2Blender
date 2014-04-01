"""
Utility module
Created on 31.03.2014

@author: Carbon
"""
MODE_FILE = 0
MODE_INTERNAL = 1

from _io import UnsupportedOperation
def get_arm_from_context(context):
    """
    Finds a suitable armature-obj in the context
    """
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

def split(arr, *args):
    """
    Splits a list into several parts on the indices given.
    Example: split([1,2,3,4,5,6,7], 1,3,5,6) == (1, [2,3], [4,5], 6, 7)
    """
    if len(args) == 0:
        return arr[0] if len(arr) == 1 else arr
    return tuple(arr[x] if (y.__index__() - x.__index__() == 1) else arr[x:y] for x, y in zip((0,) + args, args + (len(arr),)))

def hash_name(name):
    """
    Hashes the given name -> version 4 of anm only uses hashed names
    """
    i = 0
    for char in bytes(name, "ISO-8859-1"):
        i = ord(chr(char).lower()) + 16 * i
        if i & 0xF0000000:
            i ^= i & 0xF0000000 ^ ((i & 0xF0000000) >> 24)
    return i

def seek(fstream, offset, mode=0):
    """
    Seeks for the given position in the file with the given filestream and the given mode.
    """
    try:
        return fstream.seek(offset, mode)
    except UnsupportedOperation as uop:
        if mode == 1:
            cur = fstream.tell()
            return fstream.seek(cur + offset, 0)
        raise uop
