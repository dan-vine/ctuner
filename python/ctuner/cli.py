"""
Command-line interface for the tuner
"""

import time
from .tuner import Tuner
from .pitch_detector import PitchResult
from .constants import A4_REFERENCE


def main():
    """Simple command-line tuner"""

    def on_pitch(result: PitchResult):
        if result.valid:
            # Build cents bar: |-------|O|-------|
            bar_width = 30
            center = bar_width // 2
            pos = int(center + (result.cents / 50) * center)
            pos = max(0, min(bar_width - 1, pos))

            bar = ["-"] * bar_width
            bar[center] = "|"
            bar[pos] = "O"

            cents_str = f"{result.cents:+5.1f}"
            print(
                f"\r{result.note_name:2}{result.octave} "
                f"{result.frequency:7.2f} Hz "
                f"[{''.join(bar)}] "
                f"{cents_str} cents  ",
                end="",
                flush=True,
            )

    tuner = Tuner(callback=on_pitch)

    print("Instrument Tuner")
    print("================")
    print(f"Reference: {A4_REFERENCE} Hz")
    print("Press Ctrl+C to exit\n")

    try:
        tuner.start()
        while True:
            time.sleep(0.1)
    except KeyboardInterrupt:
        print("\n\nStopping...")
    finally:
        tuner.stop()


if __name__ == "__main__":
    main()
