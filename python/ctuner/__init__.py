"""
ctuner - Instrument tuner with temperament support
"""

from .constants import SAMPLE_RATE, BUFFER_SIZE, A4_REFERENCE, NOTE_NAMES
from .multi_pitch_detector import MultiPitchDetector, MultiPitchResult, Maximum
from .temperaments import Temperament, TEMPERAMENTS

# Optional imports that require aubio
try:
    from .pitch_detector import PitchDetector, PitchResult
    from .tuner import Tuner
    _HAS_AUBIO = True
except ImportError:
    PitchDetector = None
    PitchResult = None
    Tuner = None
    _HAS_AUBIO = False

__version__ = "0.1.0"
__all__ = [
    "PitchDetector",
    "PitchResult",
    "MultiPitchDetector",
    "MultiPitchResult",
    "Maximum",
    "Tuner",
    "Temperament",
    "TEMPERAMENTS",
    "SAMPLE_RATE",
    "BUFFER_SIZE",
    "A4_REFERENCE",
    "NOTE_NAMES",
]
