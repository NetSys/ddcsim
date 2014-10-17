from decimal import Decimal
from string import split

in_file_path = "usage_raw.txt"
out_file_path = "usage.txt"

separator = ","
newline = "\n"

def main():
    in_file = open(in_file_path, 'r')
    out_file = open(out_file_path, 'w')
    time_to_bytes = {}

    for l in in_file:
        s = l.split(separator)
        time = Decimal(s[0])
        size = Decimal(s[1])
        if time in time_to_bytes:
            time_to_bytes[time] += size
        else:
            time_to_bytes[time] = size

    for k,v in time_to_bytes.iteritems():
        out_file.write(str(k) + separator + str(v))
        out_file.write(newline)

if __name__ == "__main__":
    main()
