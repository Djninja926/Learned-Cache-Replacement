import struct
import sys

INPUT  = sys.argv[1]
OUTPUT = sys.argv[2]
LIMIT  = int(sys.argv[3]) if len(sys.argv) > 3 else 10_000_000

RECORD_SIZE = 64

# Struct layout (CRC-2 / old ChampSim format):
# offset 0:  uint64_t ip                          (8 bytes)
# offset 8:  uint8_t  is_branch                   (1 byte)
# offset 9:  uint8_t  branch_taken                (1 byte)
# offset 10: uint8_t  destination_registers[2]    (2 bytes)
# offset 12: uint8_t  source_registers[4]         (4 bytes)  
# offset 16: uint64_t destination_memory[2]       (16 bytes)
# offset 32: uint64_t source_memory[3]            (24 bytes)
# offset 56: padding                              (8 bytes)

with open(INPUT, 'rb') as fin, open(OUTPUT, 'w') as fout:
    count = 0
    skipped = 0
    while count < LIMIT:
        record = fin.read(RECORD_SIZE)
        if len(record) < RECORD_SIZE:
            break
        ip       = struct.unpack_from('<Q', record, 0)[0]
        dst_mem  = struct.unpack_from('<Q', record, 16)[0]  # first destination memory
        src_mem  = struct.unpack_from('<Q', record, 32)[0]  # first source memory

        # Use whichever memory address is non-zero
        # dst_mem = stores, src_mem = loads
        addr = src_mem if src_mem != 0 else dst_mem
        if addr == 0:
            skipped += 1
            continue  # skip non-memory instructions

        type_ = 1 if dst_mem != 0 else 0  # 1=store, 0=load
        fout.write(f'{ip:#x} {addr:#x} {type_}\n')
        count += 1

    print(f'Converted {count} memory accesses, skipped {skipped} non-memory instructions')