# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A musical instrument strobe tuner with multi-pitch detection, temperament support, and specialized accordion tuning features. The project has implementations in multiple languages:

- **python/** - Main development focus: PySide6 GUI application with pure numpy pitch detection
- **swift/** - macOS native app
- **windows/** - Windows native app (C++)
- **linux/** - Linux native app (C/GTK)
- **cli/** - C command-line tool

## Python Development

### Setup
```bash
cd python
python3 -m venv .venv
source .venv/bin/activate
pip install -e ".[dev]"
```

### Commands
```bash
# Run the accordion tuner GUI
python -m ctuner.gui.accordion_window

# Run CLI tuner
python -m ctuner.cli

# Run all tests
pytest

# Run specific test file
pytest tests/test_pitch_detector.py

# Run specific test
pytest tests/test_pitch_detector.py::TestPitchDetectorBasic::test_a4_440hz -v

# Lint
ruff check ctuner/

# Lint and auto-fix
ruff check --fix ctuner/
```

### Architecture

**Pitch Detection** (`multi_pitch_detector.py`):
- FFT + phase vocoder algorithm (pure numpy, no C dependencies)
- Detects multiple simultaneous pitches
- 16384-sample FFT with 1024-sample hop size
- Phase difference between frames provides sub-bin frequency accuracy

**Accordion Detection** (`accordion.py`):
- Wraps `MultiPitchDetector` with accordion-specific settings
- Detects 1-4 reeds playing the same note with slight detuning (tremolo/musette)
- Calculates beat frequencies between reed pairs

**Temperaments** (`temperaments.py`):
- 32 historical temperaments ported from C++ `built_in_temperaments.h`
- Each temperament: 12 frequency ratios relative to tonic (C through B)
- `TEMPERAMENT_NAMES` dict provides UI display names

**GUI** (`gui/`):
- `accordion_window.py` - Main window, audio capture, settings panel
- `reed_panel.py` - Individual reed cents display
- `tuning_meter.py` - Horizontal cents deviation meter
- `spectrum_view.py` - FFT spectrum visualization
- `note_display.py` - Current note name display
- `styles.py` - Color constants and theming

### Key Constants (`constants.py`)
- `SAMPLE_RATE = 44100`
- `BUFFER_SIZE = 1024`
- `FFT_SIZE = 16384`
- `A4_REFERENCE = 440.0`

## C++ Temperament Data

When adding/modifying temperaments, the authoritative source is:
`windows/src/tuning/built_in_temperaments.h`

## Testing

Tests use synthetic sine waves at known frequencies to verify:
- Note detection accuracy
- Cents calculation
- Temperament adjustments
- Multi-pitch detection

Test data files (WAV) are generated in `tests/test_data/` - this directory is gitignored.