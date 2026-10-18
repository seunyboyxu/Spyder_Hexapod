#!/usr/bin/env python3
"""Compile the real Uno image using scripts/bootstrap-avr.py's toolchain."""
import os
import pathlib
import subprocess

root = pathlib.Path(__file__).resolve().parents[1]
tools = root / '.tools'
prefix = tools / 'avr/usr/bin'
core = tools / 'avr/usr/share/arduino/hardware/arduino/avr'
servo = tools / 'Servo/src'
source = root / 'firmware/spyder_uno'
build = root / '.pio/local-uno'
build.mkdir(parents=True, exist_ok=True)
environment = dict(os.environ, PATH=str(prefix)+os.pathsep+os.environ['PATH'])
common = ['-Os','-g','-ffunction-sections','-fdata-sections','-mmcu=atmega328p','-DF_CPU=16000000L','-DARDUINO=10819','-DARDUINO_AVR_UNO','-DARDUINO_ARCH_AVR', '-I'+str(core/'cores/arduino'),'-I'+str(core/'variants/standard'),'-I'+str(core/'libraries/EEPROM/src'),'-I'+str(servo),'-I'+str(source)]
def run(command):
    subprocess.run([str(x) for x in command], check=True, env=environment)
objects = []
sources = sorted((core/'cores/arduino').glob('*')) + sorted(source.glob('*.cpp')) + [source/'spyder_uno.ino',servo/'avr/Servo.cpp']
for index, path in enumerate(sources):
    if path.suffix not in ('.c','.cpp','.S','.ino'):
        continue
    obj = build / (str(index)+'-'+path.name+'.o')
    if path.suffix in ('.cpp','.ino'):
        compiler='avr-g++'; flags=['-std=gnu++11','-fno-exceptions','-fno-threadsafe-statics','-fpermissive']
    else:
        compiler='avr-gcc'; flags=['-std=gnu11'] if path.suffix=='.c' else ['-x','assembler-with-cpp']
    if path.suffix=='.ino': flags += ['-x','c++']
    run([prefix/compiler,*common,*flags,'-Wall','-Wextra','-c',path,'-o',obj])
    objects.append(obj)
elf = build/'spyder_uno.elf'
run([prefix/'avr-g++','-Os','-mmcu=atmega328p','-Wl,--gc-sections',*objects,'-lm','-o',elf])
run([prefix/'avr-objcopy','-O','ihex','-R','.eeprom',elf,build/'spyder_uno.hex'])
run([prefix/'avr-size','--format=avr','--mcu=atmega328p',elf])
size = subprocess.check_output([str(prefix/'avr-size'),str(elf)],text=True,env=environment).splitlines()[1].split()
text,data,bss=map(int,size[:3])
if text+data>32256 or data+bss>1536:
    raise SystemExit('Uno flash limit or 512-byte minimum stack margin exceeded')
print('Uno image compiled; at least 512 bytes reserved for stack/runtime')
