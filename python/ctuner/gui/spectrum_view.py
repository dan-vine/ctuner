"""
Spectrum view widget - FFT spectrum display showing reed peaks.
"""

import numpy as np
from PySide6.QtWidgets import QFrame, QVBoxLayout
from PySide6.QtCore import Qt
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QPainterPath

from .styles import (
    PANEL_BACKGROUND,
    ACCENT_GREEN,
    TEXT_COLOR,
    TEXT_SECONDARY,
    BORDER_COLOR,
    METER_BACKGROUND,
)


class SpectrumView(QFrame):
    """
    FFT spectrum display showing frequency peaks.

    Displays:
    - Frequency spectrum as a line graph
    - Detected reed peaks highlighted
    - Frequency axis labels
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("spectrumView")
        self._frequencies: np.ndarray | None = None
        self._magnitudes: np.ndarray | None = None
        self._peak_frequencies: list[float] = []

        self._min_freq = 20.0
        self._max_freq = 2000.0

        self._apply_style()
        self.setMinimumHeight(120)

    def _apply_style(self):
        """Apply styling to the widget."""
        self.setStyleSheet(f"""
            QFrame#spectrumView {{
                background-color: {PANEL_BACKGROUND};
                border: 1px solid {BORDER_COLOR};
                border-radius: 8px;
            }}
        """)

    def set_spectrum(self, frequencies: np.ndarray, magnitudes: np.ndarray):
        """
        Set spectrum data.

        Args:
            frequencies: Array of frequency values in Hz
            magnitudes: Array of magnitude values (normalized 0-1)
        """
        self._frequencies = frequencies
        self._magnitudes = magnitudes
        self.update()

    def set_peaks(self, peak_frequencies: list[float]):
        """
        Set detected peak frequencies for highlighting.

        Args:
            peak_frequencies: List of detected peak frequencies in Hz
        """
        self._peak_frequencies = peak_frequencies
        self.update()

    def clear(self):
        """Clear the spectrum display."""
        self._frequencies = None
        self._magnitudes = None
        self._peak_frequencies = []
        self.update()

    def set_zoom(self, center_freq: float | None, zoom_enabled: bool = True):
        """
        Set zoom to focus on a frequency range.

        Args:
            center_freq: Center frequency to zoom to (None to reset to full range)
            zoom_enabled: Whether zoom is enabled
        """
        if not zoom_enabled or center_freq is None:
            self._min_freq = 20.0
            self._max_freq = 2000.0
        else:
            # Zoom to +/- 1 octave around the center frequency
            self._min_freq = max(20.0, center_freq / 2)
            self._max_freq = min(2000.0, center_freq * 2)
        self.update()

    def paintEvent(self, event):
        """Paint the spectrum display."""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        width = self.width()
        height = self.height()
        margin_left = 40
        margin_right = 10
        margin_top = 10
        margin_bottom = 25

        plot_width = width - margin_left - margin_right
        plot_height = height - margin_top - margin_bottom

        # Draw plot background
        painter.setPen(QPen(QColor(BORDER_COLOR), 1))
        painter.setBrush(QBrush(QColor(METER_BACKGROUND)))
        painter.drawRect(margin_left, margin_top, plot_width, plot_height)

        # Draw frequency axis labels
        painter.setPen(QPen(QColor(TEXT_SECONDARY), 1))
        painter.setFont(painter.font())
        freq_labels = [100, 200, 500, 1000, 2000]
        for freq in freq_labels:
            if freq < self._min_freq or freq > self._max_freq:
                continue
            x = self._freq_to_x(freq, margin_left, plot_width)
            painter.drawText(int(x) - 15, height - 5, f"{freq}")
            # Grid line
            painter.setPen(QPen(QColor(BORDER_COLOR), 1, Qt.DotLine))
            painter.drawLine(int(x), margin_top, int(x), margin_top + plot_height)
            painter.setPen(QPen(QColor(TEXT_SECONDARY), 1))

        # Draw magnitude axis labels
        for db in [0, -20, -40]:
            mag = 10 ** (db / 20)  # Convert dB to linear
            y = self._mag_to_y(mag, margin_top, plot_height)
            painter.drawText(5, int(y) + 4, f"{db}")
            # Grid line
            painter.setPen(QPen(QColor(BORDER_COLOR), 1, Qt.DotLine))
            painter.drawLine(margin_left, int(y), margin_left + plot_width, int(y))
            painter.setPen(QPen(QColor(TEXT_SECONDARY), 1))

        # Draw spectrum if data available
        if self._frequencies is not None and self._magnitudes is not None and len(self._frequencies) > 0:
            # Create path for spectrum line
            path = QPainterPath()

            # Downsample for performance if needed
            step = max(1, len(self._frequencies) // plot_width)
            freqs = self._frequencies[::step]
            mags = self._magnitudes[::step]

            first_point = True
            for freq, mag in zip(freqs, mags):
                if freq < self._min_freq or freq > self._max_freq:
                    continue
                x = self._freq_to_x(freq, margin_left, plot_width)
                y = self._mag_to_y(mag, margin_top, plot_height)

                if first_point:
                    path.moveTo(x, y)
                    first_point = False
                else:
                    path.lineTo(x, y)

            # Draw spectrum line
            painter.setPen(QPen(QColor(ACCENT_GREEN), 1.5))
            painter.setBrush(Qt.NoBrush)
            painter.drawPath(path)

            # Draw peak markers
            painter.setPen(QPen(QColor(TEXT_COLOR), 2))
            for peak_freq in self._peak_frequencies:
                if peak_freq < self._min_freq or peak_freq > self._max_freq:
                    continue
                x = self._freq_to_x(peak_freq, margin_left, plot_width)
                # Find magnitude at this frequency
                idx = np.searchsorted(self._frequencies, peak_freq)
                if 0 <= idx < len(self._magnitudes):
                    mag = self._magnitudes[idx]
                    y = self._mag_to_y(mag, margin_top, plot_height)
                    # Draw marker
                    painter.drawEllipse(int(x) - 4, int(y) - 4, 8, 8)

    def _freq_to_x(self, freq: float, start: float, width: float) -> float:
        """Convert frequency to x coordinate (logarithmic scale)."""
        if freq <= 0:
            return start
        log_min = np.log10(self._min_freq)
        log_max = np.log10(self._max_freq)
        log_freq = np.log10(freq)
        normalized = (log_freq - log_min) / (log_max - log_min)
        return start + normalized * width

    def _mag_to_y(self, mag: float, start: float, height: float) -> float:
        """Convert magnitude to y coordinate (linear scale, inverted)."""
        # Clamp magnitude to valid range
        mag = max(0.001, min(1.0, mag))
        return start + (1.0 - mag) * height
