"""
Qt stylesheets for modern dark theme.

Color scheme:
- Background: #1e1e1e (dark gray)
- Panel background: #2d2d2d
- Text: #ffffff
- Accent (in tune): #4caf50 (green)
- Warning (slightly off): #ff9800 (orange)
- Error (very off): #f44336 (red)
- Meter background: #3d3d3d
"""

# Colors
BACKGROUND = "#1e1e1e"
PANEL_BACKGROUND = "#2d2d2d"
TEXT_COLOR = "#ffffff"
TEXT_SECONDARY = "#b0b0b0"
ACCENT_GREEN = "#4caf50"
WARNING_ORANGE = "#ff9800"
ERROR_RED = "#f44336"
METER_BACKGROUND = "#3d3d3d"
BORDER_COLOR = "#404040"

# Main window stylesheet
MAIN_WINDOW_STYLE = f"""
QMainWindow {{
    background-color: {BACKGROUND};
}}
QWidget {{
    background-color: {BACKGROUND};
    color: {TEXT_COLOR};
    font-family: "Segoe UI", "SF Pro Display", "Helvetica Neue", sans-serif;
}}
QLabel {{
    color: {TEXT_COLOR};
}}
QComboBox {{
    background-color: {PANEL_BACKGROUND};
    border: 1px solid {BORDER_COLOR};
    border-radius: 4px;
    padding: 5px 10px;
    min-width: 80px;
    color: {TEXT_COLOR};
}}
QComboBox:hover {{
    border-color: {ACCENT_GREEN};
}}
QComboBox::drop-down {{
    border: none;
    width: 20px;
}}
QComboBox::down-arrow {{
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 5px solid {TEXT_COLOR};
    margin-right: 5px;
}}
QComboBox QAbstractItemView {{
    background-color: {PANEL_BACKGROUND};
    border: 1px solid {BORDER_COLOR};
    selection-background-color: {ACCENT_GREEN};
    color: {TEXT_COLOR};
}}
QSpinBox, QDoubleSpinBox {{
    background-color: {PANEL_BACKGROUND};
    border: 1px solid {BORDER_COLOR};
    border-radius: 4px;
    padding: 5px 10px;
    color: {TEXT_COLOR};
}}
QSpinBox:hover, QDoubleSpinBox:hover {{
    border-color: {ACCENT_GREEN};
}}
QPushButton {{
    background-color: {PANEL_BACKGROUND};
    border: 1px solid {BORDER_COLOR};
    border-radius: 4px;
    padding: 8px 16px;
    color: {TEXT_COLOR};
}}
QPushButton:hover {{
    background-color: {METER_BACKGROUND};
    border-color: {ACCENT_GREEN};
}}
QPushButton:pressed {{
    background-color: {ACCENT_GREEN};
}}
QGroupBox {{
    border: 1px solid {BORDER_COLOR};
    border-radius: 6px;
    margin-top: 12px;
    padding-top: 10px;
    font-weight: bold;
}}
QGroupBox::title {{
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 8px;
    color: {TEXT_SECONDARY};
}}
"""

# Reed panel stylesheet
REED_PANEL_STYLE = f"""
QFrame#reedPanel {{
    background-color: {PANEL_BACKGROUND};
    border: 1px solid {BORDER_COLOR};
    border-radius: 8px;
}}
QLabel#reedTitle {{
    font-size: 14px;
    font-weight: bold;
    color: {TEXT_SECONDARY};
}}
QLabel#centsLabel {{
    font-size: 28px;
    font-weight: bold;
}}
QLabel#frequencyLabel {{
    font-size: 14px;
    color: {TEXT_SECONDARY};
}}
QLabel#beatLabel {{
    font-size: 12px;
    color: {TEXT_SECONDARY};
}}
"""

# Note display stylesheet
NOTE_DISPLAY_STYLE = f"""
QFrame#noteDisplay {{
    background-color: {PANEL_BACKGROUND};
    border: 1px solid {BORDER_COLOR};
    border-radius: 12px;
}}
QLabel#noteName {{
    font-size: 72px;
    font-weight: bold;
    color: {TEXT_COLOR};
}}
QLabel#octaveLabel {{
    font-size: 36px;
    color: {TEXT_SECONDARY};
}}
QLabel#refFrequency {{
    font-size: 14px;
    color: {TEXT_SECONDARY};
}}
"""

# Spectrum view stylesheet
SPECTRUM_VIEW_STYLE = f"""
QFrame#spectrumView {{
    background-color: {PANEL_BACKGROUND};
    border: 1px solid {BORDER_COLOR};
    border-radius: 8px;
}}
"""


def get_cents_color(cents: float) -> str:
    """Get color based on cents deviation."""
    abs_cents = abs(cents)
    if abs_cents <= 5:
        return ACCENT_GREEN
    elif abs_cents <= 15:
        return WARNING_ORANGE
    else:
        return ERROR_RED


def get_tuning_gradient(cents: float) -> str:
    """Get gradient for tuning meter based on cents position."""
    color = get_cents_color(cents)
    return f"qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 {METER_BACKGROUND}, stop:0.5 {color}, stop:1 {METER_BACKGROUND})"
