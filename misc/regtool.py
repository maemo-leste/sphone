import sys
import re

from regdata import *

def fileiter(fp):
    for line in fp:
        if not line.strip():
            continue

        k, v = line.split(':')
        k = k.strip()
        v = v.strip()
        yield k, v

def file_to_vals(fp):
    d = {}
    for k, v in fileiter(fp):
        d[k] = v
    return d

def file_to_regs(fp):
    d = {}
    for k, v in fileiter(fp):
        v = int(v, 16)
        if k in AUDIO_REGS:
            name = AUDIO_REG_NAMES[AUDIO_REGS.index(k)]
            d[name] = {}
            #print(name, hex(v))
            if name in AUDIO_REG_BITS:
                for regname, regpos in AUDIO_REG_BITS[name].items():
                    d[name][regname] = (v & (1 << regpos)) >> regpos
                    #if v & (1 << regpos) > 0:
                    #    print('\t', regname, 'ON')
                    #else:
                    #    print('\t', regname, 'OFF')
    return d

mode = sys.argv[1]

FILE = '/sys/kernel/debug/regmap/spi0.0/registers'

if mode == 'dump':
    fp = open(FILE, 'r')
    if len(sys.argv) > 2:
        fp2 = open(sys.argv[2], 'w+')
        fp2.write(fp.read())
        fp.close()
        fp2.close()
    else:
        print(fp.read())
        fp.close()
elif mode == 'info':
    fp = open(FILE, 'r')
    data = file_to_regs(fp)
    fp.close()
    from pprint import pprint
    pprint(data)
elif mode == 'cmp':
    f1 = sys.argv[2]
    f2 = sys.argv[3]
    fp1 = open(f1, 'r')
    fp2 = open(f2, 'r')
    d1 = file_to_regs(fp1)
    d2 = file_to_regs(fp2)
    fp1.close()
    fp2.close()

    for regname, regs1 in d1.items():
        regs2 = d2[regname]
        for k in regs1:
            if regs1[k] != regs2[k]:
                print('Difference in %s.%s: %d != %d' % (regname, k, regs1[k], regs2[k]))
        
elif mode == 'restore':
    f1 = sys.argv[2]
    fp1 = open(f1, 'r')
    fp2 = open(FILE, 'r')

    d1 = file_to_regs(fp1)
    d2 = file_to_regs(fp2)
    fp1.seek(0)
    filevals = file_to_vals(fp1)
    fp1.close()
    fp2.close()

    val = 0
    for regname, regs1 in d1.items():
        regs2 = d2[regname]
        diff = False
        for k in regs1:
            if regs1[k] != regs2[k]:
                diff = True
                print('Difference in %s.%s: %d != %d' % (regname, k, regs1[k], regs2[k]))

        if diff:
            reg_index = AUDIO_REG_NAMES.index(regname)
            reg = AUDIO_REGS[reg_index]
            print('Restoring:', reg, filevals[reg])
            fp = open(FILE, 'w')
            fp.write('%s %s' % (reg, filevals[reg]))
            fp.close()
elif mode == 'get':
    to_set = sys.argv[2]

    fp = open(FILE, 'r')
    data = file_to_regs(fp)

    reg_name, reg_bit_name = sys.argv[2].split('.')

    if reg_name not in AUDIO_REG_NAMES:
        print('Unknown reg')
        sys.exit(1)

    reg = AUDIO_REG_NAMES.index(reg_name)
    reg_bits = AUDIO_REG_BITS[reg_name]

    if reg_bit_name not in reg_bits:
        print('Unknown reg bit')
        sys.exit(1)

    cur_value = data[reg_name][reg_bit_name]
    print(cur_value)
elif mode == 'set':
    to_set = sys.argv[2]
    value = int(sys.argv[3])

    fp = open(FILE, 'r')
    data = file_to_regs(fp)
    fp.seek(0)
    filevals = file_to_vals(fp)
    fp.close()

    reg_name, reg_bit_name = sys.argv[2].split('.')

    if reg_name not in AUDIO_REG_NAMES:
        print('Unknown reg')
        sys.exit(1)

    reg_index = AUDIO_REG_NAMES.index(reg_name)
    reg_bits = AUDIO_REG_BITS[reg_name]

    if reg_bit_name not in reg_bits:
        print('Unknown reg bit')
        sys.exit(1)

    cur_value = data[reg_name][reg_bit_name]

    if cur_value != value:
        print('have to change value')
        reg_bit_index = AUDIO_REG_BITS[reg_name][reg_bit_name]

        if value == 1:
            val = int(filevals[AUDIO_REGS[reg_index]],16) | (1 << reg_bit_index)
        else:
            val = int(filevals[AUDIO_REGS[reg_index]],16) & ~(1 << reg_bit_index)

        fp = open(FILE, 'w')
        s = '%s %s' % (AUDIO_REGS[reg_index], hex(val)[2:])
        fp.write('%s %s' % (AUDIO_REGS[reg_index], hex(val)[2:]))
        fp.close()
