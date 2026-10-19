#!/usr/bin/env python3
"""Execute the real Uno ELF with simavr and assert GPIO regression behavior."""
import argparse
import concurrent.futures
import csv
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--simavr-root', type=Path,
                    help='Extracted Debian simulator packages; omit for system install')
parser.add_argument('--elf', type=Path, default=root / '.pio/local-uno/spyder_uno.elf')
parser.add_argument('--output', type=Path, default=root / '.pio/avr-tests')
parser.add_argument('--bench-min', type=int, default=1000)
parser.add_argument('--bench-max', type=int, default=2000)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)

with tempfile.TemporaryDirectory(prefix='spyder-avr-tests-') as directory:
    binary = Path(directory) / 'waveform'
    command = ['cc', '-O2', '-Wall', '-Wextra', str(root / 'tests/avr/waveform.c')]
    if args.simavr_root:
        include = args.simavr_root / 'usr/include'
        lib = args.simavr_root / 'usr/lib/x86_64-linux-gnu'
        command += ['-I' + str(include), '-I' + str(include / 'simavr'),
                    '-L' + str(lib), '-Wl,-rpath,' + str(lib)]
    command += ['-lsimavr', '-l:libelf.so.1', '-o', str(binary)]
    subprocess.run(command, check=True)
    release_modes = ['release-high'] + [f'release-offset-{i}' for i in range(0, 20000, 1000)]
    modes = ['steady', 'reattach', 'serial', 'bench-sweep'] + release_modes

    def simulate(mode):
        invocation = [str(binary), str(args.elf), mode, str(args.output / (mode + '.csv'))]
        if mode == 'bench-sweep':
            invocation += [str(args.bench_min), str(args.bench_max)]
        result = subprocess.run(invocation, check=True, capture_output=True,
                                text=True, timeout=120 if mode == 'bench-sweep' else 20)
        (args.output / (mode + '.log')).write_text(result.stdout)
        return mode, result.stdout

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        results = dict(pool.map(simulate, modes))
    failures = []
    for mode in release_modes:
        highs = re.findall(r'D(\d+):[^\n]*final_level=1', results[mode])
        if highs:
            failures.append(f'{mode}: released pins still HIGH: {highs}')

    # Decode edges independently, including pulses spanning release and rearm.
    with (args.output / 'reattach.csv').open() as source:
        rows = list(csv.DictReader(source))
    rises, long = {}, []
    for row in rows:
        pin, time = int(row['pin']), float(row['time_us'])
        if row['level'] == '1':
            rises[pin] = time
        elif pin in rises:
            width = time - rises.pop(pin)
            if width > 2015:
                long.append((pin, width))
    if long:
        failures.append(f'reattach: abnormally long HIGH intervals: {long}')

    serial = re.findall(r'JOINT \d+ 0 [^\n]+', results['serial'])
    if not serial:
        raise SystemExit('AVR REGRESSIONS FAILED: serial scenario returned no joint readback')
    commanded = int(serial[-1].split()[10])
    if not 1553 <= commanded <= 1565:
        failures.append(f'serial load: expected ~1557us commanded, got {commanded}')

    with (args.output / 'steady.csv').open() as source:
        steady = list(csv.DictReader(source))
    widths, rise, pins = [], {}, set()
    for row in steady:
        if row['phase'] != 'steady':
            continue
        pin, time = int(row['pin']), float(row['time_us'])
        pins.add(pin)
        if row['level'] == '1':
            rise[pin] = time
        elif pin in rise:
            widths.append(time - rise.pop(pin))
    if len(pins) != 12 or not widths or min(widths) < 1485 or max(widths) > 1515:
        failures.append('steady output has missing pins or incorrect 1500us pulses')
    with (args.output / 'bench-sweep.csv').open() as source:
        bench = list(csv.DictReader(source))
    bench_widths, bench_rises, bench_pins = [], {}, set()
    for row in bench:
        if row['phase'] not in ('bench-center', 'bench-sweep'):
            continue
        pin, time = int(row['pin']), float(row['time_us'])
        if row['level'] == '1':
            bench_pins.add(pin)
            bench_rises[pin] = time
        elif pin in bench_rises:
            bench_widths.append(time - bench_rises.pop(pin))
    if (bench_pins != {6} or not bench_widths or
            abs(min(bench_widths) - args.bench_min) > 15 or
            abs(max(bench_widths) - args.bench_max) > 15):
        failures.append('unloaded sweep failed pin isolation or actual endpoint pulse widths')
    if 'BEGIN 62 0 0 4 0' not in results['bench-sweep'] or 'ERR ' in results['bench-sweep']:
        failures.append('unloaded sweep did not complete once and return to center')
    bench_joint = re.findall(r'JOINT 62 4 [^\n]+', results['bench-sweep'])
    if not bench_joint or list(map(int, bench_joint[-1].split()[10:12])) != [1500, 1500]:
        failures.append('unloaded sweep final command/target are not centered')
    if re.findall(r'D(\d+):[^\n]*final_level=1', results['bench-sweep']):
        failures.append('unloaded release left a signal HIGH')
    if failures:
        raise SystemExit('AVR REGRESSIONS FAILED:\n' + '\n'.join(failures))
    print(f'AVR PASS: {len(modes)} scenarios; 21 release phases LOW; rearm pulses bounded; '
          f'serial command {commanded}us; 12 steady channels; '
          f'isolated unloaded sweep {args.bench_min}..{args.bench_max}us')
