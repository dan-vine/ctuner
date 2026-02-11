"""
Main application window for accordion reed tuning.
"""

import sys

import numpy as np
import sounddevice as sd
from PySide6.QtCore import Qt, QTimer
from PySide6.QtWidgets import (
    QApplication,
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPushButton,
    QSizePolicy,
    QSlider,
    QVBoxLayout,
    QWidget,
)

from ..accordion import AccordionDetector, AccordionResult
from ..constants import A4_REFERENCE, NOTE_NAMES, SAMPLE_RATE
from ..temperaments import Temperament
from .note_display import NoteDisplay
from .reed_panel import ReedPanel
from .spectrum_view import SpectrumView
from .styles import (
    BORDER_COLOR,
    MAIN_WINDOW_STYLE,
    PANEL_BACKGROUND,
    SETTINGS_PANEL_STYLE,
    TEXT_SECONDARY,
    TOGGLE_BUTTON_STYLE,
)


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
        self._input_device = None  # None = default device

        # Display settings
        self._lock_display = False
        self._zoom_spectrum = True

        # UI components
        self._reed_panels: list[ReedPanel] = []
        self._settings_expanded = False
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
        self._reeds_combo.addItems(["1", "2", "3", "4"])
        self._reeds_combo.setCurrentIndex(2)  # Default to 3
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

        settings_layout.addSpacing(30)

        # Settings toggle button
        self._settings_toggle = QPushButton("▼ Settings")
        self._settings_toggle.setObjectName("settingsToggle")
        self._settings_toggle.setCheckable(True)
        self._settings_toggle.setStyleSheet(TOGGLE_BUTTON_STYLE)
        self._settings_toggle.clicked.connect(self._toggle_settings)
        settings_layout.addWidget(self._settings_toggle)

        settings_layout.addStretch()

        # Status label
        self._status_label = QLabel("Listening...")
        self._status_label.setStyleSheet(f"color: {TEXT_SECONDARY};")
        settings_layout.addWidget(self._status_label)

        main_layout.addWidget(settings_frame)

        # Expandable settings panel (hidden by default)
        self._settings_panel = self._create_settings_panel()
        self._settings_panel.setVisible(False)
        main_layout.addWidget(self._settings_panel)

    def _create_reed_panels(self):
        """Create reed panels based on current number of reeds."""
        # Clear all items from layout (panels and stretches)
        while self._reed_container.count():
            item = self._reed_container.takeAt(0)
            if item.widget():
                item.widget().deleteLater()

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

    def _create_settings_panel(self) -> QFrame:
        """Create the expandable settings panel with grouped options."""
        panel = QFrame()
        panel.setObjectName("settingsPanel")
        panel.setStyleSheet(SETTINGS_PANEL_STYLE)
        panel.setMinimumHeight(280)

        layout = QGridLayout(panel)
        layout.setContentsMargins(15, 15, 15, 15)
        layout.setSpacing(15)

        # Detection group
        detection_group = QGroupBox("Detection")
        detection_group.setMinimumWidth(220)
        detection_layout = QVBoxLayout(detection_group)
        detection_layout.setSpacing(4)
        detection_layout.setContentsMargins(6, 6, 6, 6)

        # Octave Filter checkbox
        self._octave_filter_cb = QCheckBox("Octave Filter")
        self._octave_filter_cb.setToolTip("Restrict detection to one octave (OFF for chords)")
        self._octave_filter_cb.setChecked(False)  # Default OFF for accordion
        self._octave_filter_cb.stateChanged.connect(self._on_octave_filter_changed)
        detection_layout.addWidget(self._octave_filter_cb)

        # Fundamental Filter checkbox
        self._fundamental_filter_cb = QCheckBox("Fundamental Filter")
        self._fundamental_filter_cb.setToolTip("Only detect harmonics of fundamental")
        self._fundamental_filter_cb.setChecked(False)
        self._fundamental_filter_cb.stateChanged.connect(self._on_fundamental_filter_changed)
        detection_layout.addWidget(self._fundamental_filter_cb)

        # Downsample checkbox
        self._downsample_cb = QCheckBox("Downsample")
        self._downsample_cb.setToolTip("Better low frequency detection")
        self._downsample_cb.setChecked(False)
        self._downsample_cb.stateChanged.connect(self._on_downsample_changed)
        detection_layout.addWidget(self._downsample_cb)

        # Sensitivity slider row
        sensitivity_row = QHBoxLayout()
        sensitivity_row.addWidget(QLabel("Sensitivity:"))
        self._sensitivity_slider = QSlider(Qt.Horizontal)
        self._sensitivity_slider.setRange(5, 50)  # 0.05 to 0.50
        self._sensitivity_slider.setValue(10)  # Default 0.10
        self._sensitivity_slider.setToolTip("Detection threshold (lower = more sensitive)")
        self._sensitivity_slider.valueChanged.connect(self._on_sensitivity_changed)
        sensitivity_row.addWidget(self._sensitivity_slider)
        self._sensitivity_value = QLabel("0.10")
        self._sensitivity_value.setMinimumWidth(35)
        sensitivity_row.addWidget(self._sensitivity_value)
        detection_layout.addLayout(sensitivity_row)

        # Reed Spread slider row
        spread_row = QHBoxLayout()
        spread_row.addWidget(QLabel("Reed Spread:"))
        self._reed_spread_slider = QSlider(Qt.Horizontal)
        self._reed_spread_slider.setRange(20, 100)  # 20 to 100 cents
        self._reed_spread_slider.setValue(50)  # Default 50 cents
        self._reed_spread_slider.setToolTip("Max cents to group as same note")
        self._reed_spread_slider.valueChanged.connect(self._on_reed_spread_changed)
        spread_row.addWidget(self._reed_spread_slider)
        self._reed_spread_value = QLabel("50¢")
        self._reed_spread_value.setMinimumWidth(35)
        spread_row.addWidget(self._reed_spread_value)
        detection_layout.addLayout(spread_row)

        layout.addWidget(detection_group, 0, 0)  # Row 0, Col 0

        # Tuning group
        tuning_group = QGroupBox("Tuning")
        tuning_group.setMinimumWidth(260)
        tuning_layout = QVBoxLayout(tuning_group)
        tuning_layout.setSpacing(4)
        tuning_layout.setContentsMargins(6, 6, 6, 6)

        # Temperament combo row
        temperament_row = QHBoxLayout()
        temperament_row.addWidget(QLabel("Temperament:"))
        self._temperament_combo = QComboBox()
        temperament_names = [
            "Kirnberger I", "Kirnberger II", "Kirnberger III",
            "Werckmeister III", "Werckmeister IV", "Werckmeister V", "Werckmeister VI",
            "Bach-Lehman", "Equal", "Pythagorean", "Just",
            "Meantone", "Meantone 1/4", "Meantone 1/5", "Meantone 1/6",
            "Silbermann", "Salinas", "Zarlino", "Rossi", "Rossi 2",
            "Vallotti", "Young", "Kellner", "Held",
            "Neidhardt I", "Neidhardt II", "Neidhardt III",
            "Bruder 1829", "Barnes", "Prelleur", "Chaumont", "Rameau"
        ]
        self._temperament_combo.addItems(temperament_names)
        self._temperament_combo.setCurrentIndex(Temperament.EQUAL)  # Default to Equal
        self._temperament_combo.currentIndexChanged.connect(self._on_temperament_changed)
        temperament_row.addWidget(self._temperament_combo)
        tuning_layout.addLayout(temperament_row)

        # Key combo row
        key_row = QHBoxLayout()
        key_row.addWidget(QLabel("Key:"))
        self._key_combo = QComboBox()
        self._key_combo.addItems(NOTE_NAMES)
        self._key_combo.setCurrentIndex(0)  # Default to C
        self._key_combo.currentIndexChanged.connect(self._on_key_changed)
        key_row.addWidget(self._key_combo)
        tuning_layout.addLayout(key_row)

        # Transpose spinner row
        transpose_row = QHBoxLayout()
        transpose_row.addWidget(QLabel("Transpose:"))
        self._transpose_spin = QDoubleSpinBox()
        self._transpose_spin.setRange(-6, 6)
        self._transpose_spin.setValue(0)
        self._transpose_spin.setDecimals(0)
        self._transpose_spin.setSuffix(" semitones")
        self._transpose_spin.setToolTip("Transpose display for transposing instruments")
        transpose_row.addWidget(self._transpose_spin)
        tuning_layout.addLayout(transpose_row)

        layout.addWidget(tuning_group, 0, 1)  # Row 0, Col 1

        # Display group
        display_group = QGroupBox("Display")
        display_group.setMinimumWidth(150)
        display_layout = QVBoxLayout(display_group)
        display_layout.setSpacing(4)
        display_layout.setContentsMargins(6, 6, 6, 6)

        # Lock Display checkbox
        self._lock_display_cb = QCheckBox("Lock Display")
        self._lock_display_cb.setToolTip("Freeze display values")
        self._lock_display_cb.setChecked(False)
        self._lock_display_cb.stateChanged.connect(self._on_lock_display_changed)
        display_layout.addWidget(self._lock_display_cb)

        # Zoom Spectrum checkbox
        self._zoom_spectrum_cb = QCheckBox("Zoom Spectrum")
        self._zoom_spectrum_cb.setToolTip("Zoom spectrum to detected note")
        self._zoom_spectrum_cb.setChecked(True)  # Default ON
        self._zoom_spectrum_cb.stateChanged.connect(self._on_zoom_spectrum_changed)
        display_layout.addWidget(self._zoom_spectrum_cb)

        layout.addWidget(display_group, 1, 0)  # Row 1, Col 0

        # Audio group
        audio_group = QGroupBox("Audio")
        audio_group.setMinimumWidth(220)
        audio_layout = QVBoxLayout(audio_group)
        audio_layout.setSpacing(4)
        audio_layout.setContentsMargins(6, 6, 6, 6)

        # Input device combo row
        input_row = QHBoxLayout()
        input_row.addWidget(QLabel("Input Device:"))
        self._input_combo = QComboBox()
        self._populate_audio_devices()
        self._input_combo.currentIndexChanged.connect(self._on_input_device_changed)
        input_row.addWidget(self._input_combo)
        audio_layout.addLayout(input_row)

        # Refresh button
        refresh_btn = QPushButton("Refresh")
        refresh_btn.clicked.connect(self._populate_audio_devices)
        audio_layout.addWidget(refresh_btn)

        layout.addWidget(audio_group, 1, 1)  # Row 1, Col 1

        return panel

    def _populate_audio_devices(self):
        """Populate the audio input device combo box."""
        self._input_combo.clear()
        self._input_combo.addItem("Default", None)

        try:
            devices = sd.query_devices()
            for i, device in enumerate(devices):
                if device['max_input_channels'] > 0:
                    name = device['name']
                    self._input_combo.addItem(name, i)
        except Exception:
            pass

    def _toggle_settings(self, checked: bool):
        """Toggle the expanded settings panel visibility."""
        self._settings_expanded = checked
        self._settings_panel.setVisible(checked)
        self._settings_toggle.setText("▲ Settings" if checked else "▼ Settings")

        # Resize window to accommodate settings panel
        if checked:
            self.setMinimumHeight(900)
            self.resize(self.width(), 900)
        else:
            self.setMinimumHeight(600)
            self.resize(self.width(), 600)

    def _on_octave_filter_changed(self, state):
        """Handle octave filter checkbox change."""
        self._detector.set_octave_filter(self._octave_filter_cb.isChecked())

    def _on_fundamental_filter_changed(self, state):
        """Handle fundamental filter checkbox change."""
        self._detector.set_fundamental_filter(self._fundamental_filter_cb.isChecked())

    def _on_downsample_changed(self, state):
        """Handle downsample checkbox change."""
        self._detector.set_downsample(self._downsample_cb.isChecked())

    def _on_sensitivity_changed(self, value: int):
        """Handle sensitivity slider change."""
        threshold = value / 100.0
        self._detector.set_sensitivity(threshold)
        self._sensitivity_value.setText(f"{threshold:.2f}")

    def _on_reed_spread_changed(self, value: int):
        """Handle reed spread slider change."""
        self._detector.set_reed_spread(float(value))
        self._reed_spread_value.setText(f"{value}¢")

    def _on_temperament_changed(self, index: int):
        """Handle temperament combo change."""
        self._detector.set_temperament(Temperament(index))

    def _on_key_changed(self, index: int):
        """Handle key combo change."""
        self._detector.set_key(index)

    def _on_lock_display_changed(self, state):
        """Handle lock display checkbox change."""
        self._lock_display = self._lock_display_cb.isChecked()

    def _on_zoom_spectrum_changed(self, state):
        """Handle zoom spectrum checkbox change."""
        # Use isChecked() directly for reliable state detection
        self._zoom_spectrum = self._zoom_spectrum_cb.isChecked()
        # Apply zoom change immediately using last known frequency
        center_freq = None
        if self._last_result and self._last_result.valid and self._last_result.reeds:
            center_freq = self._last_result.reeds[0].frequency
        self._spectrum_view.set_zoom(center_freq, self._zoom_spectrum)

    def _on_input_device_changed(self, index: int):
        """Handle input device combo change."""
        device_id = self._input_combo.itemData(index)
        if device_id != self._input_device:
            self._input_device = device_id
            self._restart_audio()

    def _restart_audio(self):
        """Restart audio stream with new device."""
        self._stop_audio()
        self._start_audio()

    def _apply_style(self):
        """Apply styling to the window."""
        self.setStyleSheet(MAIN_WINDOW_STYLE)

    def _start_audio(self):
        """Start audio capture."""
        try:
            self._stream = sd.InputStream(
                device=self._input_device,
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
        # Skip update if display is locked
        if self._lock_display:
            return

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
            # Zoom to detected note if enabled
            if result.reeds:
                self._spectrum_view.set_zoom(result.reeds[0].frequency, self._zoom_spectrum)
            else:
                self._spectrum_view.set_zoom(None, self._zoom_spectrum)

        # Update reed panels with beat frequencies on detuned reeds
        # beat_frequencies contains: [|f1-f2|, |f2-f3|, ...]
        # Display logic:
        #   2 reeds: beat on Reed 2 (detuned reed beats with Reed 1)
        #   3 reeds: beat on Reed 1 and Reed 3 (outer reeds beat with center Reed 2)
        #   4 reeds: beat on Reed 1 and Reed 4 (outer reeds beat with inner reeds)
        num_reeds = len(result.reeds)
        for i, panel in enumerate(self._reed_panels):
            if i < num_reeds:
                reed = result.reeds[i]
                beat_freq = None

                if num_reeds == 2:
                    # Show beat on Reed 2 (index 1)
                    if i == 1 and len(result.beat_frequencies) > 0:
                        beat_freq = result.beat_frequencies[0]
                elif num_reeds == 3:
                    # Show beat on Reed 1 and Reed 3 (indices 0 and 2)
                    if i == 0 and len(result.beat_frequencies) > 0:
                        beat_freq = result.beat_frequencies[0]  # |f1-f2|
                    elif i == 2 and len(result.beat_frequencies) > 1:
                        beat_freq = result.beat_frequencies[1]  # |f2-f3|
                elif num_reeds >= 4:
                    # Show beat on outer reeds (first and last)
                    if i == 0 and len(result.beat_frequencies) > 0:
                        beat_freq = result.beat_frequencies[0]
                    elif i == num_reeds - 1 and len(result.beat_frequencies) > i - 1:
                        beat_freq = result.beat_frequencies[-1]

                panel.set_data(reed.frequency, reed.cents, beat_freq)
            else:
                panel.set_inactive()

    def _on_reeds_changed(self, index: int):
        """Handle number of reeds change."""
        self._num_reeds = index + 1  # 0=1, 1=2, 2=3, 3=4
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
