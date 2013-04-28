// viewer_events.c - process_events() function.

#include "viewer.h"

static double __range_angle(double a);

bool process_events(bool *need_redraw, Camera *camera, SOCKET *s, ResGetTanksTankRecord *tanks, size_t *tanks_count)
{
    assert(need_redraw && "Bad redraw flag pointer.");
    assert(camera && "Bad camera pointer.");
    assert(s && "Bad socket pointer.");
    assert(tanks && "Bad tanks pointer.");
    assert(tanks_count && "Bad tanks count pointer.");

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
                        //move_tanks(l, tanks, tanks_count);
                        process_shells();

                        while (client_protocol_process_event(s, viewer_protocol, sizeof(viewer_protocol) / sizeof(viewer_protocol[0])));

                        if (w_pressed ||
                            s_pressed ||
                            a_pressed ||
                            d_pressed ||
                            f_pressed ||
                            v_pressed)
                        {
                            key_event = true;
                        }
                        *need_redraw = true;
                       break;

                    case TANKS_TIMER_EVENT_ID:
                        *tanks_count = client_get_tanks(false, s, tanks, viewer_protocol, sizeof(viewer_protocol) / sizeof(viewer_protocol[0]));
                        //printf("Got %d tanks.\n", tanks_count);
                        *need_redraw = true;
                        break;

                    default:
                        break;
                }

                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                {
                    key_event = true;
                    bool keydown = event.type == SDL_KEYDOWN;

                    switch (event.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                            return false;

                        case SDLK_f:
                        case SDLK_HOME:
                        case SDLK_PAGEUP:
                            f_pressed = keydown;
                            break;

                        case SDLK_v:
                        case SDLK_END:
                        case SDLK_PAGEDOWN:
                            v_pressed = keydown;
                            break;

                        case SDLK_w:
                        case SDLK_UP:
                            w_pressed = keydown;
                            break;

                        case SDLK_s:
                        case SDLK_DOWN:
                            s_pressed = keydown;
                            break;

                        case SDLK_a:
                        case SDLK_LEFT:
                            a_pressed = keydown;
                            break;

                        case SDLK_d:
                        case SDLK_RIGHT:
                            d_pressed = keydown;
                            break;

                        default:
                            break;
                    }
                }
                break;

            case SDL_MOUSEMOTION:
                {
                    int xrel = event.motion.x - camera->mouse_prev_x,
                        yrel = event.motion.y - camera->mouse_prev_y;

                    camera->horizontal_angle -= xrel * camera->mouse_sensitivity;
                    camera->vertical_angle += yrel * camera->mouse_sensitivity;

                    camera->horizontal_angle = __range_angle(camera->horizontal_angle);
                    camera->vertical_angle = __range_angle(camera->vertical_angle);

                    SDL_WarpMouse(camera->mouse_prev_x = camera->w / 2, camera->mouse_prev_y = camera->h / 2);
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
            camera->x -= camera->move_speed * sin(camera->horizontal_angle / 180 * M_PI) * sin(camera->vertical_angle / 180 * M_PI);
            camera->y -= camera->move_speed * cos(camera->horizontal_angle / 180 * M_PI) * sin(camera->vertical_angle / 180 * M_PI);
            camera->z += camera->move_speed * cos(camera->vertical_angle / 180 * M_PI);
            *need_redraw = true;
        }

        if (s_pressed)
        {
            camera->x += camera->move_speed * sin(camera->horizontal_angle / 180 * M_PI) * sin(camera->vertical_angle / 180 * M_PI);
            camera->y += camera->move_speed * cos(camera->horizontal_angle / 180 * M_PI) * sin(camera->vertical_angle / 180 * M_PI);
            camera->z -= camera->move_speed * cos(camera->vertical_angle / 180 * M_PI);
            *need_redraw = true;
        }

        if (a_pressed)
        {
            camera->x -= camera->move_speed * cos(camera->horizontal_angle / 180 * M_PI);
            camera->y += camera->move_speed * sin(camera->horizontal_angle / 180 * M_PI);
            *need_redraw = true;
        }

        if (d_pressed)
        {
            camera->x += camera->move_speed * cos(camera->horizontal_angle / 180 * M_PI);
            camera->y -= camera->move_speed * sin(camera->horizontal_angle / 180 * M_PI);
            *need_redraw = true;
        }

        if (f_pressed)
        {
            camera->z += camera->move_speed;
            *need_redraw = true;
        }

        if (v_pressed)
        {
            camera->z -= camera->move_speed;
            *need_redraw = true;
        }
    } while (SDL_PollEvent(&event));

    return true;
}

static double __range_angle(double a)
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
