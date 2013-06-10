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
import shutil

class ProgramLog:
    def __init__(self, program_file):
        self.program_file = program_file

        self.ticks = 0
        self.hp = 0
        self.direct_hits = 0
        self.hits = 0
        self.got_direct_hits = 0
        self.got_hits = 0

        program_log_filename = program_file + ".log"

        try:
            with open(program_log_filename, mode='r') as f:
                self.ticks = int(readline(f))
                self.hp = int(readline(f))
                self.direct_hits = int(readline(f))
                self.hits = int(readline(f))
                self.got_direct_hits = int(readline(f))
                self.got_hits = int(readline(f))
        except IOError:
            print("Failed to open program log: {0}.".format(program_log_filename))

    def get_fitness(self):
        return self.ticks + \
               self.hp + \
               self.direct_hits + \
               self.hits + \
               self.got_direct_hits + \
               self.got_hits

class Program:
    def __init__(self, program_filename):
        self.program_filename = program_filename

    def load(self):
        with open(self.program_filename, mode='r') as f:
            self.program = f.readline().strip()

        if '.' != self.program[len(self.program) - 1]:
            raise Exception("Program doesn't ends with '.'")

        self.program_commands = self.program.split('/')

    def save(self):
        with open(self.program_filename, mode='w') as f:
            for c in self.program_commands:
                f.write(c)
                f.write('/')

            f.write('.')

    def crossover(self, another):
        min_len = min(len(self.program_commands), len(another.program_commands))

        p1 = random.randrange(0, min_len)
        while True:
            p2 = random.randrange(0, min_len)
            if p2 != p1: break

        if p1 > p2:
            p1, p2 = p2, p1

        child1 = list(self.program_commands)
        child2 = list(another.program_commands)

        for i in range(p1, p2 + 1):
            child1[i] = another.program_commands[i]
            child2[i] = self.program_commands[i]

        self.program_commands = child1
        another.program_commands = child2

    def mutate(self):
        instruction_index = random.randrange(0, len(self.program_commands))
        target_instruction = random.sample(instructions, 1)[0]
        self.program_commands[instruction_index] = target_instruction

instructions = [
    "itof", "ftoi", "inc", "dec", "load", "save", "swap", "cmp", "label",
    "gotoifp", "jumpifn", "jumphere", "loop", "endloop", #"input", "output",
    "add", "sub", "mul", "div", "abs", "sign", "exp", "log", "sin", "cos",
    "pow", "ran", "SetEnginePower", "Shoot", "Turn", "LookAt", "GetFireDelay"
]
instruction_count = len(instructions)

max_i = 0xffff

population_size = 16
populations_path = "bin/populations"

crossover_rate = 0.8
mutation_rate = 0.1

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

    last_population = int(max(subdirectories, key=lambda subdir: int(subdir)))
    population_path = os.path.realpath(os.path.join(populations_path, str(last_population)))
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

    server_pid.terminate()
    server_pid.wait()

    fitnesses = list(map(lambda p: ProgramLog(p), programs))

    new_population_path = os.path.realpath(os.path.join(populations_path, str(1 + last_population)))
    os.mkdir(new_population_path)

    new_fitnesses = []
    perform_selection(fitnesses, new_population_path, new_fitnesses)
    perform_crossover(new_fitnesses, new_population_path)
    perform_mutation(new_population_path)

def perform_selection(fitnesses, new_population_path, new_fitnesses):
    weights = list(map(lambda f: f.get_fitness(), fitnesses))

    for i in range(population_size):
        program_index = roulette_wheel(weights)
        new_program_file = os.path.join(new_population_path, "{0:x}.sla".format(i))
        shutil.copy(fitnesses[program_index].program_file, new_program_file)
        new_fitnesses.append((i, weights[program_index]))

def perform_crossover(new_fitnesses, population_path):
    weights = list(map(lambda f: f[1], new_fitnesses))

    for i in range(population_size // 4):
        if not(probability_test(crossover_rate)): continue

        program_index_1 = roulette_wheel(weights)
        program_index_2 = roulette_wheel(weights)

        program_1_filename = os.path.join(population_path, "{0:x}.sla".format(new_fitnesses[program_index_1][0]))
        program_2_filename = os.path.join(population_path, "{0:x}.sla".format(new_fitnesses[program_index_2][0]))

        program_1 = Program(program_1_filename)
        program_1.load()
        program_2 = Program(program_2_filename)
        program_2.load()

        program_1.crossover(program_2)

        program_1.save()
        program_2.save()

def perform_mutation(population_path):
    for i in range(population_size):
        if not(probability_test(mutation_rate)): continue

        program = Program(os.path.join(population_path, "{0:x}.sla".format(i)))
        program.load()
        program.mutate()
        program.save()

def roulette_wheel(weights):
    total = sum(weights)
    transformed_weights = list(map(lambda w: w / total if 0 != total else w, weights))
    transformed_weights_len = len(transformed_weights)

    value = random.random()

    for i in range(transformed_weights_len):
        if value < transformed_weights[i]:
            return i

        value = value - transformed_weights[i]

    return transformed_weights_len - 1

def initialize_population():
    population_path = os.path.realpath(os.path.join(populations_path, "0"))
    print("Creating new population with size {0} at {1}".format(population_size, population_path))
    if not os.path.exists(population_path):
        os.makedirs(population_path)

    for i in range(population_size):
        program_file = os.path.join(population_path, "{0:x}.sla".format(i))
        print("Writing {0}".format(program_file))
        with open(program_file, mode='w') as f:
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

def probability_test(rate):
    return random.random() < rate

if "__main__" == __name__:
    main()
