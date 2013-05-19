// genetic_client_main.cpp - main() for genetic client.

#include <cassert>
#include <csignal>

#include <vector>
#include <fstream>

extern "C"
{
    #include "process.h"
    #include "time.h"
    #include "sys/time.h"
    #include "unistd.h"
}

#include "SlashA.hpp"

extern "C"
{
    #include "debug.h"
    #include "client_protocol.h"
}

#include "genetic_client_net.hpp"
#include "genetic_client_commands.hpp"

// 1 sec.
#define TICK_DURATION 1000000

volatile bool working = true;

static void __stop(int unused);
static void __insert_additional_instructions(SlashA::InstructionSet &instruction_set);
static void __cleanup(void);
static unsigned long __timeval_sub(struct timeval *t1, struct timeval *t2);

int main(int argc, char *argv[])
{
    // Parse input.
    if (3 > argc)
    {
        fprintf(stderr, "Usage: %s <program.sla> <server-address> [<port>]\n", argv[0]);
        return -1;
    }

    unsigned short port = 0;
    if (3 < argc)
    {
        port = (unsigned short) atoi(argv[3]);
        if (!port)
        {
            port = PORT;
        }
    }

    if (SIG_ERR == signal(SIGINT, __stop) ||
        SIG_ERR == signal(SIGTERM, __stop))
    {
        fprintf(stderr, "Failed to set signal handler.");
        __cleanup();
        return -1;
    }

    bool clean_exit = true;
    try
    {
        // Prepare Slash/A.
        puts(SlashA::getHeader().c_str());

        puts("Initializing Slash/A.");
        std::vector<double> input, output;
        SlashA::MemCore mem_core(0, 0, input, output);

        SlashA::InstructionSet instruction_set(0xffff);
        instruction_set.insert_DIS_full();
        __insert_additional_instructions(instruction_set);

        // Load program.
        puts("Loading program.");
        std::string source;
        std::ifstream f(argv[1]);

        if (!f)
        {
            fprintf(stderr, "Failed to open file: %s.\n", argv[1]);
            __cleanup();
            return -1;
        }

        std::getline(f, source, std::char_traits<char>::to_char_type(std::char_traits<char>::eof()));
        f.close();

        // Compile program.
        puts("Compilling program.");
        SlashA::ByteCode bytecode;
        source2ByteCode(source, bytecode, instruction_set);

        // Prepare networking.
        puts("Connecting to server.");
        if (!client_net_start())
        {
            fprintf(stderr, "Failed to initialize net.");
            __cleanup();
            return -1;
        }

        if (!client_connect(&genetic_client_protocol, argv[2], port, true))
        {
            fprintf(stderr, "Failed to connect..");
            __cleanup();
            return -1;
        }
        puts("Connected to server.");

        // Main loop.
        struct timeval tick_start_time, tick_end_time;
        do
        {
            puts("Tick start.");
            gettimeofday(&tick_start_time, NULL);

            while (client_protocol_process_event(&genetic_client_protocol));
            // TODO: get landscape.
            // TODO: get tanks.

            // Run genetic program.
            bool failed = SlashA::runByteCode(instruction_set,
                                              mem_core,
                                              bytecode,
                                              time(NULL) ^ _getpid(),
                                              1,
                                              256);

            if (failed)
            {
                log_warning("Program failed.", "");
            }

            gettimeofday(&tick_end_time, NULL);

            unsigned long tick_length = __timeval_sub(&tick_end_time, &tick_start_time); // Microseconds.
            if (TICK_DURATION >= tick_length)
            {
                unsigned long long time_to_sleep = TICK_DURATION - tick_length;
                usleep(time_to_sleep);
            }
        } while (working);
    }
    catch (std::string &e)
    {
        fprintf(stderr, "Exception: %s\n", e.c_str());
        clean_exit = false;
    }

    std::string statistics_filename(argv[1]);
    statistics_filename += ".log";

    if (clean_exit)
    {
        ResGetStatistics statistics;
        check(tank_get_statistics(&genetic_client_protocol, &statistics), "Failed to get tank statistics.", "");

        FILE *statistics_file = fopen(statistics_filename.c_str(), "w");
        check(statistics_file, "Failed to open statistics file.", "");
        fprintf(statistics_file, "%lu\n", (unsigned long) statistics.ticks);
        fprintf(statistics_file, "%u\n", statistics.hp);
        fprintf(statistics_file, "%u\n", statistics.direct_hits);
        fprintf(statistics_file, "%u\n", statistics.hits);
        fprintf(statistics_file, "%u\n", statistics.got_direct_hits);
        fprintf(statistics_file, "%u\n", statistics.got_hits);
        fclose(statistics_file);
    }
    else
    {
        remove(statistics_filename.c_str());
    }

    __cleanup();
    return 0;

    error:
    __cleanup();
    return -1;
}

static void __stop(int unused)
{
    working = false;
}

static void __insert_additional_instructions(SlashA::InstructionSet &instruction_set)
{
    SetEnginePower *set_engine_power_instruction = new SetEnginePower();
    Turn *turn_instruction = new Turn();
    LookAt *look_at_instruction = new LookAt();
    Shoot *shoot_instruction = new Shoot();

    check(set_engine_power_instruction &&
          turn_instruction &&
          look_at_instruction &&
          shoot_instruction,
          "Failed to create instructions.",
          "");

    instruction_set.insert(set_engine_power_instruction);
    instruction_set.insert(turn_instruction);
    instruction_set.insert(look_at_instruction);
    instruction_set.insert(shoot_instruction);

    error:
    return;
}

static void __cleanup(void)
{
    if (genetic_client_protocol.connected)
    {
        check(client_disconnect(&genetic_client_protocol, true), "Failed to disconnect.", "");
    }

    static bool net_stopped = false;
    error:
    if (!net_stopped)
    {
        client_net_stop();
        net_stopped = true;
    }
}

static unsigned long __timeval_sub(struct timeval *t1, struct timeval *t2)
{
    assert(t1 && t2 && "Bad time pointers.");
    unsigned long _t1 = t1->tv_sec * 1000000 + t1->tv_usec,
                  _t2 = t2->tv_sec * 1000000 + t2->tv_usec;
    return _t1 - _t2;
}
