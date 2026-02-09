"""
Musical temperaments

Each temperament is defined as cents offset from equal temperament for each
note in the chromatic scale (C, C#, D, Eb, E, F, F#, G, Ab, A, Bb, B).

The offsets are relative to the key, so when key=0 (C), index 0 is C.
When key=2 (D), the temperament is rotated so index 0 applies to D.
"""

from enum import IntEnum


class Temperament(IntEnum):
    """Available temperament presets"""
    KIRNBERGER_I = 0
    KIRNBERGER_II = 1
    KIRNBERGER_III = 2
    WERCKMEISTER_III = 3
    WERCKMEISTER_IV = 4
    WERCKMEISTER_V = 5
    WERCKMEISTER_VI = 6
    BACH_LEHMAN = 7
    EQUAL = 8  # Default
    PYTHAGOREAN = 9
    JUST = 10
    MEANTONE = 11
    MEANTONE_1_4 = 12
    MEANTONE_1_5 = 13
    MEANTONE_1_6 = 14
    SILBERMANN = 15
    SALINAS = 16
    ZARLINO = 17
    ROSSI = 18
    ROSSI_2 = 19
    VALLOTTI = 20
    YOUNG = 21
    KELLNER = 22
    HELD = 23
    NEIDHARDT_I = 24
    NEIDHARDT_II = 25
    NEIDHARDT_III = 26
    BRUDER_1829 = 27
    BARNES = 28
    PRELLEUR = 29
    CHAUMONT = 30
    RAMEAU = 31


# Temperament data as ratios (matching C++ built_in_temperaments.h)
# Each row contains 12 ratios for notes C through B
# These are frequency ratios relative to the tonic
TEMPERAMENT_RATIOS = {
    Temperament.EQUAL: [
        1.0, 1.059463, 1.122462, 1.189207, 1.259921, 1.334840,
        1.414214, 1.498307, 1.587401, 1.681793, 1.781797, 1.887749
    ],
    Temperament.PYTHAGOREAN: [
        1.0, 1.053498, 1.125000, 1.185185, 1.265625, 1.333333,
        1.404664, 1.500000, 1.580247, 1.687500, 1.777778, 1.898437
    ],
    Temperament.JUST: [
        1.0, 1.041667, 1.125000, 1.200000, 1.250000, 1.333333,
        1.406250, 1.500000, 1.600000, 1.666667, 1.800000, 1.875000
    ],
    Temperament.MEANTONE: [
        1.0, 1.044907, 1.118034, 1.196279, 1.250000, 1.337481,
        1.397542, 1.495349, 1.562500, 1.671851, 1.788854, 1.869186
    ],
    Temperament.MEANTONE_1_4: [
        1.0, 1.044907, 1.118034, 1.196279, 1.250000, 1.337481,
        1.397542, 1.495349, 1.562500, 1.671851, 1.788854, 1.869186
    ],
}

# For temperaments not yet defined, use equal temperament as fallback
_equal = TEMPERAMENT_RATIOS[Temperament.EQUAL]
for t in Temperament:
    if t not in TEMPERAMENT_RATIOS:
        TEMPERAMENT_RATIOS[t] = _equal.copy()


# Pre-computed cents offsets from equal temperament (for faster lookup)
def _ratios_to_cents(ratios: list[float]) -> list[float]:
    """Convert frequency ratios to cents offset from equal temperament"""
    import math
    equal = TEMPERAMENT_RATIOS[Temperament.EQUAL]
    return [1200 * math.log2(r / e) for r, e in zip(ratios, equal)]


TEMPERAMENTS = {t: _ratios_to_cents(TEMPERAMENT_RATIOS[t]) for t in Temperament}
