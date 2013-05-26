#! /usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Genetic programming driver.

Operations:
    init
    step
"""

import sys
import os.path
import subprocess
import random
import re
import glob
import signal
import time

class ProgramLog:
    def __init__(self, program_file):
        self.program_file = program_file

        self.ticks = 0
        self.hp = 0
        self.direct_hits = 0
        self.hits = 0
        self.got_direct_hits = 0
        self.got_hits = 0

        with open(program_file + ".log", mode="r") as f:
            self.ticks = int(readline(f))
            self.hp = int(readline(f))
            self.direct_hits = int(readline(f))
            self.hits = int(readline(f))
            self.got_direct_hits = int(readline(f))
            self.got_hits = int(readline(f))

    def get_fitness(self):
        return self.ticks + \
               self.hp + \
               self.direct_hits + \
               self.hits + \
               self.got_direct_hits + \
               self.got_hits

instructions = [
    "itof", "ftoi", "inc", "dec", "load", "save", "swap", "cmp", "label",
    "gotoifp", "jumpifn", "jumphere", "loop", "endloop", #"input", "output",
    "add", "sub", "mul", "div", "abs", "sign", "exp", "log", "sin", "cos",
    "pow", "ran", "SetEnginePower", "Shoot", "Turn", "LookAt"
]
instruction_count = len(instructions)

max_i = 0xffff

population_size = 16
populations_path = "bin/populations"

min_program_length = 256
max_program_length = 2048

morrigan_server = "bin/morrigan.exe"
morrigan_client = "bin/morrigan_genetic_client"

def main():
    if 1 >= len(sys.argv):
        print("usage: {0} <action>\nactions:\n\tinit\n\tstep".format(sys.argv[0]))
        return 1

    random.seed()

    if "init" == sys.argv[1]:
        initialize_population()
        return 0
    elif "step" == sys.argv[1]:
        genetic_step()
        return 0
    else:
        print("Unknown operation. See usage for help.")
        return -1

def genetic_step():
    subdirectories = [ subdir for subdir in os.listdir(populations_path) if os.path.isdir(os.path.join(populations_path, subdir)) ]
    subdirectories = list(filter(lambda subdir: None != re.match(r"^\d+$", subdir), subdirectories))

    if 0 == len(subdirectories):
        print("No populations found. Nothing to do here.")
        return -1

    last_population = max(subdirectories, key=lambda subdir: int(subdir))
    population_path = os.path.realpath(os.path.join(populations_path, last_population))
    print("Wokring with population at {0}".format(population_path))

    programs = glob.glob(os.path.join(population_path, "*.sla"))
    if population_size != len(programs):
        print("Bad program count.")
        return -1

    server_pid = start_morrigan_server()
    time.sleep(2.0)

    program_pids = [ start_morrigan_client(p) for p in programs ]

    print("Waiting for clients to terminate.")
    for pid in program_pids:
        pid.wait()

    server_pid.send_signal(signal.SIGINT)
    server_pid.wait()

    fitnesses = list(map(lambda p: ProgramLog(p), programs))

    # TODO: Perform selection.
    # TODO: Perform crossover.
    # TODO: Perform mutation.

def initialize_population():
    population_path = os.path.realpath(os.path.join(populations_path, "0"))
    print("Creating new population with size {0} at {1}".format(population_size, population_path))
    if not os.path.exists(population_path):
        os.makedirs(population_path)

    for i in range(population_size):
        program_file = os.path.join(population_path, "{0:x}.sla".format(i))
        print("Writing {0}".format(program_file))
        with open(program_file, mode="w") as f:
            write_program(f)

def write_program(f):
    program_length = random.randint(min_program_length, max_program_length)

    for i in range(program_length):
        instruction_index = random.randint(0, instruction_count)

        if instruction_index < instruction_count:
            f.write(instructions[instruction_index])
        else:
            f.write(str(random.randint(0, max_i)))

        if i != program_length - 1:
            f.write("/")

    f.write(".\n")

def start_morrigan_server():
    print("Starting {0}".format(morrigan_server))
    return subprocess.Popen(morrigan_server, cwd=os.path.dirname(morrigan_server))

def start_morrigan_client(program):
    client_command = "{0} {1} {2}".format(morrigan_client,
                                          program,
                                          "localhost")
    print("Starting {0}".format(client_command))
    return subprocess.Popen(client_command)

if "__main__" == __name__:
    main()
