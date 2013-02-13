// viewer_main.c - main() for viewer.

#include "viewer.h"

#include "debug.h"
#include "client_net.h"

SOCKET s;
bool connected = false;
Landscape *l = NULL;
ResGetTanksTankRecord tanks[MAX_CLIENTS];
size_t tanks_count = 0;
bool need_redraw_tanks = false;
#define TANKS_POLL_INTERVAL 1000

int w = 800, h = 600, bpp = 32;
const double move_speed = 4.0, mouse_sensitivity = 0.1;

#define DISPLAY_LISTS_COUNT 2
#define LANDSCAPE_DISPLAY_LIST 0
#define TANK_BODY_DISPLAY_LIST 1
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
                        need_redraw_tanks = true;
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

    glPopMatrix();

    if (need_redraw_tanks)
    {
        draw_tanks();
        need_redraw_tanks = false;
    }

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
    // TODO: Write it.
}

void draw_tank(const ResGetTanksTankRecord *tank)
{
    // TODO: Write it.
    // todo: static array of team colors.
    glPushMatrix();
    /*glRotated(-vertical_angle, 1.0, 0.0, 0.0);
        glRotated(horizontal_angle, 0.0, 0.0, 1.0);
        glTranslated(-camera_x, -camera_y, -camera_z);*/
        //call list
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
    }

    if (NULL != tanks_timer_id)
    {
        SDL_RemoveTimer(tanks_timer_id);
    }

    if (0 != display_lists)
    {
        glDeleteLists(display_lists, DISPLAY_LISTS_COUNT);
    }

    if (l)
    {
        landscape_destroy(l);
    }

    if (connected)
    {
        check(client_disconnect(&s, false), "Failed to disconnect.", "");
    }

    error:
    client_net_stop();
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
    if (need_redraw_tanks)
    {
        return interval;
    }

    tanks_count = client_get_tanks(&s, tanks);
    puts("Got tanks.");

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
