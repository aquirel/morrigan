// viewer_events.c - process_events() function.

#include "viewer.h"

bool process_events(bool *need_redraw, Camera *camera)
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
                    int xrel = event.motion.x - camera->mouse_prev_x,
                        yrel = event.motion.y - camera->mouse_prev_y;

                    camera->horizontal_angle -= xrel * camera->mouse_sensitivity;
                    camera->vertical_angle += yrel * camera->mouse_sensitivity;

                    camera->horizontal_angle = range_angle(camera->horizontal_angle);
                    camera->vertical_angle = range_angle(camera->vertical_angle);

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
