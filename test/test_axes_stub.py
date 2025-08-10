#!/usr/bin/env python3
"""Axis descriptor test stub.

This is a lightweight host-side sanity helper for the RP2040 firmware's
unified axis descriptor system. It does NOT talk to hardware directly;
instead it documents expected invariants so future automated integration
(e.g. via a custom USB HID diagnostic report) can assert correctness.

Planned future upgrade: extend firmware to expose a diagnostic feature
report returning the active axis table (mirroring AxisDescriptor) so the
checks below become live tests instead of static expectations.
"""

import re
from pathlib import Path

AXIS_FILE = Path(__file__).parent.parent / 'src' / 'config' / 'ConfigAxis.h'

RE_DESCRIPTOR_ARRAY = re.compile(r'static\s+const\s+AxisDescriptor\s+axisDescriptors\s*\[\]\s*=\s*{', re.MULTILINE)
RE_ENTRY = re.compile(r'\{\s*AnalogAxisManager::AXIS_([A-Z0-9]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*}\s*,?')

EXPECTED_AXIS_ORDER = ["X","Y","Z","RX","RY","RZ","S1","S2"]


def extract_descriptor_entries(text: str):
    in_array = False
    entries = []
    for line in text.splitlines():
        if RE_DESCRIPTOR_ARRAY.search(line):
            in_array = True
            continue
        if in_array and line.strip().startswith('};'):
            break
        if in_array:
            m = RE_ENTRY.search(line)
            if m:
                entries.append(m.group(1))
    return entries


def test_axis_descriptor_array_exists():
    content = AXIS_FILE.read_text(encoding='utf-8')
    assert RE_DESCRIPTOR_ARRAY.search(content), "axisDescriptors[] definition not found"


def test_axis_entries_are_unique_and_ordered():
    content = AXIS_FILE.read_text(encoding='utf-8')
    axes = extract_descriptor_entries(content)
    # Duplicates
    assert len(axes) == len(set(axes)), f"Duplicate axis descriptors found: {axes}"
    # Order should follow EXPECTED order for present subset
    filtered_expected = [a for a in EXPECTED_AXIS_ORDER if a in axes]
    assert axes == filtered_expected, f"Axis order mismatch. Got {axes}, expected {filtered_expected}"


def test_no_legacy_macros():
    content = AXIS_FILE.read_text(encoding='utf-8')
    assert 'AXIS_CFG' not in content, 'Legacy macro AXIS_CFG still present'
    assert 'AXIS_CONFIG_LIST' not in content, 'Legacy macro AXIS_CONFIG_LIST still present'

if __name__ == '__main__':  # rudimentary manual run
    import pytest, sys
    sys.exit(pytest.main([__file__]))
