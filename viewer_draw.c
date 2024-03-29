// viewer_draw.c - drawing routines.

#include "viewer.h"

static void __draw_shell_event(const NotViewerShellEvent *e, double radius, double delay);

static void __draw_tank(const ResGetTanksTankRecord *tank, const GLuint *display_lists)
{
    static Vector tank_colors[] = {
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
    assert(display_lists && "Bad display lists pointer.");

    glPushMatrix();
    glTranslated(tank->x, tank->y, tank->z);

    Vector tank_direction   = { .x = tank->direction_x,   .y = tank->direction_y,   .z = tank->direction_z   },
           tank_orientation = { .x = tank->orientation_x, .y = tank->orientation_y, .z = tank->orientation_z },
           side;

    VECTOR_NORMALIZE(&tank_direction);
    VECTOR_NORMALIZE(&tank_orientation);

    vector_vector_mul(&tank_orientation, &tank_direction, &side);
    VECTOR_NORMALIZE(&side);

    double m[] = {
        tank_direction.x,   tank_direction.y,   tank_direction.z,   0.0,
        side.x,             side.y,             side.z,             0.0,
        tank_orientation.x, tank_orientation.y, tank_orientation.z, 0.0,
        0.0,                0.0,                0.0,                1.0
    };
    glMultMatrixd(m);

    glColor4ub(0x35, 0x5e, 0x3b, tank->hp ? 0xff : 64);
    glCallList(*display_lists + TANK_BODY_DISPLAY_LIST);

    glColor4d(tank_colors[tank->team].x, tank_colors[tank->team].y, tank_colors[tank->team].z, tank->hp ? 1.0 : 0.25);
    glCallList(*display_lists + TANK_TURRET_DISPLAY_LIST);

    glTranslated(0, 0, TANK_BOUNDING_SPHERE_RADIUS / 2.0);

    Vector default_turret_look = { .x = 1, .y = 0, .z = 0 },
           turret_look         = { .x = tank->turret_x, .y = tank->turret_y, .z = tank->turret_z };

    if (0 != memcmp(&default_turret_look, &turret_look, sizeof(Vector)))
    {
        vector_get_orthogonal(&turret_look, &side);
        VECTOR_NORMALIZE(&side);

        Vector top;
        vector_vector_mul(&turret_look, &side, &top);
        VECTOR_NORMALIZE(&top);

        double m[] = {
            turret_look.x,      turret_look.y,      turret_look.z,      0,
            side.x,             side.y,             side.z,             0,
            top.x,              top.y,              top.z,              0,
            0,                  0,                  0,                  1
        };
        glMultMatrixd(m);
    }

    glColor4ub(0x35, 0x5e, 0x3b, tank->hp ? 0xff : 64);
    glCallList(*display_lists + TANK_GUN_DISPLAY_LIST);

    glPopMatrix();
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
    GLUquadric *q = gluNewQuadric();
    assert(q && "Failed to create quadric.");
    gluQuadricNormals(q, GLU_SMOOTH);
    gluQuadricOrientation(q, GLU_OUTSIDE);
    glTranslated(2, 0, extent.z);
    gluSphere(q, TANK_BOUNDING_SPHERE_RADIUS, 32, 32);
    gluDeleteQuadric(q);
}

void draw_tank_gun(void)
{
    static double gun_radius = 0.25;
    static size_t gun_slices = 8;

    GLUquadric *q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluQuadricOrientation(q, GLU_OUTSIDE);
    assert(q && "Failed to create quadric.");

    glRotated(90, 0, 0, 1);
    glRotated(90, 1, 0, 0);

    gluCylinder(q, gun_radius, gun_radius, TANK_GUN_LENGTH, gun_slices, gun_slices);

    glTranslated(0, 0, TANK_GUN_LENGTH);
    gluDisk(q, 0.0, gun_radius, gun_slices, gun_slices);

    gluDeleteQuadric(q);
}

static void __draw_tanks(const ResGetTanksTankRecord *tanks, size_t tanks_count, const GLuint *display_lists)
{
    assert(tanks && "Bad tanks pointer.");
    assert(display_lists && "Bad display lists pointer.");

    for (size_t i = 0; i < tanks_count; i++)
    {
        __draw_tank(&tanks[i], display_lists);
    }
}

void draw_landscape(const Landscape *l)
{
    assert(l && "Bad landscape pointer.");

    size_t ls = l->landscape_size,
           ts = l->tile_size;

    for (size_t i = 0; i < ls - 1; i++)
    {
        for (size_t j = 0; j < ls - 1; j++)
        {
            double x  = (double) (j * ts),
                   xp = (double) (x + ts),
                   y  = (double) (i * ts),
                   yp = (double) (y + ts);

            double h00 = landscape_get_height_at_node(l, i, j),
                   h01 = landscape_get_height_at_node(l, i, j + 1),
                   h10 = landscape_get_height_at_node(l, i + 1, j),
                   h11 = landscape_get_height_at_node(l, i + 1, j + 1);

            Vector n00, n01, n10, n11;
            landscape_get_normal_at(l, x, y, &n00);
            landscape_get_normal_at(l, xp, y, &n01);
            landscape_get_normal_at(l, x, yp, &n10);
            landscape_get_normal_at(l, xp, yp, &n11);

            /*glColor3ub(0xff, 0xff, 0xff);
            glBegin(GL_LINES);
            glVertex3d(x, y, h00);
            glVertex3d(x + 5 * n00.x,
                       y + 5 * n00.y,
                       h00 + 5 * n00.z);
            glEnd();*/

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

static void __draw_shell_events(const DynamicArray *shell_events, void (*f)(const NotViewerShellEvent *e))
{
    assert(shell_events && "Bad shell events array pointer.");
    assert(f && "Bad draw callback.");

    size_t c = dynamic_array_count(shell_events);
    for (size_t i = 0; i < c; i++)
    {
        NotViewerShellEvent *e = *DYNAMIC_ARRAY_GET(NotViewerShellEvent **, shell_events, i);
        f(e);
    }
}

static void __draw_shoot(const NotViewerShellEvent *shoot)
{
    assert(shoot && "Bad shoot pointer.");
    __draw_shell_event(shoot, 3.0, 8.0);
}

static void __draw_explosion(const NotViewerShellEvent *explosion)
{
    assert(explosion && "Bad explosion pointer.");
    __draw_shell_event(explosion, 5.0, 16.0);
}

static void __draw_shell_event(const NotViewerShellEvent *e, double radius, double delay)
{
    assert(e && "Bad shell event pointer.");

    glPushMatrix();
    glTranslated(e->x, e->y, e->z);

    GLUquadric *q = NULL;
    q = gluNewQuadric();
    if (q)
    {
        gluQuadricNormals(q, GLU_SMOOTH);
        gluQuadricOrientation(q, GLU_OUTSIDE);

        glColor4d(0.8, 0.0, 0.0, 0.5);
        gluSphere(q, radius / (1.0 + e->type / delay), 32, 32);

        gluDeleteQuadric(q);
    }

    glPopMatrix();
}

void draw(const Landscape *l,
          const ResGetTanksTankRecord *tanks,
          size_t tanks_count,
          const Camera *camera,
          const GLuint *display_lists,
          const DynamicArray *shoots,
          const DynamicArray *explosions)
{
    assert(l && "Bad lanscape pointer.");
    assert(tanks && "Bad tanks pointer.");
    assert(camera && "Bad camera pointer.");
    assert(display_lists && "Bad display lists pointer.");
    assert(shoots && "Bad shoots array pointer.");
    assert(explosions && "Bad explosions array pointer.");

    glClearColor(0, 0, 0, 0);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();

    glRotated(-camera->vertical_angle, 1.0, 0.0, 0.0);
    glRotated(camera->horizontal_angle, 0.0, 0.0, 1.0);
    glTranslated(-camera->x, -camera->y, -camera->z);

    size_t ls = l->landscape_size,
           ts = l->tile_size;

    // Light.
    GLfloat light_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);

    GLfloat light_diffuse[] = { 0.4, 0.4, 0.4, 1.0 };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);

    GLfloat light_specular[] = { 0.4, 0.4, 0.4, 1.0 };
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    GLfloat light_position[] = { ts * ls / 2.0F, ts * ls / 2.0F, 512.0F, 1.0F };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    GLfloat specular_material_parameters[] = { 0.1, 0.1, 0.1, 1.0 };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_material_parameters);

    glCallList(*display_lists + LANDSCAPE_DISPLAY_LIST);

    __draw_tanks(tanks, tanks_count, display_lists);

    __draw_shell_events(shoots, __draw_shoot);
    __draw_shell_events(explosions, __draw_explosion);

    glPopMatrix();

    glFlush();
    glFinish();
    SDL_GL_SwapBuffers();
}
