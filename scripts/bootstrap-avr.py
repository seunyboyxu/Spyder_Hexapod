#!/usr/bin/env python3
"""Install a verified Debian AVR build toolchain locally; no root access needed."""
import hashlib
import lzma
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
TOOLS = ROOT / '.tools'
CACHE = TOOLS / 'downloads'
CACHE.mkdir(parents=True, exist_ok=True)

def run(*args):
    subprocess.run([str(a) for a in args], check=True)

def download(url, path):
    run('curl', '--fail', '--silent', '--show-error', '--location', '--proto', '=https', url, '-o', path)

release = CACHE / 'InRelease'
index = CACHE / 'Packages.xz'
download('https://deb.debian.org/debian/dists/trixie/InRelease', release)
keyhome = TOOLS / 'gpg'
keyhome.mkdir(mode=0o700, exist_ok=True)
run('gpg', '--no-options', '--homedir', keyhome, '--no-default-keyring', '--keyring', '/usr/share/keyrings/debian-archive-keyring.gpg', '--verify', release)
download('https://deb.debian.org/debian/dists/trixie/main/binary-amd64/Packages.xz', index)
expected = [hashlib.sha256(index.read_bytes()).hexdigest(), str(index.stat().st_size), 'main/binary-amd64/Packages.xz']
if not any(line.split() == expected for line in release.read_text().splitlines()):
    raise SystemExit('Signed package index checksum mismatch')
versions = {'gcc-avr': '1:14.2.0-2', 'binutils-avr': '2.43.50.20250108-1', 'avr-libc': '1:2.2.1-1', 'arduino-core-avr': '1.8.7+dfsg-1~deb13u1'}
destination = TOOLS / 'avr'
destination.mkdir(exist_ok=True)
found = set()
for block in lzma.open(index, 'rt').read().split('\n\n'):
    fields = dict(line.split(': ', 1) for line in block.splitlines() if ': ' in line and not line.startswith(' '))
    name = fields.get('Package')
    if name not in versions or fields.get('Version') != versions[name]:
        continue
    archive = CACHE / pathlib.Path(fields['Filename']).name
    download('https://deb.debian.org/debian/' + fields['Filename'], archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != fields['SHA256']:
        raise SystemExit('Package checksum mismatch: ' + name)
    run('dpkg-deb', '-x', archive, destination)
    found.add(name)
if found != set(versions):
    raise SystemExit('Pinned packages missing from signed index: ' + str(set(versions)-found))
servo = TOOLS / 'Servo'
if not servo.exists():
    run('git', 'clone', '--quiet', '--depth', '1', '--branch', '1.2.2', 'https://github.com/arduino-libraries/Servo.git', servo)
commit = subprocess.check_output(['git', '-C', str(servo), 'rev-parse', 'HEAD'], text=True).strip()
if commit != '8297c322120d9eb21478eb48f1f3d80219b9d800':
    raise SystemExit('Servo 1.2.2 commit mismatch')
print('Verified local AVR toolchain and Servo 1.2.2 ready')
