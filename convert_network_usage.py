from sys import stdin
from decimal import Decimal
from string import split

separator = ","
newline = "\n"

def main():
    time_to_bytes = {}

    for l in stdin:
        s = l.split(separator)
        time = Decimal(s[0])
        size = Decimal(s[1])
        if time in time_to_bytes:
            time_to_bytes[time] += size
        else:
            time_to_bytes[time] = size

    for k,v in time_to_bytes.iteritems():
        print str(k) + separator + str(v)

if __name__ == "__main__":
    main()
