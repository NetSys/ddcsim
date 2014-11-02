from sys import stdin
from decimal import Decimal
from string import split

separator = ","
newline = "\n"
window_size = Decimal(0.05) # measured in seconds
unit= Decimal(2 ** 10) # kilobytes

# TODO change to a streaming form instead?
def main():
    index_to_bytes = {}

    for l in stdin:
        s = l.split(separator)
        time = Decimal(s[0])
        size = Decimal(s[1])

        index = int(time / window_size)

        if index in index_to_bytes:
            index_to_bytes[index] += size
        else:
            index_to_bytes[index] = size

    for t,total in index_to_bytes.iteritems():
        print str(float(t * window_size)) + separator + \
            str(float(total / window_size / unit))

if __name__ == "__main__":
    main()
