// genetic_client.hpp - genetic client commons.

#pragma once
#ifndef __GENETIC_CLIENT_HPP__
#define __GENETIC_CLIENT_HPP__

extern "C"
{
    #include "vector.h"
    #include "tank_defines.h"
    #include "client_protocol.h"
}

extern volatile bool working;
extern double landscape[TANK_OBSERVING_RANGE][TANK_OBSERVING_RANGE];
extern ResGetTanksTankRecord tanks[MAX_CLIENTS];
extern size_t tanks_count;

extern bool not_hit_bound_flag;
extern bool not_tank_collision_flag;
extern bool not_near_shoot_flag;
extern Vector not_near_shoot_position;
extern bool not_hit_flag;
extern bool not_near_explosion_flag;
extern Vector not_near_explosion_position;
extern bool not_explosion_damage_flag;
extern Vector not_explosion_damage_position;

#endif /* __GENETIC_CLIENT_HPP__ */
