"""
Ensures PyQt5 runs headless (no real display needed) for any test that
constructs a QWidget - must be set before PyQt5 is imported anywhere.
"""
import os

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
