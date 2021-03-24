#include "application.h"


Event
Event_ConvertFromSDLScancode(uint8 const* scan_state)
{
    if (scan_state[SDL_SCANCODE_ESCAPE])
    {
        return EVENT_QUIT;
    }
    return EVENT_NO_EVENT;
}

Event
Event_ConvertFromSDLEvent(SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_QUIT:
        return EVENT_QUIT;
    case SDL_WINDOWEVENT:
        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_RESIZED:
            return EVENT_WINDOW_RESIZED;
        case SDL_WINDOWEVENT_MOVED:
            return EVENT_WINDOW_MOVED;
        }
    }
    return EVENT_NO_EVENT;
}

void
Event_UpdateKeyState(KeyState& state, int current_value)
{
    if (current_value)
    {
        if (state == KEY_RELEASED)
        {
            state += 1;
        }
        else if (state == KEY_PRESSED)
        {
            state += 1;
        }
        else if (state == KEY_HELD)
        {
            // do nothing
        }
    }
    else
    {
        if (state == KEY_RELEASED)
        {
            // do nothing
        }
        else if (state == KEY_PRESSED)
        {
            state = KEY_RELEASED;
        }
        else if (state == KEY_HELD)
        {
            state = KEY_RELEASED;
        }
    }
}

void
Event_PollSDLEvents(EventManager& event_manager)
{
    uint8 const* keyboard_state = SDL_GetKeyboardState(NULL);
    // Process codes that become events, for example escape becomes QUIT.
    {
        Event event = Event_ConvertFromSDLScancode(keyboard_state);

        event_manager.event_table[event] = 1;
    }

    // Process regular keys.
    {
        for (auto& binding : event_manager.key_bindings)
        {
            auto scan_code = SCANCODE_TABLE[binding.ch - 'a'];
            auto key_state = keyboard_state[scan_code];

            Event_UpdateKeyState(binding.state, key_state);
        }
    }

    {
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event) != 0)
        {
            Event event = Event_ConvertFromSDLEvent(sdl_event);

            event_manager.event_table[event] = 1;
        }
    }
}

void
Event_PollTimeEvents(EventManager& event_manager)
{
    auto dcount          = ElapsedCount(event_manager.clk);
    event_manager.dcount = dcount;

    for (auto timer : event_manager.timers)
    {
        timer.counter += dcount;
        if (timer.counter > timer.count)
        {
            timer.counter -= timer.count;
            event_manager.event_table[timer.event] += 1;
        }
    }
}

void
Event_Poll(EventManager& event_manager)
{
    Event_PollSDLEvents(event_manager);
    Event_PollTimeEvents(event_manager);
}

int
Event_QueryAndReset(EventTable& table, int event, int clear_value)
{
    auto result  = table[event];
    table[event] = clear_value;
    return result;
}

Window
Make_Window(int screen_width, int screen_height, bool hw_acceleration)
{

    Window      the_window;
    SDL_Window* window;
    {
        window = SDL_CreateWindow("Window Resizing",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  screen_width,
                                  screen_height,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

        if (!window)
        {
            SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                         "Could not create window: %s.\n",
                         SDL_GetError());
            the_window.error = SYS_CREATE_WINDOW_ERROR;
            return the_window;
        }
    }

    SDL_Renderer* renderer;
    {
        auto renderer_mode = hw_acceleration
            ? (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
            : SDL_RENDERER_SOFTWARE;

        // -1 to initialize the first driver supporting the requested flags.
        renderer = SDL_CreateRenderer(window, -1, renderer_mode);

        if (!renderer)
        {
            SDL_LogError(SDL_LOG_CATEGORY_SYSTEM,
                         "Could not create renderer: %s.\n",
                         SDL_GetError());
            the_window.error = SYS_CREATE_RENDERER_ERROR;
            return the_window;
        }
    }

    the_window.window   = window;
    the_window.renderer = renderer;
    SDL_GetWindowSize(the_window.window, &the_window.w, &the_window.h);
    SDL_GetWindowPosition(the_window.window, &the_window.x, &the_window.y);
    return the_window;
}

void
Free_Window(Window& window)
{
    SDL_DestroyWindow(window.window);
    SDL_DestroyRenderer(window.renderer);
}

void
Window_Clear(Window& window)
{
    SDL_SetRenderTarget(window.renderer, nullptr);
    SDL_SetRenderDrawColor(window.renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(window.renderer);
}

void
Window_PresentRenderer(Window& window)
{
    SDL_RenderPresent(window.renderer);
}

void
Window_HandleEvents(Window& window, EventTable& events)
{
    if (Event_QueryAndReset(events, EVENT_WINDOW_RESIZED, 0))
    {
        printf("resized\n");
        SDL_GetWindowSize(window.window, &window.w, &window.h);
    }
    if (Event_QueryAndReset(events, EVENT_WINDOW_MOVED, 0))
    {
        SDL_GetWindowPosition(window.window, &window.x, &window.y);
        printf("moved %d %d\n", window.x, window.y);
    }
}

void
Window_LoadState(Window* window)
{
    SDL_SetWindowSize(window->window, window->w, window->h);
    SDL_SetWindowPosition(window->window, window->x, window->y);
}