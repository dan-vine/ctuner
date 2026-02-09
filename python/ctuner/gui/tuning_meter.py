"""
Tuning meter widget - horizontal bar showing cents deviation.
"""

from PySide6.QtWidgets import QWidget
from PySide6.QtCore import Qt, QRectF
from PySide6.QtGui import QPainter, QColor, QPen, QBrush, QLinearGradient

from .styles import (
    METER_BACKGROUND,
    ACCENT_GREEN,
    WARNING_ORANGE,
    ERROR_RED,
    TEXT_COLOR,
    BORDER_COLOR,
)


class TuningMeter(QWidget):
    """
    Horizontal tuning meter showing cents deviation from -50 to +50.

    The meter displays:
    - A center line at 0 cents
    - A moving indicator showing current deviation
    - Color coding: green (in tune), orange (slightly off), red (very off)
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self._cents = 0.0
        self._active = False

        # Appearance settings
        self._min_cents = -50.0
        self._max_cents = 50.0
        self._green_zone = 5.0   # ±5 cents = green
        self._orange_zone = 15.0  # ±15 cents = orange

        self.setMinimumSize(200, 30)
        self.setMaximumHeight(40)

    def set_cents(self, cents: float):
        """Set the current cents deviation."""
        self._cents = max(self._min_cents, min(self._max_cents, cents))
        self._active = True
        self.update()

    def set_inactive(self):
        """Set meter to inactive state (no signal)."""
        self._active = False
        self.update()

    def paintEvent(self, event):
        """Paint the tuning meter."""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        width = self.width()
        height = self.height()
        margin = 4
        bar_height = height - 2 * margin
        bar_width = width - 2 * margin

        # Draw background
        painter.setPen(QPen(QColor(BORDER_COLOR), 1))
        painter.setBrush(QBrush(QColor(METER_BACKGROUND)))
        painter.drawRoundedRect(margin, margin, bar_width, bar_height, 4, 4)

        # Draw colored zones
        zone_height = bar_height - 4
        zone_y = margin + 2
        zone_width = bar_width - 4
        zone_x = margin + 2

        # Green zone (center)
        green_start = self._cents_to_x(0 - self._green_zone, zone_x, zone_width)
        green_end = self._cents_to_x(0 + self._green_zone, zone_x, zone_width)
        painter.setPen(Qt.NoPen)
        painter.setBrush(QBrush(QColor(ACCENT_GREEN).darker(200)))
        painter.drawRect(int(green_start), int(zone_y), int(green_end - green_start), int(zone_height))

        # Orange zones
        painter.setBrush(QBrush(QColor(WARNING_ORANGE).darker(200)))
        # Left orange
        orange_left_start = self._cents_to_x(0 - self._orange_zone, zone_x, zone_width)
        painter.drawRect(int(orange_left_start), int(zone_y), int(green_start - orange_left_start), int(zone_height))
        # Right orange
        orange_right_end = self._cents_to_x(0 + self._orange_zone, zone_x, zone_width)
        painter.drawRect(int(green_end), int(zone_y), int(orange_right_end - green_end), int(zone_height))

        # Red zones (outer)
        painter.setBrush(QBrush(QColor(ERROR_RED).darker(200)))
        # Left red
        painter.drawRect(int(zone_x), int(zone_y), int(orange_left_start - zone_x), int(zone_height))
        # Right red
        painter.drawRect(int(orange_right_end), int(zone_y), int(zone_x + zone_width - orange_right_end), int(zone_height))

        # Draw center line
        center_x = self._cents_to_x(0, margin, bar_width)
        painter.setPen(QPen(QColor(TEXT_COLOR), 2))
        painter.drawLine(int(center_x), int(margin + 2), int(center_x), int(margin + bar_height - 2))

        # Draw tick marks at ±10, ±20, ±30, ±40 cents
        painter.setPen(QPen(QColor(TEXT_COLOR).darker(150), 1))
        for tick in [-40, -30, -20, -10, 10, 20, 30, 40]:
            tick_x = self._cents_to_x(tick, margin, bar_width)
            painter.drawLine(int(tick_x), int(margin + bar_height - 6), int(tick_x), int(margin + bar_height - 2))

        # Draw indicator if active
        if self._active:
            indicator_x = self._cents_to_x(self._cents, margin, bar_width)
            indicator_color = self._get_indicator_color()

            # Draw indicator triangle/pointer
            painter.setBrush(QBrush(indicator_color))
            painter.setPen(QPen(QColor(TEXT_COLOR), 1))

            # Triangle pointing down
            triangle_size = 8
            points = [
                (indicator_x - triangle_size, margin - 2),
                (indicator_x + triangle_size, margin - 2),
                (indicator_x, margin + 6),
            ]
            from PySide6.QtGui import QPolygonF
            from PySide6.QtCore import QPointF
            polygon = QPolygonF([QPointF(x, y) for x, y in points])
            painter.drawPolygon(polygon)

            # Vertical line
            painter.setPen(QPen(indicator_color, 3))
            painter.drawLine(int(indicator_x), int(margin + 4), int(indicator_x), int(margin + bar_height - 2))

    def _cents_to_x(self, cents: float, start: float, width: float) -> float:
        """Convert cents value to x coordinate."""
        normalized = (cents - self._min_cents) / (self._max_cents - self._min_cents)
        return start + normalized * width

    def _get_indicator_color(self) -> QColor:
        """Get color for indicator based on cents deviation."""
        abs_cents = abs(self._cents)
        if abs_cents <= self._green_zone:
            return QColor(ACCENT_GREEN)
        elif abs_cents <= self._orange_zone:
            return QColor(WARNING_ORANGE)
        else:
            return QColor(ERROR_RED)
