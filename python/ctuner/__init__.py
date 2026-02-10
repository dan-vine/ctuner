"""
ctuner - Instrument tuner with temperament support
"""

from .constants import SAMPLE_RATE, BUFFER_SIZE, A4_REFERENCE, NOTE_NAMES
from .multi_pitch_detector import MultiPitchDetector, MultiPitchResult, Maximum
from .temperaments import Temperament, TEMPERAMENTS
from .accordion import AccordionDetector, AccordionResult, ReedInfo

__version__ = "0.1.0"
__all__ = [
    "MultiPitchDetector",
    "MultiPitchResult",
    "Maximum",
    "Temperament",
    "TEMPERAMENTS",
    "SAMPLE_RATE",
    "BUFFER_SIZE",
    "A4_REFERENCE",
    "NOTE_NAMES",
    "AccordionDetector",
    "AccordionResult",
    "ReedInfo",
]