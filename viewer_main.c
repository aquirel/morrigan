// viewer_main.c - main() for viewer.

#include "viewer.h"

#include "debug.h"
#include "client_net.h"

SOCKET s;
bool connected = false;
Landscape *l = NULL;
Vector *landscape_normals = NULL;
///double *landscape_vertex_map;

int w = 640, h = 480, bpp = 32;
double camera_x, camera_y, camera_z = 512;
double vertical_angle = 90.0, horizontal_angle = 0.0;
const double fov_y = 60.0;
const double move_speed = 2.0, mouse_sensitivity = 0.1;
int mouse_prev_x, mouse_prev_y;

bool init_video(void);
bool process_events(bool *need_redraw);
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
    check_mem(landscape_normals = (Vector *) calloc(l->landscape_size * l->landscape_size, sizeof(Vector)));

    for (size_t i = 0; i < l->landscape_size; i++)
    {
        for (size_t j = 0; j < l->landscape_size; j++)
        {
            landscape_get_normal_at(l, j * l->tile_size, i * l->tile_size, &landscape_normals[i * l->landscape_size + j]);
        }
    }

    puts("Loaded landscape.");

    camera_x = camera_y = l->landscape_size / 2.0 * l->tile_size;

    check(init_video(), "Failed to init video.", "");

    /// TODO: Prepare vertex & normal arrays.
    ///glVertexPointer(3, GL_DOUBLE, 0, )

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
    check(0 == SDL_Init(SDL_INIT_VIDEO), "Failed to init SDL.", "");
    SDL_WM_SetCaption("morrigan viewer", NULL);
    check(0 == SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16), "Failed to set SDL_GL_DEPTH_SIZE.", "");
    check(0 == SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "Failed to set SDL_GL_DOUBLEBUFFER.", "");
    const SDL_VideoInfo *vi = SDL_GetVideoInfo();
    check(NULL != vi, "SDL_GetVideoInfo() failed.", "");
    //w = vi->current_w;
    //h = vi->current_h;
    w = 800;
    h = 600;
    check(NULL != SDL_SetVideoMode(w, h, bpp, SDL_HWSURFACE | SDL_SWSURFACE | SDL_OPENGL), "Failed to set video mode.", "");
    SDL_ShowCursor(SDL_DISABLE);
    check(0 == SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL), "Failed to set key repeat.", "");

    glLineWidth(0.5);

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

    ///glEnableClientState(GL_VERTEX_ARRAY);
    ///glEnableClientState(GL_NORMAL_ARRAY);

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
    SDL_Event event;

    SDL_WaitEvent(&event);

    switch (event.type)
    {
        case SDL_QUIT:
            return false;

        case SDL_ACTIVEEVENT:
        case SDL_SYSWMEVENT:
            *need_redraw = true;
            return true;

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
                    *need_redraw = true;
                    break;

                case SDLK_v:
                case SDLK_END:
                case SDLK_PAGEDOWN:
                    camera_z -= move_speed;
                    *need_redraw = true;
                    break;

                case SDLK_w:
                case SDLK_UP:
                    camera_x -= move_speed * sin(horizontal_angle / 180 * M_PI) * sin(vertical_angle / 180 * M_PI);
                    camera_y -= move_speed * cos(horizontal_angle / 180 * M_PI) * sin(vertical_angle / 180 * M_PI);
                    camera_z += move_speed * cos(vertical_angle / 180 * M_PI);
                    *need_redraw = true;
                    break;

                case SDLK_s:
                case SDLK_DOWN:
                    camera_x += move_speed * sin(horizontal_angle / 180 * M_PI) * sin(vertical_angle / 180 * M_PI);
                    camera_y += move_speed * cos(horizontal_angle / 180 * M_PI) * sin(vertical_angle / 180 * M_PI);
                    camera_z -= move_speed * cos(vertical_angle / 180 * M_PI);
                    *need_redraw = true;
                    break;

                case SDLK_a:
                case SDLK_LEFT:
                    camera_x -= move_speed * cos(horizontal_angle / 180 * M_PI);
                    camera_y += move_speed * sin(horizontal_angle / 180 * M_PI);
                    *need_redraw = true;
                    break;

                case SDLK_d:
                case SDLK_RIGHT:
                    camera_x += move_speed * cos(horizontal_angle / 180 * M_PI);
                    camera_y -= move_speed * sin(horizontal_angle / 180 * M_PI);
                    *need_redraw = true;
                    break;

                default:
                    return true;
            }
            break;

        case SDL_MOUSEMOTION:
            {
                int xrel = event.motion.x - mouse_prev_x,
                    yrel = event.motion.y - mouse_prev_y;

                horizontal_angle -= xrel * mouse_sensitivity;
                vertical_angle += yrel * mouse_sensitivity;

                horizontal_angle = __range_angle(horizontal_angle);
                vertical_angle = __range_angle(vertical_angle);

                SDL_WarpMouse(mouse_prev_x = w / 2, mouse_prev_y = h / 2);
                *need_redraw = true;
            }
            break;
    }

    return true;
}

void draw(const Landscape *l)
{
    glClearColor(0, 0, 0, 0);
    glClearDepth(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    glRotated(-vertical_angle, 1.0, 0.0, 0.0);
    glRotated(horizontal_angle, 0.0, 0.0, 1.0);
    glTranslated(-camera_x, -camera_y, -camera_z);

    // Draw landscape.
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

    for (size_t i = 0; i < ls; i++)
    {
        for (size_t j = 0; j < ls; j++)
        {
            // TODO: Move it to vertex array.
            double h00 = landscape_get_height_at_node(l, i, j),
                   h01 = landscape_get_height_at_node(l, i, j + 1),
                   h10 = landscape_get_height_at_node(l, i + 1, j),
                   h11 = landscape_get_height_at_node(l, i + 1, j + 1);

            double x  = j * ts,
                   xp = x + ts,
                   y  = i * ts,
                   yp = y + ts;

            // TODO: Move it to normals array.
            Vector *n00 = &landscape_normals[i * ls + j],
                   *n01 = &landscape_normals[i * ls + j + 1],
                   *n10 = &landscape_normals[(i + 1) * ls + j],
                   *n11 = &landscape_normals[(i + 1) * ls + j + 1];

            glColor3ub(0x22, 0x8b, 0x22);
            glBegin(GL_TRIANGLE_STRIP);
            glNormal3dv((GLdouble *) n00);
            glVertex3d(x, y, h00);
            glNormal3dv((GLdouble *) n01);
            glVertex3d(xp, y, h01);
            glNormal3dv((GLdouble *) n10);
            glVertex3d(x, yp, h10);
            glNormal3dv((GLdouble *) n11);
            glVertex3d(xp, yp, h11);
            glEnd();

            glColor3ub(0x1e, 0x87, 0x1e);
            glBegin(GL_LINE_LOOP);
            glVertex3d(x, y, h00);
            glVertex3d(xp, y, h01);
            glVertex3d(x, yp, h10);
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

    if (landscape_normals)
    {
        free(landscape_normals);
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
