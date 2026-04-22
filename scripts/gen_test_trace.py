import random
# random.seed(42)

# 50 unique cache-line-aligned addresses (lower 6 bits zeroed)
addrs = [random.randint(0, 0xFFFFFF) & ~0x3F for _ in range(50)]

with open('traces/test.txt', 'w') as f:
    for _ in range(10000):
        num = random.choice([0, 1])
        pc   = random.randint(0x400000, 0x500000)
        addr = random.choice(addrs)
        f.write(f'{pc:#x} {addr:#x} {num}\n')

print('Generated traces/test.txt with 10000 accesses over 50 unique addresses')