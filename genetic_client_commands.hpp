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

        while (angle >= +M_PI) angle -= M_PI;
        while (angle <= -M_PI) angle += M_PI;

        turn(&genetic_client_protocol, angle);
    }
};

class LookAt : public SlashA::Instruction
{
    public:
    LookAt() { name = "LookAt"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        if (core.D_size <= core.I + 2)
        {
            n_invops++;
            return;
        }

        Vector l = { .x = core.D[core.I + 0], .y = core.D[core.I + 1], .z =  core.D[core.I + 2] };
        VECTOR_NORMALIZE(&l);
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

class GetFireDelay : public SlashA::Instruction
{
    public:
    GetFireDelay() { name = "GetFireDelay"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        int delay = tank_get_fire_delay(&genetic_client_protocol);
        core.setF(delay);
    }
};

class GetHeading : public SlashA::Instruction
{
    public:
    GetHeading() { name = "GetHeading"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        double heading = client_tank_get_heading(&genetic_client_protocol);
        core.setF(heading);
    }
};

class GetSpeed : public SlashA::Instruction
{
    public:
    GetSpeed() { name = "GetSpeed"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        double speed = tank_get_speed(&genetic_client_protocol);
        core.setF(speed);
    }
};

#endif __GENETIC_CLIENT_COMMANDS_HPP__ /* __GENETIC_CLIENT_COMMANDS_HPP__ */
