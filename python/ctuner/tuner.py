"""
Main tuner class combining audio capture and pitch detection
"""

from typing import Callable, Optional
import numpy as np
import sounddevice as sd

from .constants import SAMPLE_RATE, BUFFER_SIZE
from .pitch_detector import PitchDetector, PitchResult
from .temperaments import Temperament


class Tuner:
    """
    Main tuner class that combines audio capture with pitch detection.

    Usage:
        def on_pitch(result):
            if result.valid:
                print(f"{result.note_name}{result.octave}: {result.cents:+.1f} cents")

        tuner = Tuner(callback=on_pitch)
        tuner.start()
        # ... run until done ...
        tuner.stop()
    """

    def __init__(
        self,
        callback: Optional[Callable[[PitchResult], None]] = None,
        sample_rate: int = SAMPLE_RATE,
        buffer_size: int = BUFFER_SIZE,
    ):
        """
        Initialize tuner.

        Args:
            callback: Function called with PitchResult for each audio buffer
            sample_rate: Audio sample rate in Hz
            buffer_size: Number of samples per processing block
        """
        self.sample_rate = sample_rate
        self.buffer_size = buffer_size
        self.callback = callback

        self.detector = PitchDetector(
            sample_rate=sample_rate,
            buffer_size=buffer_size,
        )

        self._stream: Optional[sd.InputStream] = None
        self._running = False

        # Store last result for polling access
        self._last_result = PitchResult()

    def start(self):
        """Start audio capture and pitch detection"""
        if self._running:
            return

        self._running = True
        self._stream = sd.InputStream(
            samplerate=self.sample_rate,
            blocksize=self.buffer_size,
            channels=1,
            dtype=np.float32,
            callback=self._audio_callback,
        )
        self._stream.start()

    def stop(self):
        """Stop audio capture"""
        self._running = False
        if self._stream:
            self._stream.stop()
            self._stream.close()
            self._stream = None

    def _audio_callback(self, indata, frames, time, status):
        """Called by sounddevice for each audio buffer"""
        if status:
            # Could log this, but don't print in callback (performance)
            pass

        # Process pitch
        samples = indata[:, 0]  # Mono (first channel)
        result = self.detector.process(samples)
        self._last_result = result

        # Notify callback
        if self.callback:
            self.callback(result)

    @property
    def is_running(self) -> bool:
        """Check if audio capture is running"""
        return self._running

    @property
    def last_result(self) -> PitchResult:
        """Get the last pitch detection result"""
        return self._last_result

    # Pass-through methods to detector for convenience
    def set_reference(self, freq: float):
        """Set reference frequency for A4"""
        self.detector.set_reference(freq)

    def set_temperament(self, temperament: Temperament):
        """Set musical temperament"""
        self.detector.set_temperament(temperament)

    def set_key(self, key: int):
        """Set key for temperament"""
        self.detector.set_key(key)
