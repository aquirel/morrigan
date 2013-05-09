// viewer_main.c - main() for viewer.

#include "debug.h"
#include "viewer.h"
#include "tank.h"

static Landscape *l = NULL;
static ResGetTanksTankRecord tanks[MAX_CLIENTS];
static size_t tanks_count = 0;

#define SHOOT_LIFETIME 100
#define EXPLOSION_LIFETIME 250
static DynamicArray *shoots = NULL, *explosions = NULL;

#define TANKS_POLL_INTERVAL 100
static SDL_TimerID timer_id = NULL, tanks_timer_id = NULL;

static Camera camera;

static GLuint display_lists = 0;

static bool __init_video(Camera *camera);
static void __cleanup(void);

static Uint32 __timer_handler(Uint32 interval, void *param);

int main(int argc, char *argv[])
{
    if (2 > argc)
    {
        fprintf(stderr, "Usage: %s <server-address> [<port>]\n", argv[0]);
        return -1;
    }

    unsigned short port = 0;
    if (2 < argc)
    {
        port = (unsigned short) atoi(argv[2]);
        if (!port)
        {
            port = PORT;
        }
    }

    check_mem(shoots = DYNAMIC_ARRAY_CREATE(NotViewerShellEvent *, 16));
    check_mem(explosions = DYNAMIC_ARRAY_CREATE(NotViewerShellEvent *, 16));

    check(client_net_start(), "Failed to initialize net.", "");
    check(client_connect(&viewer_protocol, argv[1], port, false), "Failed to connect.", "");

    puts("Connected to server.");

    l = client_get_landscape(&viewer_protocol);
    check(l, "Failed to get landscape.", "");
    puts("Loaded landscape.");

    camera.x = camera.y = l->landscape_size / 2.0 * l->tile_size;

    check(__init_video(&camera), "Failed to init video.", "");

    check(0 != (display_lists = glGenLists(DISPLAY_LISTS_COUNT)), "Failed to create display list.", "");

    glNewList(display_lists + LANDSCAPE_DISPLAY_LIST, GL_COMPILE);
    check(GL_NO_ERROR == glGetError(), "Failed to create display list.", "");
    draw_landscape(l);
    glEndList();

    glNewList(display_lists + TANK_BODY_DISPLAY_LIST, GL_COMPILE);
    check(GL_NO_ERROR == glGetError(), "Failed to create display list.", "");
    draw_tank_body();
    glEndList();

    glNewList(display_lists + TANK_TURRET_DISPLAY_LIST, GL_COMPILE);
    check(GL_NO_ERROR == glGetError(), "Failed to create display list.", "");
    draw_tank_turret();
    glEndList();

    glNewList(display_lists + TANK_GUN_DISPLAY_LIST, GL_COMPILE);
    check(GL_NO_ERROR == glGetError(), "Failed to create display list.", "");
    draw_tank_gun();
    glEndList();

    while (true)
    {
        bool need_redraw = false;
        if (!process_events(&need_redraw, &camera, tanks, &tanks_count, &viewer_protocol))
        {
            break;
        }

        if (need_redraw)
        {
            draw(l, tanks, tanks_count, &camera, &display_lists, shoots, explosions);
        }
    }

    __cleanup();
    return 0;

    error:
    __cleanup();
    return -1;
}

static bool __init_video(Camera *camera)
{
    assert(camera && "Bad camera pointer.");

    camera->w = 800;
    camera->h = 600;
    camera->bpp = 32;
    camera->move_speed = 4.0;
    camera->mouse_sensitivity = 0.1;
    camera->z = 512;
    camera->vertical_angle = 90.0;
    camera->horizontal_angle = 0.0;
    camera->fov_y = 60.0;

    check(0 == SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER), "Failed to init SDL.", "");
    SDL_WM_SetCaption("morrigan viewer", NULL);
    check(0 == SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16), "Failed to set SDL_GL_DEPTH_SIZE.", "");
    check(0 == SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "Failed to set SDL_GL_DOUBLEBUFFER.", "");
    /*
    const SDL_VideoInfo *vi = SDL_GetVideoInfo();
    check(NULL != vi, "SDL_GetVideoInfo() failed.", "");
    w = vi->current_w;
    h = vi->current_h;
    */
    check(NULL != SDL_SetVideoMode(camera->w, camera->h, camera->bpp, SDL_HWSURFACE | SDL_SWSURFACE | SDL_OPENGL), "Failed to set video mode.", "");
    SDL_ShowCursor(SDL_DISABLE);
    check(NULL != (timer_id = SDL_AddTimer(SDL_DEFAULT_REPEAT_INTERVAL, __timer_handler, (void *) TIMER_EVENT_ID)), "Failed to setup timer.", "");
    check(NULL != (tanks_timer_id = SDL_AddTimer(TANKS_POLL_INTERVAL, __timer_handler, (void *) TANKS_TIMER_EVENT_ID)), "Failed to setup tanks timer.", "");

    glLineWidth(1.0);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_FRONT);
    glEnable(GL_DITHER);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    glViewport(0, 0, camera->w, camera->h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float ratio = (float) camera->w / (float) camera->h;
    gluPerspective(camera->fov_y, ratio, 1, 16384.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScaled(1.0, 1.0, -1.0);

    SDL_WarpMouse(camera->mouse_prev_x = camera->w / 2,
                  camera->mouse_prev_y = camera->h / 2);
    return true;
    error:
    return false;
}

static void __cleanup(void)
{
    if (SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_TIMER))
    {
        SDL_Quit();
    }

    if (NULL != timer_id)
    {
        SDL_RemoveTimer(timer_id);
        timer_id = NULL;
    }

    if (NULL != tanks_timer_id)
    {
        SDL_RemoveTimer(tanks_timer_id);
        tanks_timer_id = NULL;
    }

    if (0 != display_lists)
    {
        glDeleteLists(display_lists, DISPLAY_LISTS_COUNT);
        display_lists = 0;
    }

    if (l)
    {
        landscape_destroy(l);
        l = NULL;
    }

    if (viewer_protocol.connected)
    {
        check(client_disconnect(&viewer_protocol, false), "Failed to disconnect.", "");
    }

    if (shoots)
    {
        dynamic_array_destroy(shoots);
        shoots = NULL;
    }

    if (explosions)
    {
        dynamic_array_destroy(explosions);
        explosions = NULL;
    }

    static bool net_stopped = false;
    error:
    if (!net_stopped)
    {
        client_net_stop();
        net_stopped = true;
    }
}

static Uint32 __timer_handler(Uint32 interval, void *param)
{
    SDL_Event event =
    {
        .user =
        {
            .type = SDL_USEREVENT,
            .code = (int) param,
            .data1 = NULL,
            .data2 = NULL
        }
    };
    check(0 == SDL_PushEvent(&event), "SDL_PushEvent() failed.", "");
    error:
    return interval;
}

/*void move_tanks(const Landscape *l, ResGetTanksTankRecord *tanks, size_t tanks_count)
{
    assert(l && "Bad landscape pointer.");
    assert(tanks && "Bad tanks pointer.");

    for (size_t i = 0; i < tanks_count; i++)
    {
        move_tank(l, &tanks[i]);
    }
}

void move_tank(const Landscape *l, ResGetTanksTankRecord *tank)
{
    assert(l && "Bad landscape pointer.");
    assert(tank && "Bad tank pointer.");

    Vector position    = { .x = tank->x, .y = tank->y, .z = tank->z },
           direction   = { .x = tank->direction_x, .y = tank->direction_y, .z = tank->direction_z },
           orientation = { .x = tank->orientation_x, .y = tank->orientation_y, .z = tank->orientation_z },
           t;

    vector_scale(&direction, tank->speed / SDL_DEFAULT_REPEAT_INTERVAL, &t);
    VECTOR_ADD(&position, &t);

    if (!(0.0 <= position.x &&
          0.0 <= position.y &&
          l->tile_size * l->landscape_size >= position.x &&
          l->tile_size * l->landscape_size >= position.y))
    {
        return;
    }

    position.z = landscape_get_height_at(l, position.x, position.y);

    Vector old_orientation = orientation;
    landscape_get_normal_at(l, position.x, position.y, &orientation);
    tank_rotate_direction(&direction, &old_orientation, &orientation);

    tank_change_turn_worker(&tank->target_turn,
                            TANK_MAX_TURN_SPEED / SDL_DEFAULT_REPEAT_INTERVAL,
                            &direction,
                            &orientation);

    tank->x = position.x;
    tank->y = position.y;
    tank->z = position.z;
    tank->direction_x = direction.x;
    tank->direction_y = direction.y;
    tank->direction_z = direction.z;
    tank->orientation_x = orientation.x;
    tank->orientation_y = orientation.y;
    tank->orientation_z = orientation.z;

    Vector turret_direction        = { .x = tank->turret_x, .y = tank->turret_y, .z = tank->turret_z },
           turret_direction_target = { .x = tank->target_turret_x, .y = tank->target_turret_y, .z = tank->target_turret_z };

    tank_rotate_turret_worker(&turret_direction_target, &turret_direction, TANK_MAX_TURRET_TURN_SPEED / SDL_DEFAULT_REPEAT_INTERVAL);

    tank->turret_x = turret_direction.x;
    tank->turret_y = turret_direction.y;
    tank->turret_z = turret_direction.z;
}*/

void process_shell_collection(DynamicArray *c, const size_t lifetime)
{
    assert(c && "Bad shell event collection.");

    size_t count = dynamic_array_count(c);
    for (size_t i = 0; i < count;)
    {
        NotViewerShellEvent *e = *DYNAMIC_ARRAY_GET(NotViewerShellEvent **, c, i);

        if (lifetime < e->type)
        {
            dynamic_array_delete_at(c, i);
            count--;
        }
        else
        {
            e->type++;
            i++;
        }
    }
}

void process_shells(void)
{
    process_shell_collection(shoots, SHOOT_LIFETIME);
    process_shell_collection(explosions, EXPLOSION_LIFETIME);
}

DynamicArray *viewer_get_shoots(void)
{
    return shoots;
}

DynamicArray *viewer_get_explosions(void)
{
    return explosions;
}
