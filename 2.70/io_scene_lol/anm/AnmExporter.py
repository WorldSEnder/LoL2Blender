"""
Created on 29.03.2014

@author: Carbon
"""
from ..util import get_arm_from_context
from .AnmData import AnmBone, AnmData, AnmTrans, MODE_INTERNAL
from os import path
import struct



def write_to_file(filepath, context, **options):
    """
    Exports the active object in the context given to the file given
    """
    if path.isfile(filepath):
        file = open(filepath, mode='wb')
        writer = AnmWriter()
        writer.from_scene(context)
        try:
            if options["dumpData"]:
                # this is debug
                # pylint: disable=protected-access
                writer._data.dump_data()
        except KeyError:
            pass # do nothing
        writer.write_to_file(file)
        file.flush()
        file.close()
        return {'FINISHED'}
    else:
        return {'CANCELLED'}

class AnmWriter(object):
    """
    Convenience class to write data
    """
    def __init__(self):
        self._data = AnmData(MODE_INTERNAL)
        self._data.version = 3

    def from_scene(self, context):
        """
        Initializes the writer (reads from the context given)
        Will work on the currently selected object
        """

        # setup
        armature = get_arm_from_context(context)
        data = self._data
        scene = context.scene
        data.fps = scene.render.fps / scene.render.fps_base
        start = scene.frame_start
        end = scene.frame_end
        # setup data
        data.num_frames = (end - start) + 1
        data.bones = [AnmBone(bone.parent is None, bone.name) for bone in armature.pose.bones]
        # append poses of current motion
        for frame in range(start, end+1):
            scene.frame_current = frame
            for bone, bone_internal in zip(data.bones, armature.pose.bones):
                bone.poses.append(AnmTrans(bone_internal.rotation_quaternion, bone_internal.location))
            # all bones done
        # all frames done

    def write_to_file(self, fostream):
        """
        Writes the previously initialized data to the stream given
        """
        data = self._data
        data.switch_to_file_mode()
        fostream.write("r3d2anmd".encode())
        header_data = struct.pack("<4If", data.version, 0x43211234, len(data.bones), data.num_frames, data.fps)
        fostream.write(header_data)
        for bone in data.bones:
            buffer = bytearray(AnmBone.kHeaderSize + len(bone.poses) * AnmTrans.kSizeInFile)
            # this can't handle utf-8 I KNOW
            name_encoded = bone.name.encode()[:AnmBone.kNameLen]
            buffer[:len(name_encoded)] = name_encoded
            struct.pack_into("<I", buffer, AnmBone.kNameLen, 2 if bone.is_root else 0)
            for i, pose in enumerate(bone.poses):
                # rot and loc are vectors so we unpack them and pack them into the bytebuffer
                struct.pack_into("<7f", buffer, i * AnmTrans.kSizeInFile, *(pose.rot + pose.loc))
            fostream.write(buffer)
        # all bones finished
