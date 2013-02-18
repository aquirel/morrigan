// game.c - game main loop.

#include <assert.h>
#include <threads.h>
#include <stdatomic.h>
#include <sys/time.h>
#include <time.h>

#include "debug.h"
#include "game.h"
#include "landscape.h"
#include "protocol.h"
#include "tank.h"

static thrd_t worker_tid;
static volatile atomic_bool working = false;

const Landscape *landscape = NULL;
static DynamicArray *clients = NULL;

static int game_worker(void *unused);
static unsigned long long __timeval_sub(struct _timeval *t1, struct _timeval *t2);

bool game_start(const Landscape *l, DynamicArray *c)
{
    fprintf(stderr, "game_start start.\n");
    assert(l && "Bad landscape pointer.");
    assert(c && "Bad clients array pointer.");

    landscape = l;
    clients = c;

    working = true;
    check(thrd_success == thrd_create(&worker_tid, game_worker, NULL), "Failed to start game worker thread.", "");

    fprintf(stderr, "game_start end.\n");
    return true;
    error:
    fprintf(stderr, "game_start error.\n");
    thrd_detach(worker_tid);
    return false;
}

void game_stop(void)
{
    fprintf(stderr, "game_stop start.\n");
    working = false;
    thrd_join(worker_tid, NULL);
    thrd_detach(worker_tid);
    fprintf(stderr, "game_stop end.\n");
}

static int game_worker(void *unused)
{
    struct _timeval tick_start_time, tick_end_time;

    srand(time(NULL));

    while (working)
    {
        _gettimeofday(&tick_start_time, NULL);

        dynamic_array_lock(clients);
        size_t clients_count = dynamic_array_count(clients);
        for (size_t i = 0; i < clients_count; i++)
        {
            Client *c = *DYNAMIC_ARRAY_GET(Client **, clients, i);

            if (cs_connected == c->state)
            {
                continue;
            }

            if (cs_acknowledged == c->state)
            {
                Vector position, top;

                size_t j;
                do
                {
                    position.x = rand() % (landscape->landscape_size * landscape->tile_size - 1);
                    position.y = rand() % (landscape->landscape_size * landscape->tile_size - 1);
                    position.z = landscape_get_height_at(landscape, position.x, position.y);

                    for (j = 0; j < clients_count; j++)
                    {
                        if (i == j)
                        {
                            continue;
                        }

                        Client *previous_c = *DYNAMIC_ARRAY_GET(Client **, clients, j);
                        if (cs_in_game == previous_c->state &&
                                          intersection_test(&c->tank.bounding, &previous_c->tank.bounding))
                        {
                            break;
                        }
                    }
                } while (j < clients_count);

                landscape_get_normal_at(landscape, position.x, position.y, &top);
                tank_initialize(&c->tank, &position, &top, clients_count);
                c->state = cs_in_game;
            }

            if (!tank_tick(&c->tank, landscape))
            {
                uint8_t data = not_tank_hit_bound;
                respond(&data, 1, &c->address);
            }

            for (size_t j = 0; j < i; j++)
            {
                if (i == j)
                {
                    continue;
                }

                Client *previous_c = *DYNAMIC_ARRAY_GET(Client **, clients, j);
                if (cs_in_game == previous_c->state &&
                    intersection_test(&c->tank.bounding, &previous_c->tank.bounding))
                {
                    intersection_resolve(&c->tank.bounding, &previous_c->tank.bounding);

                    uint8_t data = not_tank_collision;
                    respond(&data, 1, &c->address);
                    respond(&data, 1, &previous_c->address);
                }
            }
        }
        dynamic_array_unlock(clients);

        _gettimeofday(&tick_end_time, NULL);

        unsigned long long tick_length = __timeval_sub(&tick_end_time, &tick_start_time); // Microseconds.
        if (GAME_TICK_DURATION >= tick_length)
        {
            unsigned long long time_to_sleep = GAME_TICK_DURATION - tick_length;
            struct timespec sleep_duration = { .tv_sec = time_to_sleep / 1000000, .tv_nsec = (time_to_sleep % 1000000) * 1000 };
            check(0 == thrd_sleep(&sleep_duration, NULL), "Failed to sleep.", "");
        }
    }

    return 0;
    error:
    return -1;
}

static unsigned long long __timeval_sub(struct _timeval *t1, struct _timeval *t2)
{
    assert(t1 && t2 && "Bad time pointers.");
    unsigned long long _t1 = t1->tv_sec * 1000000 + t1->tv_usec,
                       _t2 = t2->tv_sec * 1000000 + t2->tv_usec;
    return _t1 - _t2;
}
