from string import split
from decimal import Decimal
import yaml

topology_in_file = "sam0.yaml"
topology_out_file = "sw.yaml"
events_in_file = "sam.tr"
events_out_file = "failures.yaml"
controller_count = 20
host_count = 1000
switch_count = 10000

def to_mw(raw):
    t = raw[0]
    id = int(but_first(raw)) - 1
    if t is 'h':
        id += switch_count + controller_count
    elif t is 'c':
        id += switch_count
    else:
        assert t is 's'
    return id

def but_first(s):
    return s[1:len(s)]

def main():
    # TODO don't hardcode entities

    topo_out = open(topology_out_file, 'w+')

    print >>topo_out, 'entities:'

    for s in range(0, switch_count):
        print >>topo_out, '  - {id: ' + str(s) + ', type: switch}'

    for c in range(switch_count, switch_count + controller_count):
        print >>topo_out, '  - {id: ' + str(c) + ', type: controller}'

    for h in range(switch_count + controller_count, switch_count + controller_count + host_count):
        print >>topo_out, '  - {id: ' + str(h) + ', type: host}'

    print >>topo_out, 'links:'

    yaml_in = yaml.load(open(topology_in_file, 'r'))

    links = yaml_in['links']

    entity_to_out_link = {}

    for l in links:
        cur = str(l).split('-')
        raw_src = cur[0]
        raw_dst = cur[1]
        src = to_mw(raw_src)
        dst = to_mw(raw_dst)

        if src in entity_to_out_link:
            entity_to_out_link[src].append(dst)
        else:
            entity_to_out_link[src] = [dst]
        if dst in entity_to_out_link:
            entity_to_out_link[dst].append(src)
        else:
            entity_to_out_link[dst] = [src]

    for src, out_vertices in entity_to_out_link.iteritems():
        print >>topo_out, '  ' + str(src) + ': ' + str(out_vertices)

    events_in = open(events_in_file, 'r')
    events_out = open(events_out_file, 'w+')

    down_links = set()

    for line in events_in:
        cur = line.split(' ')
        time = Decimal(cur[0])
        if time == 0:
            continue
        elif cur[1] == 'end\n':
            continue
        else:
            l = cur[1].split('-')
            src = to_mw(l[0])
            dst = to_mw(l[1])
            state = cur[2]

            if state == 'up\n':
                assert (src, dst) in down_links
                assert (dst, src) in down_links
                down_links.remove((src,dst))
                down_links.remove((dst,src))
                event_type = 'linkup'
            else:
                assert (src, dst) not in down_links
                assert (dst, src) not in down_links
                assert state == 'down\n'
                down_links.add((src, dst))
                down_links.add((dst, src))
                event_type = 'linkdown'

            print >>events_out, str(time) + ':'
            print >>events_out, '  {src_id: ' + str(src) + ', dst_id: ' + str(dst) + ', type: ' + event_type + '}'
            print >>events_out, str(time) + ':'
            print >>events_out, '  {src_id: ' + str(dst) + ', dst_id: ' + str(src) + ', type: ' + event_type + '}'


if __name__ == "__main__":
    main()
