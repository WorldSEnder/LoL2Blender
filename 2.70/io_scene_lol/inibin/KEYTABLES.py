"""
Created on 05.05.2014

This work is based on https://github.com/Met48/inibin and https://github.com/hmoog/riot-decode/blob/master/riotDecode/inibin/
   -> last fork 05.05.2014

@author: Carbon
"""
from ..util import split, unpack
from collections import defaultdict
import pickle
import re

def read_int(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a plain int...
    """
    return unpack("<%si" % count, fistream)

def read_float(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a plain float...
    """
    return unpack("<%sf" % count, fistream)

def read_bytepercent(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a plain 10%-integer...
    """
    return [i / 10 for i in unpack("<%sB" % (count,), fistream)]

def read_short(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a plain short...
    """
    return unpack("<%sh" % (count,), fistream)

def read_byte(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a plain byte...
    """
    return unpack("<%sB" % (count,), fistream)

def read_boolean(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a plain boolean...
    """
    b_count = (count + 7) // 8
    b_array = unpack("%sB" % b_count, fistream)

    return [((b_array[i // 8] & (1 << i % 8)) >> i % 8) for i in range(count)]

def read_2bytepercent(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a 2-dim vector of perc-values. E.g. ...
    """
    return split([i / 10 for i in unpack("<%sB" % (2 * count), fistream)], *((i + 1) * 2 for i in range(count - 1)))

def read_3bytepercent(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a 3-dim vector of perc-values. E.g. levelup restriction for ultimate
    """
    return split([i / 10 for i in unpack("<%sB" % (3 * count), fistream)], *((i + 1) * 3 for i in range(count - 1)))

def read_4bytepercent(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a 4-dim vector of  perc-values... E.g. max.levels restriction for skills
    """
    return split([i / 10 for i in unpack("<%sB" % (4 * count), fistream)], *((i + 1) * 4 for i in range(count - 1)))

def read_2float(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a 2-dim vector of ?ints?
    """
    return split(unpack("<%sf" % (2 * count), fistream), *((i + 1) * 2 for i in range(count - 1)))

def read_3float(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a 3-dim vector of ?ints?
    """
    return split(unpack("<%sf" % (3 * count), fistream), *((i + 1) * 3 for i in range(count - 1)))

def read_4float(count, fistream, d__data): # pylint: disable=unused-argument
    """
    Reads a 4-dim vector of ?ints?
    """
    return split(unpack("<%sf" % (4 * count), fistream), *((i + 1) * 4 for i in range(count - 1)))

def read_strings(count, fistream, data): # pylint: disable=unused-argument
    """
    Reads the strings from the fistream. They are special because they need information from the header
    """
    offsets = unpack("<%sH" % (count,), fistream)
    strings = fistream.read(data.header.string_table_length).decode("latin-1")
    return [strings[offset:].split('\x00')[0] for offset in offsets]

class CustomDefaultDict(dict):
    """
    An implementation of default-dict that allows to acces
    the key if the key is not present
    """
    def __init__(self, factory):
        """
        Factory is a callable that gets called if the key is not present
        """
        super(CustomDefaultDict, self).__init__()
        self._factory = factory

    def __missing__(self, key):
        """
        If the key is missing
        """
        self[key] = self._factory(key)
        return self[key]

# Only delegates method calls and is used to order stuff...
class NestedNode(object): # pylint: disable=too-few-public-methods
    """
    Represents a nested node. Carries an identifier and an object.
    Returns a NestedValue when called so that you don't lose the
    order.
    """
    def __init__(self, name, value):
        """
        name is like a package path. A 'a.b' means 'b' child-of 'a'
        The ordering is top-down, so:
        'a'
        'a.b'
        'a.c'
        'a.c.a'
        'a.c.b'
        'b.z.i'
        'z'
        """
        self._name = name
        self._name_pieces = name.split('.')
        self.value = value
    def matches(self, test_re):
        """
        Returns whether the path of this Node matches the re given
        """
        return re.match(test_re, self._name)
    def _cmp(self, other):
        """
        Python 2 comparison function... it's easier this way
        """
        # we should be allowed to access own privates
        other_pieces = other._name_pieces # pylint: disable=protected-access
        for s_np, o_np in zip(self._name_pieces, other_pieces):
            if s_np < o_np:
                return -1
            elif s_np > o_np:
                return 1
        return len(self._name_pieces) - len(other_pieces)

    def __lt__(self, other):
        """
        Comparison function
        """
        if not isinstance(other, NestedNode):
            return self.value.__lt__(other)
        return self._cmp(other) < 0

    def __gt__(self, other):
        """
        Comparison function
        """
        if not isinstance(other, NestedNode):
            return self.value.__gt__(other)
        return self._cmp(other) > 0

    def __eq__(self, other):
        """
        Comparison function
        """
        if not isinstance(other, NestedNode):
            return self.value.__eq__(other)
        return self._cmp(other) == 0

    def __le__(self, other):
        """
        Comparison function
        """
        if not isinstance(other, NestedNode):
            return self.value.__le__(other)
        return self._cmp(other) <= 0

    def __ge__(self, other):
        """
        Comparison function
        """
        if not isinstance(other, NestedNode):
            return self.value.__ge__(other)
        return self._cmp(other) >= 0

    def __ne__(self, other):
        """
        Comparison function
        """
        if not isinstance(other, NestedNode):
            return self.value.__ne__(other)
        return self._cmp(other) != 0

    def __call__(self, *args, **wargs):
        """
        Emulates a call to this object and delegates it to
        the function of this object
        """
        return NestedNode(self._name, self.value(*args, **wargs))

    def __str__(self):
        """
        An informal representation of the object
            -> the hold value
        """
        return str(self.value)

    def __repr__(self):
        """
        Representation of this object
        """
        return "NestedNode(%s, %s)" % (self._name, repr(self.value))

KEY_TABLE = CustomDefaultDict(lambda x: NestedNode('\xff.%s' % x, lambda y: "U_%s: %s" % (x, y)))

def func_factory(value):
    """Returns a 'lambda-function'"""
    pattern = value + " %s"
    def lmbda(y):
        """The lambda"""
        return pattern % (y,)
    return lmbda
# the problem is that a lambda-expression still references the val and therefore keeps the last val of the for-loop

KEY_TABLE.update({key: NestedNode(val, func_factory(val)) for key, val in pickle.load(open('./io_scene_lol/inibin/keydict.dict', 'rb')).items()})

CUSTOM_TABLE = {
    # Bases do not include level one per-level bonus
    742042233:      NestedNode('Stats.base.hp.base.base', lambda y: 'Base HP: %s' % y),
    - 988146097:    NestedNode('Stats.base.hp.base.perlv', lambda y: 'HP per Level: %s' % y),
    - 166675978:    NestedNode('Stats.base.hp.per5.base', lambda y: 'HP/5: %s' % (y * 5)),# Convert from hp1
    - 1232864324:   NestedNode('Stats.base.hp.per5.perlv', lambda y: 'HP/5 per Level: %s' % (y * 5)),
    742370228:      NestedNode('Stats.base.mana.base.base', lambda y: 'Base Mana: %s' % y),
    1003217290:     NestedNode('Stats.base.mana.base.perlv', lambda y: 'Mana per Level: %s' % y),
    619143803:      NestedNode('Stats.base.mana.per5.base', lambda y: 'Mana/5: %s' % (y * 5)),# Convert from hp1
    1248483905:     NestedNode('Stats.base.mana.per5.perlv', lambda y: 'Mana/5 per Level: %s' % (y * 5)),
    1387461685:     NestedNode('Stats.off.range', lambda y: 'Range: %s' % y),
    1880118880:     NestedNode('Stats.off.damage.base', lambda y: 'Base Damage: %s' % y),
    1139868982:     NestedNode('Stats.off.damage.perlv', lambda y: 'Damage per Level: %s' % y),
    - 2103674057:   NestedNode('Stats.off.attspeed.base', lambda y: 'Base Attackspeed: %s' % (0.625 / (1.0 + float(y))) if float(y) != -1.0 else 'N/A'),
    770205030:      NestedNode('Stats.off.attspeed.perlv', lambda y: 'Attackspeed per Level: %s%%' % y),
    - 1695914273:   NestedNode('Stats.def.armor.base', lambda y: 'Base Armor: %s' % y),
    1608827366:     NestedNode('Stats.def.armor.perlv', lambda y: 'Armor per Level: %s' % y),
    1395891205:     NestedNode('Stats.def.mr.base', lambda y: 'Base MR: %s' % y),
    - 262788340:    NestedNode('Stats.def.mr.perlv', lambda y: 'MR per Level: %s' % y),
    1081768566:     NestedNode('Stats.util.movespeed', lambda y: 'Movement speed: %s' % y),
    - 688356814:    NestedNode('kit.champ.icon_square', lambda y: 'Icon Square: %s' % y),
    - 902749819:    NestedNode('kit.champ.icon_round', lambda y: 'Icon Square: %s' % y),
    - 893169035:    NestedNode('kit.skills.0passive.Name', lambda y: 'Passive: %s' % y),
    743602011:      NestedNode('kit.skills.0passive.descr', lambda y: 'Passive Description: %s' % y),
    - 484483517:    NestedNode('kit.skills.0passive.icon', lambda y: 'Passive Icon: %s' % y),
    404599689:      NestedNode('kit.skills.1q.name', lambda y: 'Skill Q: %s' % y), # not in order
    404599690:      NestedNode('kit.skills.2w.name', lambda y: 'Skill W: %s' % y),
    404599691:      NestedNode('kit.skills.3e.name', lambda y: 'Skill E: %s' % y),
    404599692:      NestedNode('kit.skills.4r.name', lambda y: 'Skill R: %s' % y),
    1637964898:     NestedNode('kit.attk.crit', lambda y: 'Critical attack: %s' % y),
    - 148652351:    NestedNode('rating.tags', lambda y: 'Role: %s' % y),
    767627316:      NestedNode('rating.1att', lambda y: 'Attack Power: %s' % y),
    - 1508576948:   NestedNode('rating.2def', lambda y: 'Defense Power: %s' % y),
    1040761625:     NestedNode('rating.3magic', lambda y: 'Magic Power: %s' % y),
    105898023:      NestedNode('rating.4diff', lambda y: 'Difficulty: %s' % y),
    82690155:       NestedNode('info.info.name', lambda y: 'Display Name: %s' % y),
    - 51751813:     NestedNode('info.info.lore', lambda y: 'Champion-Lore: %s' % y),
    - 547924932:    NestedNode('info.info.descr', lambda y: 'Champion Description: %s' % y),
    70667385:       NestedNode('info.tips.friends', lambda y: 'Tips for you: %s' % y),
    70667386:       NestedNode('info.tips.enemies', lambda y: 'Tips for foes: %s' % y),
    - 1373490748:   NestedNode('info.xadd.champId', lambda y: 'Champion id: %s' % y),

    # Outdated
    - 1794871821:   NestedNode('zzz.rating.role', lambda y: 'Playstyle: %s' % y),
    - 75666821:     NestedNode('zzz.skills.0passive.descr', lambda y: 'Passive Description: %s' % y),
    - 648466076:    NestedNode('zzz.skills.1q.name', lambda y: 'Q Name: %s' % y),
    - 639479878:    NestedNode('zzz.skills.1q.descr', lambda y: 'Q Description: %s' % y),
    - 428555229:    NestedNode('zzz.skills.2w.name', lambda y: 'W Name: %s' % y),
    500084411:      NestedNode('zzz.skills.2w.descr', lambda y: 'W Description: %s' % y),
    - 208644382:    NestedNode('zzz.skills.3e.name', lambda y: 'E Name: %s' % y),
    1639648700:     NestedNode('zzz.skills.3e.descr', lambda y: 'E Description: %s' % y),
    11266465:       NestedNode('zzz.skills.4r.name', lambda y: 'Ult Name: %s' % y),
    - 1515754307:   NestedNode('zzz.skills.4r.descr', lambda y: 'Ult Description: %s' % y),
    - 1187602625:   NestedNode('zzz.release_date', lambda y: 'Release Date: %s' % y),
    - 1064145554:   NestedNode('zzz.sounds.death', lambda y: 'Death Sound: %s' % y),
    - 1398755070:   NestedNode('zzz.sounds.ready', lambda y: 'Ready Sound: %s' % y),
    - 1842104711:   NestedNode('zzz.sounds.special1', lambda y: 'Special Sound 1: %s' % y),
    - 1842104710:   NestedNode('zzz.sounds.special2', lambda y: 'Special Sound 2: %s' % y),
    245402728:      NestedNode('zzz.sounds.attack1', lambda y: 'Attack 1 Sound: %s' % y),
    245402729:      NestedNode('zzz.sounds.attack2', lambda y: 'Attack 2 Sound: %s' % y),
    245402730:      NestedNode('zzz.sounds.attack3', lambda y: 'Attack 3 Sound: %s' % y),
    245402731:      NestedNode('zzz.sounds.attack4', lambda y: 'Attack 4 Sound: %s' % y),
    882852607:      NestedNode('zzz.sounds.move1', lambda y: 'Move Sound 1: %s' % y),
    882852608:      NestedNode('zzz.sounds.move2', lambda y: 'Move Sound 2: %s' % y),
    882852609:      NestedNode('zzz.sounds.move3', lambda y: 'Move Sound 3: %s' % y),
    882852610:      NestedNode('zzz.sounds.move4', lambda y: 'Move Sound 4: %s' % y),
    789899530:      NestedNode('zzz.sounds.click1', lambda y: 'Click Sound 1: %s' % y),
    789899531:      NestedNode('zzz.sounds.click2', lambda y: 'Click Sound 2: %s' % y),
    789899532:      NestedNode('zzz.sounds.click3', lambda y: 'Click Sound 3: %s' % y),
    789899533:      NestedNode('zzz.sounds.click4', lambda y: 'Click Sound 4: %s' % y),
    - 171736365:    NestedNode('zzz.sounds.deatha', lambda y: 'Death Sound 2: %s' % y), # only ahri?
    2003803037:     NestedNode('zzz.sounds.readya', lambda y: 'Ready Sound 2: %s' % y), # only ahri?
    - 655500925:    NestedNode('zzz.sounds.attack1a', lambda y: 'Attack 1 Sound (spec.): %s' % y), # only ahri??
    - 655500924:    NestedNode('zzz.sounds.attack2a', lambda y: 'Attack 2 Sound (spec.): %s' % y), # only ahri??
    - 655500923:    NestedNode('zzz.sounds.attack3a', lambda y: 'Attack 3 Sound (spec.): %s' % y), # only ahri??
    - 655500922:    NestedNode('zzz.sounds.attack4a', lambda y: 'Attack 4 Sound (spec.): %s' % y), # only ahri??
    - 9556582:      NestedNode('zzz.sounds.move1s', lambda y: 'Move Sound 1 (spec.): %s' % y), # only ahri??
    - 9556581:      NestedNode('zzz.sounds.move2s', lambda y: 'Move Sound 2 (spec.): %s' % y), # only ahri??
    - 9556580:      NestedNode('zzz.sounds.move3s', lambda y: 'Move Sound 3 (spec.): %s' % y), # only ahri??
    - 9556579:      NestedNode('zzz.sounds.move4s', lambda y: 'Move Sound 4 (spec.): %s' % y), # only ahri??
    43754799:       NestedNode('zzz.sounds.click1s', lambda y: 'Click Sound 1 (spec.): %s' % y), # only ahri??
    43754800:       NestedNode('zzz.sounds.click2s', lambda y: 'Click Sound 2 (spec.): %s' % y), # only ahri??
    43754801:       NestedNode('zzz.sounds.click3s', lambda y: 'Click Sound 3 (spec.): %s' % y), # only ahri??
    43754802:       NestedNode('zzz.sounds.click4s', lambda y: 'Click Sound 4 (spec.): %s' % y), # only ahri??
}

# we have more beautiful values
KEY_TABLE.update({key: val for (key, val) in CUSTOM_TABLE.items() if key not in KEY_TABLE})

def unknown_type_func():
    """
    Used for types where type is not known. We can't proceed.
    """
    raise NotImplementedError('This data-type is not supported yet')

TYPEFLAG_TABLE = defaultdict(lambda:unknown_type_func)
TYPEFLAG_TABLE.update(
{
        0x0001: read_int,           # u32
        0x0002: read_float,         # f
        0x0004: read_bytepercent,   # u8/10
        0x0008: read_short,         # u16
        0x0010: read_byte,          # u8
        0x0020: read_boolean,       # u1
        0x0040: read_3bytepercent,  # [u8/10] *3
        0x0080: read_3float,        # [f]     *3
        0x0100: read_2bytepercent,  # [u8/10] *2
        0x0200: read_2float,        # [f]     *2
        0x0400: read_4bytepercent,  # [u8/10] *4
        0x0800: read_4float,        # [f]     *4
        0x1000: read_strings, # Strings
})

TYPE_OF_FLAG = {
        read_boolean: 'BIT', # also used for value of 1 etc
        read_bytepercent: 'PERC',
        read_byte: 'uBYTE',
        read_short: 'uSHORT',
        read_int: 'INT',
        read_float: 'FLOAT',
        read_2bytepercent: 'PERC[2]',
        read_3bytepercent: 'PERC[3]',
        read_4bytepercent: 'PERC[4]',
        read_2float: 'FLOAT[2]',
        read_3float: 'FLOAT[3]',
        read_4float: 'FLOAT[4]',
        read_strings: 'STR',
        unknown_type_func: 'INVAL'
}
