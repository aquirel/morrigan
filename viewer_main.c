// viewer_main.c - main() for viewer.

#include "viewer.h"

#include "debug.h"
#include "client_net.h"
#include "tank.h"

SOCKET s = INVALID_SOCKET;
bool connected = false;
Landscape *l = NULL;
ResGetTanksTankRecord tanks[MAX_CLIENTS];
size_t tanks_count = 0;
#define TANKS_POLL_INTERVAL 1000

int w = 800, h = 600, bpp = 32;
const double move_speed = 4.0, mouse_sensitivity = 0.1;

#define DISPLAY_LISTS_COUNT 3
#define LANDSCAPE_DISPLAY_LIST   0
#define TANK_BODY_DISPLAY_LIST   1
#define TANK_TURRET_DISPLAY_LIST 2
GLuint display_lists = 0;

double camera_x, camera_y, camera_z = 512;
double vertical_angle = 90.0, horizontal_angle = 0.0;
const double fov_y = 60.0;
int mouse_prev_x, mouse_prev_y;
SDL_TimerID timer_id = NULL, tanks_timer_id = NULL;
#define TIMER_EVENT_ID 1
#define TANKS_TIMER_EVENT_ID 2

bool init_video(void);
bool process_events(bool *need_redraw);
void draw(const Landscape *l);
void draw_landscape(const Landscape *l);
void draw_tank_body(void);
void draw_tank_turret(void);
void draw_tank(const ResGetTanksTankRecord *tank);
void draw_tanks(void);
void cleanup(void);

Uint32 timer_handler(Uint32 interval, void *param);
Uint32 tanks_timer_handler(Uint32 interval, void *param);

double range_angle(double a);

int main(int argc, char *argv[])
{
    if (2 > argc)
    {
        fprintf(stderr, "Usage: %s server-address\n", argv[0]);
        return -1;
    }

    check(client_net_start(), "Failed to initialize net.", "");
    check(client_connect(&s, argv[1], false), "Failed to connect.", "");
    connected = true;

    puts("Connected to server.");

    l = client_get_landscape(&s);
    puts("Loaded landscape.");

    camera_x = camera_y = l->landscape_size / 2.0 * l->tile_size;

    check(init_video(), "Failed to init video.", "");

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

    while (true)
    {
        bool need_redraw = false;
        if (!process_events(&need_redraw))
        {
            break;
        }

        if (need_redraw)
        {
            draw(l);
        }
    }

    cleanup();
    return 0;

    error:
    cleanup();
    return -1;
}

bool init_video(void)
{
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
    w = 800;
    h = 600;
    check(NULL != SDL_SetVideoMode(w, h, bpp, SDL_HWSURFACE | SDL_SWSURFACE | SDL_OPENGL), "Failed to set video mode.", "");
    SDL_ShowCursor(SDL_DISABLE);
    check(NULL != (timer_id = SDL_AddTimer(SDL_DEFAULT_REPEAT_INTERVAL, timer_handler, NULL)), "Failed to setup timer.", "");
    check(NULL != (tanks_timer_id = SDL_AddTimer(TANKS_POLL_INTERVAL, tanks_timer_handler, NULL)), "Failed to setup tanks timer.", "");

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

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float ratio = (float) w / (float) h;
    gluPerspective(fov_y, ratio, 1, 16384.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScaled(1.0, 1.0, -1.0);

    SDL_WarpMouse(mouse_prev_x = w / 2, mouse_prev_y = h / 2);
    return true;
    error:
    return false;
}

bool process_events(bool *need_redraw)
{
    static bool w_pressed = false,
                s_pressed = false,
                a_pressed = false,
                d_pressed = false,
                f_pressed = false,
                v_pressed = false;
    SDL_Event event;

    if (!SDL_WaitEvent(&event))
    {
        return false;
    }

    do
    {
        bool key_event = false;

        switch (event.type)
        {
            case SDL_QUIT:
                return false;

            case SDL_ACTIVEEVENT:
            case SDL_SYSWMEVENT:
                *need_redraw = true;
                return true;

            case SDL_USEREVENT:
                switch (event.user.code)
                {
                    case TIMER_EVENT_ID:
                        if (w_pressed ||
                            s_pressed ||
                            a_pressed ||
                            d_pressed ||
                            f_pressed ||
                            v_pressed)
                        {
                            key_event = true;
                            *need_redraw = true;
                        }
                       break;

                    case TANKS_TIMER_EVENT_ID:
                        *need_redraw = true;
                        break;

                    default:
                        break;
                }

                break;

            case SDL_KEYDOWN:
                key_event = true;

                switch (event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        return false;

                    case SDLK_f:
                    case SDLK_HOME:
                    case SDLK_PAGEUP:
                        f_pressed = true;
                        break;

                    case SDLK_v:
                    case SDLK_END:
                    case SDLK_PAGEDOWN:
                        v_pressed = true;
                        break;

                    case SDLK_w:
                    case SDLK_UP:
                        w_pressed = true;
                        break;

                    case SDLK_s:
                    case SDLK_DOWN:
                        s_pressed = true;
                        break;

                    case SDLK_a:
                    case SDLK_LEFT:
                        a_pressed = true;
                        break;

                    case SDLK_d:
                    case SDLK_RIGHT:
                        d_pressed = true;
                        break;

                    default:
                        break;
                }
                break;

            case SDL_KEYUP:
                key_event = true;

                switch (event.key.keysym.sym)
                {
                    case SDLK_f:
                    case SDLK_HOME:
                    case SDLK_PAGEUP:
                        f_pressed = false;
                        break;

                    case SDLK_v:
                    case SDLK_END:
                    case SDLK_PAGEDOWN:
                        v_pressed = false;
                        break;

                    case SDLK_w:
                    case SDLK_UP:
                        w_pressed = false;
                        break;

                    case SDLK_s:
                    case SDLK_DOWN:
                        s_pressed = false;
                        break;

                    case SDLK_a:
                    case SDLK_LEFT:
                        a_pressed = false;
                        break;

                    case SDLK_d:
                    case SDLK_RIGHT:
                        d_pressed = false;
                        break;

                    default:
                        break;
                }
                break;

            case SDL_MOUSEMOTION:
                {
                    int xrel = event.motion.x - mouse_prev_x,
                        yrel = event.motion.y - mouse_prev_y;

                    horizontal_angle -= xrel * mouse_sensitivity;
                    vertical_angle += yrel * mouse_sensitivity;

                    horizontal_angle = range_angle(horizontal_angle);
                    vertical_angle = range_angle(vertical_angle);

                    SDL_WarpMouse(mouse_prev_x = w / 2, mouse_prev_y = h / 2);
                    *need_redraw = true;
                }
                break;
        }

        if (!key_event)
        {
            continue;
        }

        if (w_pressed)
        {
            camera_x -= move_speed * sin(horizontal_angle / 180 * M_PI) * sin(vertical_angle / 180 * M_PI);
            camera_y -= move_speed * cos(horizontal_angle / 180 * M_PI) * sin(vertical_angle / 180 * M_PI);
            camera_z += move_speed * cos(vertical_angle / 180 * M_PI);
            *need_redraw = true;
        }

        if (s_pressed)
        {
            camera_x += move_speed * sin(horizontal_angle / 180 * M_PI) * sin(vertical_angle / 180 * M_PI);
            camera_y += move_speed * cos(horizontal_angle / 180 * M_PI) * sin(vertical_angle / 180 * M_PI);
            camera_z -= move_speed * cos(vertical_angle / 180 * M_PI);
            *need_redraw = true;
        }

        if (a_pressed)
        {
            camera_x -= move_speed * cos(horizontal_angle / 180 * M_PI);
            camera_y += move_speed * sin(horizontal_angle / 180 * M_PI);
            *need_redraw = true;
        }

        if (d_pressed)
        {
            camera_x += move_speed * cos(horizontal_angle / 180 * M_PI);
            camera_y -= move_speed * sin(horizontal_angle / 180 * M_PI);
            *need_redraw = true;
        }

        if (f_pressed)
        {
            camera_z += move_speed;
            *need_redraw = true;
        }

        if (v_pressed)
        {
            camera_z -= move_speed;
            *need_redraw = true;
        }
    } while (SDL_PollEvent(&event));

    return true;
}

void draw(const Landscape *l)
{
    glClearColor(0, 0, 0, 0);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    glRotated(-vertical_angle, 1.0, 0.0, 0.0);
    glRotated(horizontal_angle, 0.0, 0.0, 1.0);
    glTranslated(-camera_x, -camera_y, -camera_z);

    size_t ls = l->landscape_size,
           ts = l->tile_size;

    // Light.
    GLfloat light_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);

    GLfloat light_diffuse[] = { 0.4, 0.4, 0.4, 1.0 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

    GLfloat light_specular[] = { 0.4, 0.4, 0.4, 1.0 };
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    GLfloat light_position[] = { ts * ls / 2.0, ts * ls / 2.0, 512.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    GLfloat specular_material_parameters[] = { 0.1, 0.1, 0.1, 1.0 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_material_parameters);

    glCallList(display_lists + LANDSCAPE_DISPLAY_LIST);

    draw_tanks();

    glPopMatrix();

    glFlush();
    glFinish();
    SDL_GL_SwapBuffers();
}

void draw_landscape(const Landscape *l)
{
    size_t ls = l->landscape_size,
           ts = l->tile_size;

    for (size_t i = 0; i < ls - 1; i++)
    {
        for (size_t j = 0; j < ls - 1; j++)
        {
            double x  = j * ts,
                   xp = x + ts,
                   y  = i * ts,
                   yp = y + ts;

            double h00 = landscape_get_height_at_node(l, i, j),
                   h01 = landscape_get_height_at_node(l, i, j + 1),
                   h10 = landscape_get_height_at_node(l, i + 1, j),
                   h11 = landscape_get_height_at_node(l, i + 1, j + 1);

            Vector n00, n01, n10, n11;
            landscape_get_normal_at(l, x, y, &n00);
            landscape_get_normal_at(l, xp, y, &n01);
            landscape_get_normal_at(l, x, yp, &n10);
            landscape_get_normal_at(l, xp, yp, &n11);

            glColor3ub(0xff, 0xff, 0xff);
            glBegin(GL_LINES);
            glVertex3d(x, y, h00);
            glVertex3d(x + 5 * n00.x,
                       y + 5 * n00.y,
                       h00 + 5 * n00.z);
            glEnd();

            glColor3ub(0x22, 0x8b, 0x22);
            glBegin(GL_TRIANGLE_STRIP);
            glNormal3dv((GLdouble *) &n00);
            glVertex3d(x, y, h00);
            glNormal3dv((GLdouble *) &n01);
            glVertex3d(xp, y, h01);
            glNormal3dv((GLdouble *) &n10);
            glVertex3d(x, yp, h10);
            glNormal3dv((GLdouble *) &n11);
            glVertex3d(xp, yp, h11);
            glEnd();

            glColor3ub(0x1e, 0x87, 0x1e);
            glBegin(GL_LINE_LOOP);
            glVertex3d(x, y, h00);
            glVertex3d(xp, y, h01);
            glVertex3d(xp, yp, h11);
            glVertex3d(x, yp, h10);
            glEnd();

            glBegin(GL_LINES);
            glVertex3d(x, y, h00);
            glVertex3d(xp, yp, h11);
            glVertex3d(xp, y, h01);
            glVertex3d(x, yp, h10);
            glEnd();
        }
    }
}

void draw_tank_body(void)
{
    static Vector front  = { .x =  1, .y =  0, .z =  0 },
                  back   = { .x = -1, .y =  0, .z =  0 },
                  left   = { .x =  0, .y =  1, .z =  0 },
                  right  = { .x =  0, .y = -1, .z =  0 },
                  top    = { .x =  0, .y =  0, .z =  1 },
                  bottom = { .x =  0, .y =  0, .z = -1 },
                  extent = TANK_BOUNDING_BOX_EXTENT,
                  scaled_front,
                  scaled_back,
                  scaled_left,
                  scaled_right,
                  scaled_top,
                  scaled_bottom,
                  t;

    vector_scale(&front,  extent.x, &scaled_front);
    vector_scale(&back,   extent.x, &scaled_back);
    vector_scale(&left,   extent.y, &scaled_left);
    vector_scale(&right,  extent.y, &scaled_right);
    vector_scale(&top,    extent.z, &scaled_top);
    vector_scale(&bottom, extent.z, &scaled_bottom);

    glTranslated(0, 0, extent.z);

    glColor3ub(0x35, 0x5e, 0x3b);

    glBegin(GL_QUADS);
    // Front face.
    glNormal3dv((double *) &front);  vector_add(&scaled_front, &scaled_left, &t);   vector_add(&t, &scaled_top, &t);    glVertex3dv((double *) &t);
    glNormal3dv((double *) &front);  vector_add(&scaled_front, &scaled_right, &t);  vector_add(&t, &scaled_top, &t);    glVertex3dv((double *) &t);
    glNormal3dv((double *) &front);  vector_add(&scaled_front, &scaled_right, &t);  vector_add(&t, &scaled_bottom, &t); glVertex3dv((double *) &t);
    glNormal3dv((double *) &front);  vector_add(&scaled_front, &scaled_left, &t);   vector_add(&t, &scaled_bottom, &t); glVertex3dv((double *) &t);

    // Back face.
    glNormal3dv((double *) &back);   vector_add(&scaled_back, &scaled_left, &t);    vector_add(&t, &scaled_top, &t);    glVertex3dv((double *) &t);
    glNormal3dv((double *) &back);   vector_add(&scaled_back, &scaled_left, &t);    vector_add(&t, &scaled_bottom, &t); glVertex3dv((double *) &t);
    glNormal3dv((double *) &back);   vector_add(&scaled_back, &scaled_right, &t);   vector_add(&t, &scaled_bottom, &t); glVertex3dv((double *) &t);
    glNormal3dv((double *) &back);   vector_add(&scaled_back, &scaled_right, &t);   vector_add(&t, &scaled_top, &t);    glVertex3dv((double *) &t);

    // Left face.
    glNormal3dv((double *) &left);   vector_add(&scaled_left, &scaled_front, &t);   vector_add(&t, &scaled_top, &t);    glVertex3dv((double *) &t);
    glNormal3dv((double *) &left);   vector_add(&scaled_left, &scaled_front, &t);   vector_add(&t, &scaled_bottom, &t); glVertex3dv((double *) &t);
    glNormal3dv((double *) &left);   vector_add(&scaled_left, &scaled_back, &t);    vector_add(&t, &scaled_bottom, &t); glVertex3dv((double *) &t);
    glNormal3dv((double *) &left);   vector_add(&scaled_left, &scaled_back, &t);    vector_add(&t, &scaled_top, &t);    glVertex3dv((double *) &t);

    // Right face.
    glNormal3dv((double *) &right);  vector_add(&scaled_right, &scaled_front, &t);  vector_add(&t, &scaled_top, &t);    glVertex3dv((double *) &t);
    glNormal3dv((double *) &right);  vector_add(&scaled_right, &scaled_back, &t);   vector_add(&t, &scaled_top, &t);    glVertex3dv((double *) &t);
    glNormal3dv((double *) &right);  vector_add(&scaled_right, &scaled_back, &t);   vector_add(&t, &scaled_bottom, &t); glVertex3dv((double *) &t);
    glNormal3dv((double *) &right);  vector_add(&scaled_right, &scaled_front, &t);  vector_add(&t, &scaled_bottom, &t); glVertex3dv((double *) &t);

    // Top face.
    glNormal3dv((double *) &top);    vector_add(&scaled_top, &scaled_front, &t);    vector_add(&t, &scaled_left, &t);   glVertex3dv((double *) &t);
    glNormal3dv((double *) &top);    vector_add(&scaled_top, &scaled_back, &t);     vector_add(&t, &scaled_left, &t);   glVertex3dv((double *) &t);
    glNormal3dv((double *) &top);    vector_add(&scaled_top, &scaled_back, &t);     vector_add(&t, &scaled_right, &t);  glVertex3dv((double *) &t);
    glNormal3dv((double *) &top);    vector_add(&scaled_top, &scaled_front, &t);    vector_add(&t, &scaled_right, &t);  glVertex3dv((double *) &t);

    // Bottom face.
    glNormal3dv((double *) &bottom); vector_add(&scaled_bottom, &scaled_front, &t); vector_add(&t, &scaled_left, &t);   glVertex3dv((double *) &t);
    glNormal3dv((double *) &bottom); vector_add(&scaled_bottom, &scaled_front, &t); vector_add(&t, &scaled_right, &t);  glVertex3dv((double *) &t);
    glNormal3dv((double *) &bottom); vector_add(&scaled_bottom, &scaled_back, &t);  vector_add(&t, &scaled_right, &t);  glVertex3dv((double *) &t);
    glNormal3dv((double *) &bottom); vector_add(&scaled_bottom, &scaled_back, &t);  vector_add(&t, &scaled_left, &t);   glVertex3dv((double *) &t);
    glEnd();
}

void draw_tank_turret(void)
{
    static Vector extent = TANK_BOUNDING_BOX_EXTENT;
    static double gun_radius = 0.25,
                  gun_length,
                  gun_slices = 8;
    gun_length = extent.x * 1.5;

    GLUquadric *q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluQuadricOrientation(q, GLU_OUTSIDE);
    assert(q && "Failed to create quadric.");

    glTranslated(0, 0, extent.z);
    gluSphere(q, TANK_BOUNDING_SPHERE_RADIUS, 32, 32);

    glTranslated(0, 0, TANK_BOUNDING_SPHERE_RADIUS / 2);
    glRotated(90, 0, 0, 1);
    glRotated(90, 1, 0, 0);
    glColor3ub(0x35, 0x5e, 0x3b);
    gluCylinder(q, gun_radius, gun_radius, gun_length, gun_slices, gun_slices);

    glTranslated(0, 0, gun_length);
    gluDisk(q, 0.0, gun_radius, gun_slices, gun_slices);

    gluDeleteQuadric(q);
}

void draw_tank(const ResGetTanksTankRecord *tank)
{
    static Vector tank_colors[] =
    {
        { 0.0,  0.0,  0.0  },
        { 0.0,  0.0,  0.5  },
        { 0.0,  0.5,  0.0  },
        { 0.0,  0.5,  0.5  },
        { 0.5,  0.0,  0.0  },
        { 0.5,  0.0,  0.5  },
        { 0.5,  0.5,  0.0  },
        { 0.75, 0.75, 0.75 },
        { 0.5,  0.5,  0.5  },
        { 0.0,  0.0,  1.0  },
        { 0.0,  1.0,  0.0  },
        { 0.0,  1.0,  1.0  },
        { 1.0,  0.0,  0.0  },
        { 1.0,  0.0,  1.0  },
        { 1.0,  1.0,  0.0  },
        { 1.0,  1.0,  1.0  }
    };

    assert(tank && "Bad tank pointer.");

    glPushMatrix();
    glTranslated(tank->x, tank->y, tank->z);

    Vector default_direction   = { .x = 1.0, .y = 0.0, .z = 0.0 },
           tank_direction      = { .x = tank->direction_x, .y = tank->direction_y, .z = tank->direction_z },
           rotation_axis;

    if (0 != memcmp(&default_direction, &tank_direction, sizeof(Vector)))
    {
        vector_vector_mul(&default_direction, &tank_direction, &rotation_axis);
        VECTOR_NORMALIZE(&rotation_axis);
        double angle = acos(vector_mul(&default_direction, &tank_direction)) * 180 / M_PI;
        glRotated(angle, rotation_axis.x, rotation_axis.y, rotation_axis.z);
    }

    glCallList(display_lists + TANK_BODY_DISPLAY_LIST);

    Vector default_turret_look = { .x = 1.0, .y = 0.0, .z = 0.0 },
           turret_look         = { .x = tank->turret_x, .y = tank->turret_y, .z = tank->turret_z };

    if (0 != memcmp(&default_turret_look, &turret_look, sizeof(Vector)))
    {
        vector_vector_mul(&default_turret_look, &turret_look, &rotation_axis);
        VECTOR_NORMALIZE(&rotation_axis);
        double angle = acos(vector_mul(&default_turret_look, &turret_look)) * 180 / M_PI;
        glRotated(angle, rotation_axis.x, rotation_axis.y, rotation_axis.z);
    }

    glColor3dv((double *) &tank_colors[tank->team]);

    glCallList(display_lists + TANK_TURRET_DISPLAY_LIST);

    glPopMatrix();
}

void draw_tanks(void)
{
    for (size_t i = 0; i < tanks_count; i++)
    {
        draw_tank(&tanks[i]);
    }
}

void cleanup(void)
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

    if (connected)
    {
        check(client_disconnect(&s, false), "Failed to disconnect.", "");
        connected = false;
    }

    static bool net_stopped = false;
    error:
    if (!net_stopped)
    {
        client_net_stop();
        net_stopped = true;
    }
}

Uint32 timer_handler(Uint32 interval, void *param)
{
    SDL_Event event =
    {
        .user =
        {
            .type = SDL_USEREVENT,
            .code = TIMER_EVENT_ID,
            .data1 = NULL,
            .data2 = NULL
        }
    };
    check(0 == SDL_PushEvent(&event), "SDL_PushEvent() failed.", "");
    error:
    return interval;
}

Uint32 tanks_timer_handler(Uint32 interval, void *param)
{
    printf("Got %d tanks.\n", tanks_count = client_get_tanks(&s, tanks));

    SDL_Event event =
    {
        .user =
        {
            .type = SDL_USEREVENT,
            .code = TANKS_TIMER_EVENT_ID,
            .data1 = NULL,
            .data2 = NULL
        }
    };
    check(0 == SDL_PushEvent(&event), "SDL_PushEvent() failed.", "");
    error:
    return interval;
}

double range_angle(double a)
{
    if (a < 0.0)
    {
        return 360.0;
    }
    else if (a > 360.0)
    {
        return 0.0;
    }

    return a;
}
