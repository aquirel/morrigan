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
#include "shell.h"

static thrd_t worker_tid;
static volatile atomic_bool working = false;

const Landscape *landscape = NULL;
static DynamicArray *clients = NULL;
static DynamicArray *shells = NULL;

static int game_worker(void *unused);

static bool __game_tank_initialize(size_t i, Client *c, const Landscape *landscape, size_t clients_count);
static void __tank_collision_detection(size_t i, Client *c);
static void __perform_shooting(Client *client);
static void __notify_in_radius(const Vector *origin, double radius, uint8_t message, Client *exclude);
static Client *__shell_collision_detection(const Shell *shell);
static void __tank_hit(Client *c, int amount);
static void __shell_explode(Shell *shell, Client *exclude);

static unsigned long long __timeval_sub(struct _timeval *t1, struct _timeval *t2);

bool game_start(const Landscape *l, DynamicArray *c)
{
    log_info("start.", "");
    assert(l && "Bad landscape pointer.");
    assert(c && "Bad clients array pointer.");

    landscape = l;
    clients = c;

    check_mem(shells = DYNAMIC_ARRAY_CREATE(Shell *, 16));

    working = true;
    check(thrd_success == thrd_create(&worker_tid, game_worker, NULL), "Failed to start game worker thread.", "");

    log_info("end.", "");
    return true;
    error:
    thrd_detach(worker_tid);
    if (shells)
    {
        dynamic_array_destroy(shells);
        shells = NULL;
    }
    log_info("error.", "");
    return false;
}

void game_stop(void)
{
    log_info("start.", "");
    working = false;
    log_info("wait for worker to stop.", "");
    thrd_join(worker_tid, NULL);
    thrd_detach(worker_tid);
    dynamic_array_destroy(shells);
    shells = NULL;
    log_info("end.", "");
}

static int game_worker(void *unused)
{
    struct _timeval tick_start_time, tick_end_time;
    log_info("start.", "");

    srand(time(NULL));

    while (working)
    {
        log_info("tick start.", "");
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
                __game_tank_initialize(i, c, landscape, clients_count);
            }

            if (!tank_tick(&c->tank, landscape))
            {
                uint8_t data = not_tank_hit_bound;
                respond(&data, 1, &c->address);
            }

            __tank_collision_detection(i, c);

            if (-1 == c->tank.fire_delay)
            {
                __perform_shooting(c);
            }
        }

        for (size_t i = 0; i < dynamic_array_count(shells);)
        {
            Shell *shell = *DYNAMIC_ARRAY_GET(Shell **, shells, i);

            bool result = shell_tick(shell, landscape);
            Client *hit_tank = __shell_collision_detection(shell);

            if (hit_tank)
            {
                __tank_hit(hit_tank, 0);
            }

            if (hit_tank || !result)
            {
                __shell_explode(shell, hit_tank);
                dynamic_array_delete_at(shells, i);
                free(shell);
            }
            else
            {
                i++;
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

        log_info("tick end.", "");
    }

    log_info("end.", "");
    return 0;
    error:
    log_info("error.", "");
    return -1;
}

static bool __game_tank_initialize(size_t i, Client *c, const Landscape *landscape, size_t clients_count)
{
    assert(c && "Bad client pointer.");

    log_info("initializing new tank.", "");

    size_t j;

    do
    {
        Vector position, top;

        position.x = rand() % (landscape->landscape_size * landscape->tile_size - 1);
        position.y = rand() % (landscape->landscape_size * landscape->tile_size - 1);
        position.z = landscape_get_height_at(landscape, position.x, position.y);

        landscape_get_normal_at(landscape, position.x, position.y, &top);
        tank_initialize(&c->tank, &position, &top, clients_count);

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

    check(thrd_success == mtx_init(&c->tank.mtx, mtx_plain | mtx_recursive), "Failed to initialize tank mutex.", "");
    c->state = cs_in_game;

    return true;
    error:
    return false;
}

static void __tank_collision_detection(size_t i, Client *c)
{
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

static void __perform_shooting(Client *client)
{
    assert(client && "Bad client pointer.");

    Vector e = TANK_BOUNDING_BOX_EXTENT,
           p = client->tank.position,
           o = client->tank.orientation;

    VECTOR_SCALE(&o, 2.0 * e.z + TANK_BOUNDING_SPHERE_RADIUS / 2.0);
    VECTOR_ADD(&p, &o);

    Vector default_turret_direction = { .x = 1, .y = 0, .z = 0 },
           turret_direction = client->tank.direction,
           rotation_axis;
    double angle = vector_angle(&default_turret_direction, &client->tank.turret_direction);
    if (0.0 != angle)
    {
        vector_vector_mul(&default_turret_direction, &turret_direction, &rotation_axis);
        VECTOR_NORMALIZE(&rotation_axis);
        VECTOR_ROTATE(&turret_direction, &rotation_axis, angle);
    }

    VECTOR_SCALE(&turret_direction, TANK_GUN_LENGTH);
    VECTOR_ADD(&p, &turret_direction);

    Shell *new_shell = NULL;
    new_shell = shell_create(&p, &turret_direction);
    check_mem(new_shell);

    check(dynamic_array_push(shells, &new_shell), "Failed to add new shell.", "");

    __notify_in_radius(&client->tank.position, NEAR_SHOOT_NOTIFICATION_RARIUS, not_near_shoot, client);

    client->tank.fire_delay = TANK_FIRE_DELAY;

    NotViewerShellEvent shoot_notification = {
        .type = not_viewer_shoot,
        .x = new_shell->position.x,
        .y = new_shell->position.y,
        .z = new_shell->position.z
    };
    notify_viewers(&shoot_notification);

    error:
    return;
}

static void __notify_in_radius(const Vector *origin, double radius, uint8_t message, Client *exclude)
{
    assert(origin && "Bad origin pointer.");

    size_t clients_count = dynamic_array_count(clients);
    for (size_t i = 0; i < clients_count; i++)
    {
        Client *c = *DYNAMIC_ARRAY_GET(Client **, clients, i);

        if (cs_in_game != c->state || exclude == c)
        {
            continue;
        }

        Vector t;
        vector_sub(origin, &c->tank.position, &t);

        if (vector_length(&t) > radius)
        {
            continue;
        }

        respond(&message, sizeof(message), &c->address);
    }
}

static Client *__shell_collision_detection(const Shell *shell)
{
    assert(shell && "Bad shell pointer.");

    size_t clients_count = dynamic_array_count(clients);
    for (size_t i = 0; i < clients_count; i++)
    {
        Client *c = *DYNAMIC_ARRAY_GET(Client **, clients, i);

        if (cs_in_game != c->state)
        {
            continue;
        }

        if (intersection_test(&shell->bounding, &c->tank.bounding))
        {
            return c;
        }
    }

    return NULL;
}

static void __tank_hit(Client *c, int amount)
{
    assert(c && "Bad client pointer.");

    uint8_t response;
    if (!amount)
    {
        amount = SHELL_HIT_AMOUNT;
    }

    c->tank.hp -= amount;
    if (0 >= c->tank.hp)
    {
        c->tank.hp = 0;
        response = not_death;
    }
    else
    {
        response = not_hit;
    }

    respond(&response, sizeof(response), &c->address);
}

static void __shell_explode(Shell *shell, Client *exclude)
{
    assert(shell && "Bad shell pointer.");

    uint8_t response;
    Vector t;

    size_t clients_count = dynamic_array_count(clients);
    for (size_t i = 0; i < clients_count; i++)
    {
        Client *c = *DYNAMIC_ARRAY_GET(Client **, clients, i);

        if (cs_in_game != c->state || c == exclude)
        {
            continue;
        }

        vector_sub(&shell->position, &c->tank.position, &t);
        double r = vector_length(&t);

        if (r <= SHELL_EXPLOSION_RADIUS)
        {
            int damage_amount = SHELL_EXPLOSION_DAMAGE / (r * r);

            if (!damage_amount)
            {
                response = not_near_explosion;
            }
            else
            {
                c->tank.hp -= damage_amount;
                if (0 >= c->tank.hp)
                {
                    c->tank.hp = 0;
                    response = not_death;
                }
                else
                {
                    response = not_explosion_damage;
                }
            }

            respond(&response, sizeof(response), &c->address);
        }
        else if (r <= NEAR_EXPLOSION_NOTIFICATION_RARIUS)
        {
            response = not_near_explosion;
            respond(&response, sizeof(response), &c->address);
        }
    }

    NotViewerShellEvent explosion_notification = {
        .type = not_viewer_explosion,
        .x = shell->position.x,
        .y = shell->position.y,
        .z = shell->position.z
    };
    notify_viewers(&explosion_notification);
}

static unsigned long long __timeval_sub(struct _timeval *t1, struct _timeval *t2)
{
    assert(t1 && t2 && "Bad time pointers.");
    unsigned long long _t1 = t1->tv_sec * 1000000 + t1->tv_usec,
                       _t2 = t2->tv_sec * 1000000 + t2->tv_usec;
    return _t1 - _t2;
}
