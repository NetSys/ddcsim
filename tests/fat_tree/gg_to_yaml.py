from sys import stdin, exit
from decimal import Decimal
from string import split

separator = " "
newline = "\n"

def but_last(str):
    return str[0:len(str)-1]

def main():
    entities = []
    links = {}

    for l in stdin:
        if l[0] is 'n':
            s = l.split(separator)
            node_id = but_last(s[1])
            entities.append("  - {id: " + node_id + ", type: switch}")
        elif l[0] is 'e':
            s = l.split(separator)
            src = int(but_last(s[1]))
            dst = int(but_last(s[2]))
            if src in links:
                links[src].append(dst)
            else:
                links[src] = [dst]
            if dst in links:
                links[dst].append(src)
            else:
                links[dst] = [src]
        else:
            print "FATAL ERROR"
            exit()

    print 'entities:'

    for e in entities:
        print e

    print 'links:'

    for k,v in links.iteritems():
        print "  " + str(k) + ": " + str(v)

if __name__ == "__main__":
    main()
