import random

# Define specific PCs
PC_LOOP = 0x400100
PC_SCAN = 0x400200

# Define memory regions
LOOP_START = 0x100000
SCAN_START = 0x800000

# The loop fits easily in cache
LOOP_WORKING_SET = 200  
SCAN_WORKING_SET = 4000 

with open('traces/test.txt', 'w') as f:
    for i in range(1000000):
        if random.random() < 0.5:
            offset = random.randint(0, LOOP_WORKING_SET - 1) * 64
            addr = LOOP_START + offset
            f.write(f'{PC_LOOP:#x} {addr:#x} 0\n')
            
        else:
            offset = (i % SCAN_WORKING_SET) * 64
            addr = SCAN_START + offset
            f.write(f'{PC_SCAN:#x} {addr:#x} 0\n')