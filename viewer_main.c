//viewer_main.c - main() for viewer.

#include "viewer.h"

#include "debug.h"
#include "client_net.h"

SOCKET s;
bool connected = false;
Landscape *l = NULL;

bool init_video(void);
bool process_events(void);
void draw(const Landscape *l);
void cleanup(void);

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
    const int w = 640, h = 480, bpp = 16;

    check(0 == SDL_Init(SDL_INIT_VIDEO), "Failed to init SDL.", "");
    SDL_WM_SetCaption("morrigan viewer", NULL);
    check(0 == SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16), "Failed to set SDL_GL_DEPTH_SIZE.", "");
    check(0 == SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "Failed to set SDL_GL_DOUBLEBUFFER.", "");
    check(NULL != SDL_SetVideoMode(w, h, bpp, SDL_OPENGL), "Failed to set video mode.", "");

    glLineWidth(2.0);

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

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(0, 0, w, h);
    float ratio = (float) w / (float) h;
    gluPerspective(60.0, ratio, 1.0, 4096.0);

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
    }

    return true;
}

void draw(const Landscape *l)
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluLookAt(0, 0, 0, 0, 0, 1, 1, 1, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Light.
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    GLfloat light_ambient[] = { 0.25f, 0.25f, 0.25f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    GLfloat light_position[] = { 0, 0, 8, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    // Draw axis.
    size_t ls = l->landscape_size,
           ts = l->tile_size;

    glColor3d(1, 0, 0);
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(ls * ts, 0, 0);
    glEnd();

    glColor3d(0, 1, 0);
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(0, ls * ts, 0);
    glEnd();

    glColor3d(0, 0, 1);
    glBegin(GL_LINES);
    glVertex3d(0, 0, 0);
    glVertex3d(0, 0, ls * ts);
    glEnd();

    // Draw landscape.
    glColor3d(0, 0.5, 0);
    for (size_t i = 0; i < ls - 1; i++)
    {
        for (size_t j = 0; j < ls - 1; j++)
        {
            glBegin(GL_TRIANGLE_STRIP);

            glVertex3d(j * ts, i * ts, landscape_get_height_at_node(l, i, j));
            glVertex3d((j + 1) * ts, i * ts, landscape_get_height_at_node(l, i, j + 1));
            glVertex3d(j * ts, (i + 1) * ts, landscape_get_height_at_node(l, i + 1, j));
            glVertex3d((j + 1) * ts, (i + 1) * ts, landscape_get_height_at_node(l, i + 1, j + 1));

            glEnd();
        }
    }

    ///
    glColor3d(0.5, 0.5, 0.5);
    GLUquadric *q = gluNewQuadric();
    if (q)
    {
        gluQuadricDrawStyle(q, GLU_FILL);
        gluQuadricNormals(q, GLU_FLAT);

        gluSphere(q, 0.25, 64, 64);

        glPushMatrix();
        glTranslated(0.25, 0, 0);
        glColor3d(0.5, 0, 0);
        gluSphere(q, 0.125, 64, 64);
        glPopMatrix();

        glPushMatrix();
        glTranslated(0, 0.25, 0);
        glColor3d(0, 0.5, 0);
        gluSphere(q, 0.125, 64, 64);
        glPopMatrix();

        glPushMatrix();
        glTranslated(0, 0, 0.25);
        glColor3d(0, 0, 0.5);
        gluSphere(q, 0.125, 64, 64);
        glPopMatrix();

        gluDeleteQuadric(q);
    }
    ///

    SDL_GL_SwapBuffers( );
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
