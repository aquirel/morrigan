// viewer_main.c - main() for viewer.

#include "viewer.h"

#include "debug.h"
#include "client_net.h"

SOCKET s;
bool connected = false;
Landscape *l = NULL;

const int w = 640, h = 480, bpp = 16;
double camera_x, camera_y, camera_z = 512;
double vertical_angle = 90.0, horizontal_angle = 0.0;
const double fov_y = 60.0;
const double move_speed = 2.0, mouse_sensitivity = 0.1;

bool init_video(void);
bool process_events(void);
void draw(const Landscape *l);
void cleanup(void);

double __range_angle(double a);

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

    while (true)
    {
        if (!process_events())
        {
            break;
        }

        draw(l);
    }

    cleanup();
    return 0;

    error:
    cleanup(); 
    return -1;
}

bool init_video(void)
{
    check(0 == SDL_Init(SDL_INIT_VIDEO), "Failed to init SDL.", "");
    SDL_WM_SetCaption("morrigan viewer", NULL);
    check(0 == SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16), "Failed to set SDL_GL_DEPTH_SIZE.", "");
    check(0 == SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "Failed to set SDL_GL_DOUBLEBUFFER.", "");
    check(NULL != SDL_SetVideoMode(w, h, bpp, SDL_OPENGL), "Failed to set video mode.", "");
    SDL_ShowCursor(SDL_DISABLE);
    SDL_WM_GrabInput(SDL_GRAB_ON);
    check(0 == SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL), "Failed to set key repeat.", "");

    glLineWidth(1.0);

    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_FRONT);
    glEnable(GL_DITHER);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_LIGHT0);
    GLfloat ambient_params[] = { 0.5, 0.5, 0.5, 1.0 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_params);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);

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

    return true;
    error:
    return false;
}

bool process_events(void)
{
    SDL_Event event;

    SDL_WaitEvent(&event);

    switch (event.type)
    {
        case SDL_QUIT:
            return false;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            switch (event.key.keysym.sym)
            {
                case SDLK_ESCAPE:
                    return false;

                case SDLK_f:
                case SDLK_HOME:
                case SDLK_PAGEUP:
                    camera_z += move_speed;
                    break;

                case SDLK_v:
                case SDLK_END:
                case SDLK_PAGEDOWN:
                    camera_z -= move_speed;
                    break;

                case SDLK_w:
                case SDLK_UP:
                    camera_x -= move_speed * sin(horizontal_angle / 180 * M_PI);
                    camera_y -= move_speed * cos(horizontal_angle / 180 * M_PI);
                    camera_z += move_speed * cos(vertical_angle / 180 * M_PI);
                    break;

                case SDLK_s:
                case SDLK_DOWN:
                    camera_x += move_speed * sin(horizontal_angle / 180 * M_PI);
                    camera_y += move_speed * cos(horizontal_angle / 180 * M_PI);
                    camera_z -= move_speed * cos(vertical_angle / 180 * M_PI);
                    break;

                case SDLK_a:
                case SDLK_LEFT:
                    camera_x -= move_speed * cos(horizontal_angle / 180 * M_PI);
                    camera_y += move_speed * sin(horizontal_angle / 180 * M_PI);
                    break;

                case SDLK_d:
                case SDLK_RIGHT:
                    camera_x += move_speed * cos(horizontal_angle / 180 * M_PI);
                    camera_y -= move_speed * sin(horizontal_angle / 180 * M_PI);
                    break;

                default:
                    return true;
            }
            break;

        case SDL_MOUSEMOTION:
            {
                static bool first_time = true;
                if (first_time)
                {
                    first_time = false;
                    break;
                }

                int xrel = event.motion.xrel,
                    yrel = event.motion.yrel;

                horizontal_angle -= xrel * mouse_sensitivity;
                vertical_angle += yrel * mouse_sensitivity;

                horizontal_angle = __range_angle(horizontal_angle);
                vertical_angle = __range_angle(vertical_angle);
            }
            break;
    }

    return true;
}

void draw(const Landscape *l)
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    glRotated(-vertical_angle, 1.0, 0.0, 0.0);
    glRotated(horizontal_angle, 0.0, 0.0, 1.0);
    glTranslated(-camera_x, -camera_y, -camera_z);

    // Light.
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    GLfloat light_ambient[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    GLfloat light_position[] = { 0, 0, 256, 0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    // Draw landscape.
    size_t ls = l->landscape_size,
           ts = l->tile_size;

    glColor3d(0, 0.5, 0);
    for (size_t i = 0; i < ls - 1; i++)
    {
        for (size_t j = 0; j < ls - 1; j++)
        {
            ///glBegin(GL_TRIANGLE_STRIP);
            glBegin(GL_LINE_STRIP);

            glVertex3d(j * ts, i * ts, landscape_get_height_at_node(l, i, j));
            glVertex3d((j + 1) * ts, i * ts, landscape_get_height_at_node(l, i, j + 1));
            glVertex3d(j * ts, (i + 1) * ts, landscape_get_height_at_node(l, i + 1, j));
            glVertex3d((j + 1) * ts, (i + 1) * ts, landscape_get_height_at_node(l, i + 1, j + 1));

            ///
            glVertex3d(j * ts, i * ts, landscape_get_height_at_node(l, i, j));

            glEnd();
        }
    }

    glPopMatrix();

    SDL_GL_SwapBuffers();
}

void cleanup(void)
{
    if (SDL_WasInit(SDL_INIT_VIDEO))
    {
        SDL_Quit();
    }

    if (connected)
    {
        check(client_disconnect(&s, false), "Failed to disconnect.", "");
    }

    error:
    client_net_stop();
    if (l)
    {
        landscape_destroy(l);
    }
}

double __range_angle(double a)
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
