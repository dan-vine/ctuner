"""
Main application window for accordion reed tuning.
"""

import sys
import numpy as np
import sounddevice as sd

from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QLabel,
    QComboBox,
    QDoubleSpinBox,
    QFrame,
    QSizePolicy,
)
from PySide6.QtCore import Qt, QTimer

from ..accordion import AccordionDetector, AccordionResult
from ..constants import SAMPLE_RATE, A4_REFERENCE

from .reed_panel import ReedPanel
from .note_display import NoteDisplay
from .spectrum_view import SpectrumView
from .styles import MAIN_WINDOW_STYLE, PANEL_BACKGROUND, BORDER_COLOR, TEXT_SECONDARY


class AccordionWindow(QMainWindow):
    """
    Main window for accordion reed tuning application.

    Features:
    - Spectrum display showing FFT of audio
    - Large note display with reference frequency
    - Individual reed panels (2-4)
    - Settings for reference frequency and number of reeds
    """

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Accordion Reed Tuner")
        self.setMinimumSize(800, 600)

        # Detection - create first to get hop_size
        self._reference = A4_REFERENCE
        self._num_reeds = 3
        self._detector = AccordionDetector(
            sample_rate=SAMPLE_RATE,
            reference=self._reference,
            max_reeds=self._num_reeds,
        )

        # Audio settings - buffer size must match detector's hop_size for accurate phase vocoder
        self._sample_rate = SAMPLE_RATE
        self._buffer_size = self._detector._detector.hop_size  # 1024

        # Audio stream
        self._stream = None
        self._audio_buffer = np.zeros(self._buffer_size, dtype=np.float32)

        # UI components
        self._reed_panels: list[ReedPanel] = []
        self._setup_ui()
        self._apply_style()

        # Update timer
        self._timer = QTimer()
        self._timer.timeout.connect(self._update_display)
        self._timer.start(50)  # 20 Hz update rate

        # Last result for display
        self._last_result: AccordionResult | None = None

        # Start audio
        self._start_audio()

    def _setup_ui(self):
        """Set up the UI components."""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(15, 15, 15, 15)
        main_layout.setSpacing(15)

        # Spectrum view (top)
        self._spectrum_view = SpectrumView()
        self._spectrum_view.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self._spectrum_view.setMinimumHeight(150)
        main_layout.addWidget(self._spectrum_view)

        # Note display (center)
        note_container = QHBoxLayout()
        note_container.addStretch()
        self._note_display = NoteDisplay()
        note_container.addWidget(self._note_display)
        note_container.addStretch()
        main_layout.addLayout(note_container)

        # Reed panels (horizontal row)
        self._reed_container = QHBoxLayout()
        self._reed_container.setSpacing(15)
        self._create_reed_panels()
        main_layout.addLayout(self._reed_container)

        # Settings bar (bottom)
        settings_frame = QFrame()
        settings_frame.setStyleSheet(f"""
            QFrame {{
                background-color: {PANEL_BACKGROUND};
                border: 1px solid {BORDER_COLOR};
                border-radius: 8px;
            }}
        """)
        settings_layout = QHBoxLayout(settings_frame)
        settings_layout.setContentsMargins(15, 10, 15, 10)

        # Number of reeds
        reeds_label = QLabel("Reeds:")
        settings_layout.addWidget(reeds_label)

        self._reeds_combo = QComboBox()
        self._reeds_combo.addItems(["2", "3", "4"])
        self._reeds_combo.setCurrentIndex(1)  # Default to 3
        self._reeds_combo.currentIndexChanged.connect(self._on_reeds_changed)
        settings_layout.addWidget(self._reeds_combo)

        settings_layout.addSpacing(30)

        # Reference frequency
        ref_label = QLabel("Reference A4:")
        settings_layout.addWidget(ref_label)

        self._ref_spinbox = QDoubleSpinBox()
        self._ref_spinbox.setRange(400.0, 480.0)
        self._ref_spinbox.setValue(self._reference)
        self._ref_spinbox.setSuffix(" Hz")
        self._ref_spinbox.setDecimals(1)
        self._ref_spinbox.setSingleStep(0.5)
        self._ref_spinbox.valueChanged.connect(self._on_reference_changed)
        settings_layout.addWidget(self._ref_spinbox)

        settings_layout.addStretch()

        # Status label
        self._status_label = QLabel("Listening...")
        self._status_label.setStyleSheet(f"color: {TEXT_SECONDARY};")
        settings_layout.addWidget(self._status_label)

        main_layout.addWidget(settings_frame)

    def _create_reed_panels(self):
        """Create reed panels based on current number of reeds."""
        # Clear existing panels
        for panel in self._reed_panels:
            panel.setParent(None)
            panel.deleteLater()
        self._reed_panels.clear()

        # Add stretch at start for centering
        self._reed_container.addStretch()

        # Create new panels
        for i in range(self._num_reeds):
            panel = ReedPanel(reed_number=i + 1)
            self._reed_panels.append(panel)
            self._reed_container.addWidget(panel)

        # Add stretch at end for centering
        self._reed_container.addStretch()

    def _apply_style(self):
        """Apply styling to the window."""
        self.setStyleSheet(MAIN_WINDOW_STYLE)

    def _start_audio(self):
        """Start audio capture."""
        try:
            self._stream = sd.InputStream(
                samplerate=self._sample_rate,
                blocksize=self._buffer_size,
                channels=1,
                dtype=np.float32,
                callback=self._audio_callback,
            )
            self._stream.start()
            self._status_label.setText("Listening...")
        except Exception as e:
            self._status_label.setText(f"Audio error: {e}")

    def _stop_audio(self):
        """Stop audio capture."""
        if self._stream is not None:
            self._stream.stop()
            self._stream.close()
            self._stream = None

    def _audio_callback(self, indata, frames, time, status):
        """Audio callback - process incoming audio."""
        if status:
            pass  # Ignore status messages

        # Get audio data and apply fixed gain boost for typical microphone levels
        audio = indata[:, 0].copy() * 10.0  # Fixed 10x gain

        self._audio_buffer = audio

        # Process with detector
        self._last_result = self._detector.process(self._audio_buffer)

    def _update_display(self):
        """Update the display with the latest detection result."""
        result = self._last_result

        if result is None or not result.valid:
            self._note_display.set_inactive()
            for panel in self._reed_panels:
                panel.set_inactive()
            if result and result.spectrum_data:
                freqs, mags = result.spectrum_data
                self._spectrum_view.set_spectrum(freqs, mags)
                self._spectrum_view.set_peaks([])
            return

        # Update note display
        self._note_display.set_note(
            result.note_name,
            result.octave,
            result.ref_frequency,
        )

        # Update spectrum view
        if result.spectrum_data:
            freqs, mags = result.spectrum_data
            self._spectrum_view.set_spectrum(freqs, mags)
            self._spectrum_view.set_peaks([r.frequency for r in result.reeds])

        # Update reed panels
        for i, panel in enumerate(self._reed_panels):
            if i < len(result.reeds):
                reed = result.reeds[i]
                # Get beat frequency if not the last reed
                beat_freq = None
                if i < len(result.beat_frequencies):
                    beat_freq = result.beat_frequencies[i]
                panel.set_data(reed.frequency, reed.cents, beat_freq)
            else:
                panel.set_inactive()

    def _on_reeds_changed(self, index: int):
        """Handle number of reeds change."""
        self._num_reeds = index + 2  # 0=2, 1=3, 2=4
        self._detector.set_max_reeds(self._num_reeds)
        self._create_reed_panels()

    def _on_reference_changed(self, value: float):
        """Handle reference frequency change."""
        self._reference = value
        self._detector.set_reference(self._reference)
        self._note_display.set_reference(self._reference)

    def closeEvent(self, event):
        """Handle window close."""
        self._stop_audio()
        self._timer.stop()
        event.accept()


def main():
    """Main entry point for the accordion tuner GUI."""
    app = QApplication(sys.argv)
    app.setStyle("Fusion")  # Use Fusion style for consistent cross-platform look

    window = AccordionWindow()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
