#!/bin/bash

#tail -n +10 | sed -r -e 's/^.{1}/&,/' | python gg_to_yaml.py
tail -n +10 | python gg_to_yaml.py
