"""
Pitch detection using aubio with temperament support
"""

from dataclasses import dataclass, field
import numpy as np
import aubio

from .constants import (
    SAMPLE_RATE,
    BUFFER_SIZE,
    FFT_SIZE,
    A4_REFERENCE,
    C5_OFFSET,
    A_OFFSET,
    OCTAVE,
    NOTE_NAMES,
)
from .temperaments import Temperament, TEMPERAMENT_RATIOS


@dataclass
class PitchResult:
    """Result of pitch detection"""
    frequency: float = 0.0
    ref_frequency: float = 0.0
    cents: float = 0.0
    note: int = 0
    octave: int = 0
    note_name: str = ""
    confidence: float = 0.0
    valid: bool = False

    @property
    def note_in_octave(self) -> int:
        return self.note % OCTAVE


@dataclass
class Maximum:
    """Peak information from spectrum analysis"""
    frequency: float = 0.0
    ref_frequency: float = 0.0
    note: int = 0


class PitchDetector:
    """
    Pitch detector using aubio with temperament support.

    This class wraps aubio's pitch detection and adds:
    - Temperament-adjusted reference frequencies
    - Cents calculation relative to the tempered note
    - Key transposition support
    """

    def __init__(
        self,
        sample_rate: int = SAMPLE_RATE,
        buffer_size: int = BUFFER_SIZE,
        fft_size: int = FFT_SIZE,
        reference: float = A4_REFERENCE,
        method: str = "yin",
    ):
        """
        Initialize pitch detector.

        Args:
            sample_rate: Audio sample rate in Hz
            buffer_size: Number of samples per processing block
            fft_size: FFT size (must be >= buffer_size)
            reference: Reference frequency for A4 in Hz
            method: Pitch detection method ("yinfft", "yin", "schmitt", "fcomb")
        """
        self.sample_rate = sample_rate
        self.buffer_size = buffer_size
        self.fft_size = fft_size
        self.reference = reference
        self.temperament = Temperament.EQUAL
        self.key = 0  # 0 = C, 1 = C#, etc.

        # Initialize aubio pitch detector
        self._pitch = aubio.pitch(method, fft_size, buffer_size, sample_rate)
        self._pitch.set_unit("Hz")
        self._pitch.set_tolerance(0.8)
        self._pitch.set_silence(-40)  # dB threshold

        # Store last detection for spectrum display compatibility
        self._last_maxima: list[Maximum] = []

    def process(self, samples: np.ndarray) -> PitchResult:
        """
        Process audio samples and return pitch result.

        Args:
            samples: Audio samples as numpy array (float32 or will be converted)

        Returns:
            PitchResult with detected pitch information
        """
        # Ensure float32 format for aubio
        if samples.dtype != np.float32:
            samples = samples.astype(np.float32)

        # Ensure correct length
        if len(samples) < self.buffer_size:
            padded = np.zeros(self.buffer_size, dtype=np.float32)
            padded[:len(samples)] = samples
            samples = padded
        elif len(samples) > self.buffer_size:
            samples = samples[:self.buffer_size]

        # Detect pitch
        frequency = float(self._pitch(samples)[0])
        confidence = float(self._pitch.get_confidence())

        # No pitch detected (low confidence threshold for synthetic signals)
        if frequency < 20:
            self._last_maxima = []
            return PitchResult()

        # Convert frequency to note number
        # semitones_from_a4 = 12 * log2(f / 440)
        semitones_from_a4 = 12 * np.log2(frequency / self.reference)
        note = int(round(semitones_from_a4)) + C5_OFFSET

        if note < 0:
            self._last_maxima = []
            return PitchResult()

        # Calculate temperament-adjusted reference frequency
        ref_frequency = self._get_reference_frequency(note)

        # Calculate cents deviation from reference
        if ref_frequency > 0:
            cents = 1200 * np.log2(frequency / ref_frequency)
        else:
            cents = 0.0

        # Only valid if within +/- 50 cents
        if abs(cents) > 50:
            self._last_maxima = []
            return PitchResult()

        # Store maximum for compatibility
        self._last_maxima = [Maximum(
            frequency=frequency,
            ref_frequency=ref_frequency,
            note=note,
        )]

        return PitchResult(
            frequency=frequency,
            ref_frequency=ref_frequency,
            cents=cents,
            note=note,
            octave=note // OCTAVE,
            note_name=NOTE_NAMES[note % OCTAVE],
            confidence=confidence,
            valid=True,
        )

    def _get_reference_frequency(self, note: int) -> float:
        """
        Calculate temperament-adjusted reference frequency for a note.

        Args:
            note: MIDI-like note number (C0 = 0, A4 = 57)

        Returns:
            Reference frequency in Hz adjusted for the current temperament
        """
        note_in_octave = note % OCTAVE
        octave = note // OCTAVE

        # Get temperament ratios
        ratios = TEMPERAMENT_RATIOS[self.temperament]
        equal_ratios = TEMPERAMENT_RATIOS[Temperament.EQUAL]

        # Adjust for key (transpose the temperament)
        n = (note_in_octave - self.key + OCTAVE) % OCTAVE
        a = (A_OFFSET - self.key + OCTAVE) % OCTAVE

        # Calculate temperament adjustment ratio
        temper_ratio = ratios[n] / ratios[a]
        equal_ratio = equal_ratios[n] / equal_ratios[a]
        temper_adjust = temper_ratio / equal_ratio

        # Calculate equal temperament frequency first
        semitones_from_a4 = note - C5_OFFSET
        equal_freq = self.reference * (2 ** (semitones_from_a4 / 12))

        # Apply temperament adjustment
        return equal_freq * temper_adjust

    def get_maxima(self) -> list[Maximum]:
        """Get detected maxima (for compatibility with C++ interface)"""
        return self._last_maxima

    def set_reference(self, freq: float):
        """Set reference frequency for A4"""
        self.reference = freq

    def set_temperament(self, temperament: Temperament):
        """Set musical temperament"""
        self.temperament = temperament

    def set_key(self, key: int):
        """Set key for temperament (0=C, 1=C#, ..., 11=B)"""
        self.key = key % OCTAVE

    def set_tolerance(self, tolerance: float):
        """Set pitch detection tolerance (0.0 to 1.0)"""
        self._pitch.set_tolerance(tolerance)

    def set_silence_threshold(self, db: float):
        """Set silence threshold in dB"""
        self._pitch.set_silence(db)
