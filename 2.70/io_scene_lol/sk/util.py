"""
Created on 07.05.2014

@author: Carbon
"""

def hash_name(name):
    """
    Hashes the given name -> .anm(version 4) only uses hashed names
    """
    i = 0
    for char in name:
        i = ord(char.lower()) + 16 * i
        if i & 0xF0000000:
            i ^= i & 0xF0000000 ^ ((i & 0xF0000000) >> 24)
    return i
