// genetic_client_commands.hpp - additional tank-related commands.

#pragma once
#ifndef __GENETIC_CLIENT_COMMANDS_HPP__
#define __GENETIC_CLIENT_COMMANDS_HPP__

#include <iostream>
#include <cmath>

#include "SlashA.hpp"

extern "C"
{
    #include "landscape.h"
}

#include "genetic_client.hpp"
#include "genetic_client_net.hpp"

class ExtensionInstruction : public SlashA::Instruction
{
    public:
    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        std::cout << "Running: " << name << std::endl;
    }
};

class SetEnginePower : public ExtensionInstruction
{
    public:
    SetEnginePower() { name = "SetEnginePower"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);

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

class Turn : public ExtensionInstruction
{
    public:
    Turn() { name = "Turn"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);

        double angle = core.getF();

        while (angle >= +M_PI) angle -= M_PI;
        while (angle <= -M_PI) angle += M_PI;

        turn(&genetic_client_protocol, angle);
    }
};

class LookAt : public ExtensionInstruction
{
    public:
    LookAt() { name = "LookAt"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);

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

class Shoot : public ExtensionInstruction
{
    public:
    Shoot() { name = "Shoot"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);
        shoot(&genetic_client_protocol);
    }
};

class GetFireDelay : public ExtensionInstruction
{
    public:
    GetFireDelay() { name = "GetFireDelay"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);
        int delay = tank_get_fire_delay(&genetic_client_protocol);
        core.setF(delay);
    }
};

class GetHeading : public ExtensionInstruction
{
    public:
    GetHeading() { name = "GetHeading"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);
        double heading = client_tank_get_heading(&genetic_client_protocol);
        core.setF(heading);
    }
};

class GetSpeed : public ExtensionInstruction
{
    public:
    GetSpeed() { name = "GetSpeed"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);
        double speed = tank_get_speed(&genetic_client_protocol);
        core.setF(speed);
    }
};

class GetHP : public ExtensionInstruction
{
    public:
    GetHP() { name = "GetHP"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);
        int hp = tank_get_hp(&genetic_client_protocol);
        core.setF(hp);
    }
};

class GetHeight : public ExtensionInstruction
{
    public:
    GetHeight() { name = "GetHeight"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);

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

class GetNormal : public ExtensionInstruction
{
    public:
    GetNormal() { name = "GetNormal"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);

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

class Tanks : public ExtensionInstruction
{
    public:
    Tanks() { name = "Tanks"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);
        core.setF(tanks_count);
    }
};

class Tank : public ExtensionInstruction
{
    public:
    Tank() { name = "Tank"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);

        if (0 == tanks_count)
        {
            return;
        }

        unsigned destination = core.getF();
        if (destination >= core.D_size - (3 + 3 + 3 + 3 + 3))
        {
            return;
        }

        unsigned tank_index = core.I % tanks_count;
        core.D[destination +  0] = tanks[tank_index].x;
        core.D[destination +  1] = tanks[tank_index].y;
        core.D[destination +  2] = tanks[tank_index].z;

        core.D[destination +  3] = tanks[tank_index].direction_x;
        core.D[destination +  4] = tanks[tank_index].direction_y;
        core.D[destination +  5] = tanks[tank_index].direction_z;

        core.D[destination +  6] = tanks[tank_index].orientation_x;
        core.D[destination +  7] = tanks[tank_index].orientation_y;
        core.D[destination +  8] = tanks[tank_index].orientation_z;

        core.D[destination +  9] = tanks[tank_index].turret_x;
        core.D[destination + 10] = tanks[tank_index].turret_y;
        core.D[destination + 11] = tanks[tank_index].turret_z;

        core.D[destination + 12] = tanks[tank_index].speed;
        core.D[destination + 13] = tanks[tank_index].team;
        core.D[destination + 14] = tanks[tank_index].hp;
    }
};

class HitBound : public ExtensionInstruction
{
    public:
    HitBound() { name = "HitBound"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);
        core.setF(not_hit_bound_flag ? 1.0 : 0.0);
    }
};

class TankCollision : public ExtensionInstruction
{
    public:
    TankCollision() { name = "TankCollision"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);
        core.setF(not_tank_collision_flag ? 1.0 : 0.0);
    }
};

class Hit : public ExtensionInstruction
{
    public:
    Hit() { name = "Hit"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);
        core.setF(not_hit_flag ? 1.0 : 0.0);
    }
};

class NearShoot : public ExtensionInstruction
{
    public:
    NearShoot() { name = "NearShoot"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);

        if (not_near_shoot_flag)
        {
            core.setF(1.0);
            if (core.I < core.D_size - 2)
            {
                core.D[core.I + 0] = not_near_shoot_position.x;
                core.D[core.I + 1] = not_near_shoot_position.y;
                core.D[core.I + 2] = not_near_shoot_position.z;
            }
        }
        else
        {
            core.setF(0.0);
        }
    }
};

class NearExplosion : public ExtensionInstruction
{
    public:
    NearExplosion() { name = "NearExplosion"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);

        if (not_near_explosion_flag)
        {
            core.setF(1.0);
            if (core.I < core.D_size - 2)
            {
                core.D[core.I + 0] = not_near_explosion_position.x;
                core.D[core.I + 1] = not_near_explosion_position.y;
                core.D[core.I + 2] = not_near_explosion_position.z;
            }
        }
        else
        {
            core.setF(0.0);
        }
    }
};

class NearExplosionDamage : public ExtensionInstruction
{
    public:
    NearExplosionDamage() { name = "NearExplosionDamage"; }

    virtual inline void code(SlashA::MemCore& core, SlashA::InstructionSet& iset)
    {
        ExtensionInstruction::code(core, iset);

        if (not_explosion_damage_flag)
        {
            core.setF(1.0);
            if (core.I < core.D_size - 2)
            {
                core.D[core.I + 0] = not_explosion_damage_position.x;
                core.D[core.I + 1] = not_explosion_damage_position.y;
                core.D[core.I + 2] = not_explosion_damage_position.z;
            }
        }
        else
        {
            core.setF(0.0);
        }
    }
};

#endif __GENETIC_CLIENT_COMMANDS_HPP__ /* __GENETIC_CLIENT_COMMANDS_HPP__ */
