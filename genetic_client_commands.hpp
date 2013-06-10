// genetic_client_commands.hpp - additional tank-related commands.

#pragma once
#ifndef __GENETIC_CLIENT_COMMANDS_HPP__
#define __GENETIC_CLIENT_COMMANDS_HPP__

#include <cmath>

#include "SlashA.hpp"

extern "C"
{
    #include "landscape.h"
    #include "tank_defines.h"
    #include "client_protocol.h"
}

#include "genetic_client_net.hpp"

#define TANK_MIN_ENGINE_POWER -10
#define TANK_MAX_ENGINE_POWER 100

extern double landscape[TANK_OBSERVING_RANGE][TANK_OBSERVING_RANGE];
extern ResGetTanksTankRecord tanks[MAX_CLIENTS];
extern size_t tanks_count;

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

class GetHP : public SlashA::Instruction
{
    public:
    GetHP() { name = "GetHP"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        int hp = tank_get_hp(&genetic_client_protocol);
        core.setF(hp);
    }
};

class GetHeight : public SlashA::Instruction
{
    public:
    GetHeight() { name = "GetHeight"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        double height = 0.0;

        if (core.I < core.D_size - 1)
        {
            double x = core.D[core.I + 0] / TILE_SIZE - TANK_OBSERVING_RANGE / 2.0,
                   y = core.D[core.I + 1] / TILE_SIZE - TANK_OBSERVING_RANGE / 2.0;

            if (-TANK_OBSERVING_RANGE / 2.0 <= x && x <= TANK_OBSERVING_RANGE / 2.0 &&
                -TANK_OBSERVING_RANGE / 2.0 <= y && y <= TANK_OBSERVING_RANGE / 2.0)
            {
                height = landscape[((int) x) + TANK_OBSERVING_RANGE / 2][((int) y) + TANK_OBSERVING_RANGE / 2];
            }
        }

        core.setF(height);
    }
};

class GetNormal : public SlashA::Instruction
{
    public:
    GetNormal() { name = "GetNormal"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        Vector normal;
        if (!tank_get_normal(&genetic_client_protocol, &normal))
        {
            return;
        }

        if (core.I >= core.D_size - 2)
        {
            return;
        }

        core.D[core.I + 0] = normal.x;
        core.D[core.I + 1] = normal.y;
        core.D[core.I + 2] = normal.z;
    }
};

class Tanks : public SlashA::Instruction
{
    public:
    Tanks() { name = "Tanks"; }

    inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        core.setF(tanks_count);
    }
};

#endif __GENETIC_CLIENT_COMMANDS_HPP__ /* __GENETIC_CLIENT_COMMANDS_HPP__ */
