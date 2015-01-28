from sys import stdin
from decimal import Decimal
from string import split

separator = ","
newline = "\n"
interval=Decimal(0.05)

min = 256
max = 1328.190461

def main():
    for l in stdin:
        s = l.split(separator)
        cur_time = Decimal(s[0])
        if not (min < cur_time and cur_time < max):
            continue
        virt = Decimal(s[1])
        phys = Decimal(s[2])

        print str(cur_time) + separator + str(float(virt/phys))

if __name__ == "__main__":
    main()
