#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Genetic programming driver.

Operations:
    init
"""

import sys
import os
import os.path

instructions = [
    "itof",
    "ftoi",
    "inc",
    "dec",
    "load",
    "save",
    "swap",
    "cmp",
    "label",
    "gotoifp",
    "jumpifn",
    "jumphere",
    "loop",
    "endloop",
    "input",
    "output",
    "add",
    "sub",
    "mul",
    "div",
    "abs",
    "sign",
    "exp",
    "log",
    "sin",
    "cos",
    "pow",
    "ran"
]

max_i = 0xffff

population_size = 16
populations_path = "bin/populations"

def main():
    if 1 >= len(sys.argv):
        print("usage: {0} <action>\nactions:\n\tcreate".format(sys.argv[0]))
        return 1

    if "create" == sys.argv[1]:
        create()
        return 0

def create():
    population_path = os.path.realpath(os.path.join(populations_path, "0"))
    print("Creating new population with size {0} at {1}".format(population_size, population_path))
    if not os.path.exists(population_path):
        os.makedirs(population_path)

    for i in range(population_size):
        program_file = os.path.join(population_path, "{0:x}.sla".format(i))
        print("Writing {0}".format(program_file))
        with open(program_file, mode="w") as f:
            write_program(f)


if "__main__" == __name__:
    main()
