from sys import stdin
from decimal import Decimal
from string import split
from math import ceil

min = 256
max = False

separator = ","
unit_GB = Decimal(2 ** 30) # GB
unit_MB = Decimal(2 ** 20) # MB
unit_KB = Decimal(2 ** 10) # KB

lsu_size = Decimal(20 + 4 + 6 + 13 * 6 + 8)         # 116    bytes
lsr_size = Decimal(20 + 4 + 6)                      # 30     bytes
ru_size = Decimal(20 + 4 + 6 + 2000 * 6 + 6)        # 12,036 bytes
cv_size = Decimal(20 + 4 + 6 + 2000 * 6 + 2000 * 6) # 24,030 bytes

total_bw_file_name = "tmp/total_bw.txt"
lsr_bw_file_name = "tmp/lsr_bw.txt"
lsu_from_lsr_bw_file_name = "tmp/lsu_from_lsr_bw.txt"
cv_bw_file_name = "tmp/cv_bw.txt"

# 1327.363711,9,14
num_link_events = Decimal(9 + 14)
num_links = Decimal(16296)

def main():
    total_bw_file = open(total_bw_file_name, 'w')
    lsr_bw_file = open(lsr_bw_file_name, 'w')
    lsu_from_lsr_bw_file = open(lsu_from_lsr_bw_file_name, 'w')
    cv_bw_file = open(cv_bw_file_name, 'w')

    cum_lsu = 0
    cum_lsr = 0
    cum_ru  = 0
    cum_cv  = 0

    for l in stdin:
        s = l.split(separator)
        cur_left = Decimal(s[0])
        if min and cur_left < min:
            continue
        cur_right = Decimal(s[1])
        if max and cur_right > max:
            continue
        lsu = Decimal(Decimal(s[2]) * lsu_size)
        ru = Decimal(Decimal(s[3]) * ru_size)
        lsr = Decimal(Decimal(s[4]) * lsr_size)
        cv = Decimal(Decimal(s[5]) * cv_size)
        lsr_cause = Decimal(Decimal(s[6].strip()) * lsu_size)

        prev = cum_lsu
        cum_lsu += int(s[2])
        assert cum_lsu >= prev

        prev = cum_ru
        cum_ru += int(s[3])
        assert cum_ru >= prev

        prev = cum_lsr
        cum_lsr += int(s[4])
        assert cum_lsr >= prev

        prev = cum_cv
        cum_cv += int(s[5])
        assert cum_cv >= prev

        print >>total_bw_file, str(cur_right) + separator + str(Decimal(Decimal(lsu + ru + lsr) / unit_GB))
        print >>lsr_bw_file, str(cur_right) + separator + str(Decimal(lsr / unit_MB))
        print >>lsu_from_lsr_bw_file, str(cur_right) + separator + str(Decimal(lsr_cause / unit_GB))
        print >>cv_bw_file, str(cur_right) + separator + str(Decimal(cv / unit_GB))


    lsu_density = Decimal(cum_lsu) / num_link_events / num_links
    ru_density = Decimal(cum_ru) / num_link_events / num_links
    lsr_density = Decimal(cum_lsr) / num_link_events / num_links
    cv_density = Decimal(cum_cv) / num_link_events / num_links
    total_density = lsu_density + ru_density + lsr_density + cv_density
    total_size = (cum_lsu * lsu_size + cum_ru * ru_size + cum_lsr * lsr_size + cum_cv * cv_size) / num_link_events / num_links

    print "lsu count / link events / links=" + str(lsu_density)
    print "lsu bytes / link events / links=" + str(lsu_density * lsu_size / unit_MB) + " MB"

    print "ru count / link events / links=" + str(ru_density)
    print "ru bytes / link events / links=" + str(ru_density * ru_size / unit_GB) + " GB"

    print "lsr count / link events / links=" + str(lsr_density)
    print "lsr bytes / link events / links=" + str(lsr_density * lsr_size / unit_KB) + " KB"

    print "cv count / link events / links=" + str(cv_density)
    print "cv bytes / link events / links=" + str(cv_density * cv_size / unit_GB) + " GB"

    print "total count / link events / links=" + str(total_density)
    print "total bytes / link events / links=" + str(total_size / unit_GB) + " GB"

if __name__ == "__main__":
    main()
