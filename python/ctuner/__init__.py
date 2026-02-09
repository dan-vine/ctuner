"""
ctuner - Instrument tuner with temperament support
"""

from .constants import SAMPLE_RATE, BUFFER_SIZE, A4_REFERENCE, NOTE_NAMES
from .pitch_detector import PitchDetector, PitchResult
from .temperaments import Temperament, TEMPERAMENTS
from .tuner import Tuner

__version__ = "0.1.0"
__all__ = [
    "PitchDetector",
    "PitchResult",
    "Tuner",
    "Temperament",
    "TEMPERAMENTS",
    "SAMPLE_RATE",
    "BUFFER_SIZE",
    "A4_REFERENCE",
    "NOTE_NAMES",
]
