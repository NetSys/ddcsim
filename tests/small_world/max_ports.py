import yaml

yaml_in = yaml.load(open("sw.yaml", 'r'))
links = yaml_in['links']

def main():
    max_k = 0
    max_ports = len(links[0])

    for k,v in links.iteritems():
        if len(v) > max_ports:
            max_k = k
            max_ports = len(v)

    print max_k
    print max_ports

if __name__ == "__main__":
    main()
