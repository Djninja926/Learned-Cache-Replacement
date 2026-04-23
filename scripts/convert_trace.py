import struct
import sys

INPUT = sys.argv[1]
OUTPUT = sys.argv[2]
LIMIT = int(sys.argv[3]) if len(sys.argv) > 3 else 10_000_000

RECORD_SIZE = 64

with open(INPUT, 'rb') as fin, open(OUTPUT, 'w') as fout:
    count = 0
    skipped = 0
    while count < LIMIT:
        record = fin.read(RECORD_SIZE)
        if len(record) < RECORD_SIZE:
            break
        ip = struct.unpack_from('<Q', record, 0)[0]
        dst_mem  = struct.unpack_from('<Q', record, 16)[0]
        src_mem  = struct.unpack_from('<Q', record, 32)[0]

        addr = src_mem if src_mem != 0 else dst_mem
        if addr == 0:
            skipped += 1
            continue

        type_ = 1 if dst_mem != 0 else 0
        fout.write(f'{ip:#x} {addr:#x} {type_}\n')
        count += 1

    print(f'Converted {count} memory accesses, skipped {skipped} non-memory instructions')