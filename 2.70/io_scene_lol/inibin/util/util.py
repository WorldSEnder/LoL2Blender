"""
Created on 06.05.2014

@author: Carbon
"""

from io_scene_lol.inibin.util.KeyRegistry import ALL_KEYS
import pickle
import struct

MAXINTP1 = 2 ** (struct.Struct('i').size * 8 - 1)

def int_overflow(val):
    """
    Simulates overflowing integers
    """
    if not -MAXINTP1 <= val < MAXINTP1:
        val = (val + MAXINTP1) % (2 * MAXINTP1) - MAXINTP1
    return val

def get_numeric_inibin_key(sec, name):
    """
    Hash-function
    """
    hash_value = 0
    if sec is not None:
        for char in sec:
            hash_value = int_overflow(ord(char.lower()) + 65599 * hash_value)
        hash_value = int_overflow(65599 * hash_value + 42)
        if name is not None:
            for char in name:
                hash_value = int_overflow(ord(char.lower()) + 65599 * hash_value)
    return hash_value

if __name__ == '__main__':
    # Will update the keydict from the the KeyRegistry-file
    DICT = {}
    for section in ALL_KEYS:
        section_list = ALL_KEYS[section]
        SECTIONDICT = {}
        for var in section_list:
            numeric = get_numeric_inibin_key(section, var)
            if numeric in SECTIONDICT:
                print("Key collision in %s: %s <- %s" % (section, SECTIONDICT[numeric], var))
            SECTIONDICT[numeric] = '.'.join([section, var])
        for key in SECTIONDICT:
            var = SECTIONDICT[key]
            if key in DICT:
                print("Key collision: %s <- %s" % (SECTIONDICT[key], var))
            DICT[key] = var
        print("Processed section %s" % (section,))
    print("Done. pickling")
    FILE = open('./io_scene_lol/inibin/keydict.dict', mode='wb')
    pickle.dump(DICT, FILE)
    FILE.flush()
    FILE.close()
    print("Done")
