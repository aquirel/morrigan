// genetic_client_commands.hpp - additional tank-related commands.

#pragma once
#ifndef __GENETIC_CLIENT_COMMANDS_HPP__
#define __GENETIC_CLIENT_COMMANDS_HPP__

#include <cmath>

#include "SlashA.hpp"

#include "genetic_client_net.hpp"

#define TANK_MIN_ENGINE_POWER -10
#define TANK_MAX_ENGINE_POWER 100

class SetEnginePower : public SlashA::Instruction
{
    public:
    SetEnginePower() { name = "SetEnginePower"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        double power = core.getF();

        if (TANK_MIN_ENGINE_POWER > power)
        {
            power = TANK_MIN_ENGINE_POWER;
        }
        else if (TANK_MAX_ENGINE_POWER < power)
        {
            power = TANK_MAX_ENGINE_POWER;
        }

        set_engine_power(&genetic_client_protocol, (int) power);
    }
};

class Turn : public SlashA::Instruction
{
    public:
    Turn() { name = "Turn"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        double angle = core.getF();

        while (angle >= +2.0 * M_PI) angle -= 2.0 * M_PI;
        while (angle <= -2.0 * M_PI) angle += 2.0 * M_PI;

        turn(&genetic_client_protocol, angle);
    }
};

class LookAt : public SlashA::Instruction
{
    public:
    LookAt() { name = "LookAt"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        Vector l = { .x = core.D[core.I + 0], .y = core.D[core.I + 1], .z = core.D[core.I + 2] };
        look_at(&genetic_client_protocol, &l);
    }
};

class Shoot : public SlashA::Instruction
{
    public:
    Shoot() { name = "Shoot"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        shoot(&genetic_client_protocol);
    }
};

#endif __GENETIC_CLIENT_COMMANDS_HPP__ /* __GENETIC_CLIENT_COMMANDS_HPP__ */
