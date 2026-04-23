PC_LOOP = 0x400100
PC_SCAN = 0x400200

LOOP_START = 0x100000
SCAN_START = 0x800000

LOOP_WORKING_SET = 204
SCAN_WORKING_SET = 4096
EPOCHS = 2000

with open('traces/test.txt', 'w') as f:
    for epoch in range(EPOCHS):
        
        for _ in range(5):
            for i in range(LOOP_WORKING_SET):
                addr = LOOP_START + (i * 64)
                f.write(f'{PC_LOOP:#x} {addr:#x} 0\n')
                
        for i in range(SCAN_WORKING_SET):
            addr = SCAN_START + (i * 64)
            f.write(f'{PC_SCAN:#x} {addr:#x} 0\n')