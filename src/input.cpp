/* Copyright (C) 1995-2002  FSGames. Ported by Sean Ford and Yan Shosh
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//
// input.cpp
//
// input code
//

#include "input.h"
#include <stdio.h>
#include <time.h>
#include <string.h> //buffers: for strlen
#include <string>

#ifdef USE_SDL2
#define SDL_GetKeyState SDL_GetKeyboardState
#endif

void quit(Sint32 arg1);

int raw_key;
char* raw_text_input = NULL;
short key_press_event = 0;    // used to signed key-press
short text_input_event = 0;    // used to signal text input

#ifdef ANDROID
bool tapping = false;
int start_tap_x = 0;
int start_tap_y = 0;
bool input_continue = false;  // Done with text popups, etc.
#endif

Uint8* keystates = NULL;

Sint32 mouse_state[MSTATE];
Sint32 mouse_buttons;

float mouse_scale_x = 1;
float mouse_scale_y = 1;

JoyData player_joy[4];

#define JOY_DEAD_ZONE 8000
#define MAX_NUM_JOYSTICKS 10  // Just in case there are joysticks attached that are not useable (e.g. accelerometer)
SDL_Joystick* joysticks[MAX_NUM_JOYSTICKS];

int player_keys[4][NUM_KEYS] = {
    {
        SDLK_w, SDLK_e, SDLK_d, SDLK_c,  // movements
        SDLK_x, SDLK_z, SDLK_a, SDLK_q,
        SDLK_LCTRL, SDLK_LALT,                  // fire & special
        SDLK_BACKQUOTE,                         // switch guys
        SDLK_TAB,                               // change special
        SDLK_s,                                 // Yell
        SDLK_LSHIFT,                            // Shifter
        SDLK_1,                                 // Options menu
        SDLK_F5,                                // Cheat key
    },
    {
        SDLK_UP, SDLK_UNKNOWN, SDLK_RIGHT, SDLK_UNKNOWN,  // movements
        SDLK_DOWN, SDLK_UNKNOWN, SDLK_LEFT, SDLK_UNKNOWN,
        SDLK_PERIOD, SDLK_SLASH,                // fire & special
        SDLK_RETURN,                            // switch guys
        SDLK_QUOTE,                             // change special
        SDLK_BACKSLASH,                         // Yell
        SDLK_RSHIFT,                            // Shifter
        SDLK_2,                                 // Options menu
        SDLK_F6,                                // Cheat key
    },
    {
        SDLK_i, SDLK_o, SDLK_l, SDLK_PERIOD,  // movements
        SDLK_COMMA, SDLK_m, SDLK_j, SDLK_u,
        SDLK_SPACE, SDLK_SEMICOLON,             // fire & special
        SDLK_MINUS,                             // switch guys
        SDLK_9,                                 // change special
        SDLK_k,                                 // Yell
        SDLK_0,                                 // Shifter
        SDLK_3,                                 // Options menu
        SDLK_F7,                                // Cheat key
    },
    {
        SDLK_t, SDLK_y, SDLK_h, SDLK_n,  // movements
        SDLK_b, SDLK_v, SDLK_f, SDLK_r,
        SDLK_5, SDLK_6,                         // fire & special
        SDLK_EQUALS,                            // switch guys
        SDLK_7,                                 // change special
        SDLK_g,                                 // Yell
        SDLK_8,                                 // Shifter
        SDLK_4,                                 // Options menu
        SDLK_F8,                                // Cheat key
    }
};



//
// Input routines (for handling all events and then setting the appropriate vars)
//

void init_input()
{
    keystates = SDL_GetKeyState(NULL);

    // Set up joysticks
    for(int i = 0; i < MAX_NUM_JOYSTICKS; i++)
    {
        joysticks[i] = NULL;
    }

    int numjoy;

    numjoy = SDL_NumJoysticks();

    for(int i = 0; i < numjoy; i++)
    {
        joysticks[i] = SDL_JoystickOpen(i);
        if(joysticks[i] == NULL)
            continue;
        player_joy[i] = JoyData(i);
    }

    SDL_JoystickEventState(SDL_ENABLE);
}

void get_input_events(bool type)
{
    SDL_Event event;

    if (type == POLL)
        while (SDL_PollEvent(&event))
            handle_events(&event);
    if (type == WAIT)
    {
        SDL_WaitEvent(&event);
        handle_events(&event);
    }
}

void handle_events(SDL_Event *event)
{
    switch (event->type)
    {
        // Key pressed or released:
    case SDL_KEYDOWN:
        raw_key = event->key.keysym.sym;
        key_press_event = 1;
#ifndef USE_SDL2
        free(raw_text_input);
        raw_text_input = (char*)malloc(sizeof(char)*2);
        raw_text_input[0] = convert_to_ascii(raw_key);
        raw_text_input[1] = '\0';
        text_input_event = 1;
#endif
        break;
    case SDL_KEYUP:
        break;
#ifdef USE_SDL2
    case SDL_TEXTINPUT:
        free(raw_text_input);
        raw_text_input = strdup(event->text.text);
        text_input_event = 1;
        break;
#endif

#ifndef ANDROID
        // Mouse event
    case SDL_MOUSEMOTION:
        //Log("%i %i  -  %i %i\n", event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
        //if (!(event.motion.x < 10 && mouse_state[MOUSE_X] * mult > 620)
        //	&& !(event.motion.y == 0 && mouse_state[MOUSE_Y] > 20))
        mouse_state[MOUSE_X] = event->motion.x / mouse_scale_x;
        //if (!(event.motion.y < 10 && mouse_state[MOUSE_Y] * mult > 460))
        mouse_state[MOUSE_Y] = event->motion.y / mouse_scale_y;
        break;
    case SDL_MOUSEBUTTONUP:
        if (event->button.button == SDL_BUTTON_LEFT)
            mouse_state[MOUSE_LEFT] = 0;
        if (event->button.button == SDL_BUTTON_RIGHT)
            mouse_state[MOUSE_RIGHT] = 0;
        //mouse_state[MOUSE_LEFT] = SDL_BUTTON(SDL_BUTTON_LEFT);
        //Log ("LMB: %d",  SDL_BUTTON(SDL_BUTTON_LEFT));
        //mouse_state[MOUSE_RIGHT] = SDL_BUTTON(SDL_BUTTON_RIGHT);
        //Log ("RMB: %d",  SDL_BUTTON(SDL_BUTTON_RIGHT));
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (event->button.button == SDL_BUTTON_LEFT)
            mouse_state[MOUSE_LEFT] = 1;
        if (event->button.button == SDL_BUTTON_RIGHT)
            mouse_state[MOUSE_RIGHT] = 1;
        break;
#else
        // Mouse event
    case SDL_FINGERMOTION:
        mouse_state[MOUSE_X] = event->tfinger.x * 320;
        mouse_state[MOUSE_Y] = event->tfinger.y * 200;
        break;
    case SDL_FINGERUP:
        if(tapping)
        {
            tapping = false;
            int x = event->tfinger.x * 320;
            int y = event->tfinger.y * 200;
            if(abs(x - start_tap_x) < 2 && abs(y - start_tap_y) < 2)
                input_continue = true;
            else
                input_continue = false;
            start_tap_x = x;
            start_tap_y = y;
        }
        
        mouse_state[MOUSE_LEFT] = 0;
        break;
    case SDL_FINGERDOWN:
        
        tapping = true;
        start_tap_x = event->tfinger.x * 320;
        start_tap_y = event->tfinger.y * 200;
        input_continue = false;
        
        key_press_event = 1;
        mouse_state[MOUSE_LEFT] = 1;
        mouse_state[MOUSE_X] = event->tfinger.x * 320;
        mouse_state[MOUSE_Y] = event->tfinger.y * 200;
        break;
#endif
    case SDL_JOYAXISMOTION:
        if (event->jaxis.value > 8000)
        {
            //key_list[joy_startval[event->jaxis.which] + event->jaxis.axis * 2] = 1;
            //key_list[joy_startval[event->jaxis.which] + event->jaxis.axis * 2 + 1] = 0;
            key_press_event = 1;
            //raw_key = joy_startval[event->jaxis.which] + event->jaxis.axis * 2;
        }
        else if (event->jaxis.value < -8000)
        {
            //key_list[joy_startval[event->jaxis.which] + event->jaxis.axis * 2] = 0;
            //key_list[joy_startval[event->jaxis.which] + event->jaxis.axis * 2 + 1] = 1;
            key_press_event = 1;
            //raw_key = joy_startval[event->jaxis.which] + event->jaxis.axis * 2 + 1;
        }
        else
        {
            //key_list[joy_startval[event->jaxis.which] + event->jaxis.axis * 2] = 0;
            //key_list[joy_startval[event->jaxis.which] + event->jaxis.axis * 2 + 1] = 0;
        }
        break;
    case SDL_JOYBUTTONDOWN:
        //key_list[joy_startval[event->jbutton.which] + joy_numaxes[event->jbutton.which] * 2 + event->jbutton.button] = 1;
        //raw_key = joy_startval[event->jbutton.which] + joy_numaxes[event->jbutton.which] * 2 + event->jbutton.button;
        key_press_event = 1;
        break;
    case SDL_JOYBUTTONUP:
        //key_list[joy_startval[event->jbutton.which] + joy_numaxes[event->jbutton.which] * 2 + event->jbutton.button] = 0;
        break;
    case SDL_QUIT:
        quit(0);
        break;
    default:
        break;
    }
}


//
//Keyboard routines
//

void grab_keyboard()
{}

void release_keyboard()
{}

int query_key()
{
    return raw_key;
}

char* query_text_input()
{
    return raw_text_input;
}

bool query_key_event(int key, const SDL_Event& event)
{
    if(event.type == SDL_KEYDOWN)
        return (event.key.keysym.sym == key);
    return false;
}


bool isAnyPlayerKey(SDLKey key)
{
    for(int player_num = 0; player_num < 4; player_num++)
    {
        for(int i = 0; i < NUM_KEYS; i++)
        {
            if(player_keys[player_num][i] == key)
                return true;
        }
    }
    return false;
}

bool isPlayerKey(int player_num, SDLKey key)
{
    for(int i = 0; i < NUM_KEYS; i++)
    {
        if(player_keys[player_num][i] == key)
            return true;
    }
    return false;
}

SDL_Event wait_for_key_event()
{
    SDL_Event event;
    while(1)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT
                    || event.type == SDL_KEYDOWN
                    || (event.type == SDL_JOYAXISMOTION && (event.jaxis.value > JOY_DEAD_ZONE || event.jaxis.value < -JOY_DEAD_ZONE))
                    || event.type == SDL_JOYBUTTONDOWN
                    || event.type == SDL_JOYHATMOTION
              )
                return event;
        }
        SDL_Delay(10);
    }
    return event;
}

void quit_if_quit_event(const SDL_Event& event)
{
    if(event.type == SDL_QUIT)
        quit(0);
}

bool isKeyboardEvent(const SDL_Event& event)
{
    return (event.type == SDL_KEYDOWN);  // does not handle key up events
}

bool isJoystickEvent(const SDL_Event& event)
{
    return (event.type == SDL_JOYAXISMOTION || event.type == SDL_JOYHATMOTION || event.type == SDL_JOYBUTTONDOWN);  // does not handle button up, hats, or balls
}

void clear_events()
{
    SDL_Event event;
    while(SDL_PollEvent(&event));
}

void assignKeyFromWaitEvent(int player_num, int key_enum)
{
    SDL_Event event;

    event = wait_for_key_event();
    quit_if_quit_event(event);
    if(isKeyboardEvent(event))
    {
        if(event.key.keysym.sym != SDLK_ESCAPE)
        {
            player_keys[player_num][key_enum] = event.key.keysym.sym;
        }
    }
    else if(isJoystickEvent(event))
        player_joy[player_num].setKeyFromEvent(key_enum, event);

    SDL_Delay(400);
    clear_events();
}


//
// Set the keyboard array to all zeros, the
// virgin state, nothing depressed
//
void clear_keyboard()
{
    key_press_event = 0;
    raw_key = 0;

    text_input_event = 0;
    free(raw_text_input);
    raw_text_input = NULL;
    
    #ifdef ANDROID
    input_continue = false;
    tapping = false;
    #endif
}

Uint8* query_keyboard()
{
    return keystates;
}

bool query_input_continue()
{
    #ifndef ANDROID
    return keystates[KEYSTATE_ESCAPE];
    #else
    return input_continue;
    #endif
}

void wait_for_key(int somekey)
{
    // First wait for key press ..
#ifndef USE_SDL2
    while (!keystates[somekey])
#else
    while (!keystates[SDL_GetScancodeFromKey(somekey)])
#endif
        get_input_events(WAIT);

    // And now for the key to be released ..
#ifndef USE_SDL2
    while (!keystates[somekey])
#else
    while (!keystates[SDL_GetScancodeFromKey(somekey)])
#endif
        get_input_events(WAIT);
}

JoyData::JoyData()
    : index(-1), numAxes(0), numButtons(0), numHats(0)
{}

JoyData::JoyData(int index)
    : index(-1), numAxes(0), numButtons(0), numHats(0)
{
    SDL_Joystick *js = joysticks[index];
    if(js == NULL)
        return;

    this->index = index;
    numAxes = SDL_JoystickNumAxes(js);
    numButtons = SDL_JoystickNumButtons(js);
    numHats = SDL_JoystickNumHats(js);

    // Clear all keys for this joystick
    for(int i = 0; i < NUM_KEYS; i++)
    {
        key_type[i] = NONE;
        key_index[i] = 0;
    }

    // Default movement
    if(numAxes > 1) // Prefer two axes
    {
        key_type[KEY_RIGHT] = POS_AXIS;
        key_index[KEY_RIGHT] = 0;
        key_type[KEY_LEFT] = NEG_AXIS;
        key_index[KEY_LEFT] = 0;

        key_type[KEY_UP] = NEG_AXIS;
        key_index[KEY_UP] = 1;
        key_type[KEY_DOWN] = POS_AXIS;
        key_index[KEY_DOWN] = 1;
    }
    else if(numHats > 0) // But a single hat is okay otherwise
    {
        // indices default to hat 0
        key_type[KEY_UP] = HAT_UP;
        key_type[KEY_UP_RIGHT] = HAT_UP_RIGHT;
        key_type[KEY_RIGHT] = HAT_RIGHT;
        key_type[KEY_DOWN_RIGHT] = HAT_DOWN_RIGHT;
        key_type[KEY_DOWN] = HAT_DOWN;
        key_type[KEY_DOWN_LEFT] = HAT_DOWN_LEFT;
        key_type[KEY_LEFT] = HAT_LEFT;
        key_type[KEY_UP_LEFT] = HAT_UP_LEFT;
    }


    // Default actions
    if(numButtons > 0)
    {
        key_type[KEY_FIRE] = BUTTON;
        key_index[KEY_FIRE] = 0;
    }
    if(numButtons > 1)
    {
        key_type[KEY_SPECIAL] = BUTTON;
        key_index[KEY_SPECIAL] = 1;
    }
    if(numButtons > 2)
    {
        key_type[KEY_SPECIAL_SWITCH] = BUTTON;
        key_index[KEY_SPECIAL_SWITCH] = 2;
    }
    if(numButtons > 3)
    {
        key_type[KEY_YELL] = BUTTON;
        key_index[KEY_YELL] = 3;
    }
    if(numButtons > 4)
    {
        key_type[KEY_SHIFTER] = BUTTON;
        key_index[KEY_SHIFTER] = 4;
    }
    if(numButtons > 5)
    {
        key_type[KEY_SWITCH] = BUTTON;
        key_index[KEY_SWITCH] = 5;
    }
}


void JoyData::setKeyFromEvent(int key_enum, const SDL_Event& event)
{
    // Diagonals are ignored because they are combinations of the cardinals
    // Things get really messy when diagonals are assigned
    if(key_enum == KEY_UP_RIGHT || key_enum == KEY_UP_LEFT || key_enum == KEY_DOWN_RIGHT || key_enum == KEY_DOWN_LEFT)
    {
        key_type[key_enum] = NONE;
        key_index[key_enum] = 0;
        return;
    }

    bool gotJoy = false;
    if(event.type == SDL_JOYAXISMOTION)
    {
        if(event.jaxis.value >= 0)
            key_type[key_enum] = POS_AXIS;
        else
            key_type[key_enum] = NEG_AXIS;
        key_index[key_enum] = event.jaxis.axis;
        index = event.jaxis.which;  // USES THE LAST JOYSTICK PRESSED
        gotJoy = true;
    }
    else if(event.type == SDL_JOYBUTTONDOWN)
    {
        key_type[key_enum] = BUTTON;
        key_index[key_enum] = event.jbutton.button;
        index = event.jbutton.which;  // USES THE LAST JOYSTICK PRESSED
        gotJoy = true;
    }
    else if(event.type == SDL_JOYHATMOTION)
    {
        bool badHat = false;
        if(event.jhat.value == SDL_HAT_UP)
            key_type[key_enum] = HAT_UP;
        else if(event.jhat.value == SDL_HAT_RIGHT)
            key_type[key_enum] = HAT_RIGHT;
        else if(event.jhat.value == SDL_HAT_DOWN)
            key_type[key_enum] = HAT_DOWN;
        else if(event.jhat.value == SDL_HAT_LEFT)
            key_type[key_enum] = HAT_LEFT;
        else
        {
            badHat = true;
            // Diagonals are ignored because they are combinations of the cardinals
            /*else if(event.jhat.value == SDL_HAT_RIGHTUP)
                key_type[key_enum] = HAT_UP_RIGHT;
            else if(event.jhat.value == SDL_HAT_RIGHTDOWN)
                key_type[key_enum] = HAT_DOWN_RIGHT;
            else if(event.jhat.value == SDL_HAT_LEFTDOWN)
                key_type[key_enum] = HAT_DOWN_LEFT;
            else if(event.jhat.value == SDL_HAT_LEFTUP)
                key_type[key_enum] = HAT_UP_LEFT;*/
        }
        if(!badHat)
        {
            key_index[key_enum] = event.jhat.hat;
            index = event.jhat.which;  // USES THE LAST JOYSTICK PRESSED
            gotJoy = true;
        }
    }

    if(gotJoy)
    {
        // Take over this joystick
        for(int i = 0; i < 4; i++)
        {
            if(this != &player_joy[i] && player_joy[i].index == index)
                player_joy[i].index = -1;
        }
    }
}

bool JoyData::getState(int key_enum) const
{
    if(index < 0)
        return false;
    switch(key_type[key_enum])
    {
    case POS_AXIS:
        return SDL_JoystickGetAxis(joysticks[index], key_index[key_enum]) > JOY_DEAD_ZONE;
    case NEG_AXIS:
        return SDL_JoystickGetAxis(joysticks[index], key_index[key_enum]) < -JOY_DEAD_ZONE;
    case BUTTON:
        return SDL_JoystickGetButton(joysticks[index], key_index[key_enum]);
    case HAT_UP:
        return (SDL_JoystickGetHat(joysticks[index], key_index[key_enum]) & SDL_HAT_UP);
    case HAT_RIGHT:
        return (SDL_JoystickGetHat(joysticks[index], key_index[key_enum]) & SDL_HAT_RIGHT);
    case HAT_DOWN:
        return (SDL_JoystickGetHat(joysticks[index], key_index[key_enum]) & SDL_HAT_DOWN);
    case HAT_LEFT:
        return (SDL_JoystickGetHat(joysticks[index], key_index[key_enum]) & SDL_HAT_LEFT);
        // Diagonals are ignored because they are combinations of the cardinals
    case HAT_UP_RIGHT:
    case HAT_DOWN_RIGHT:
    case HAT_DOWN_LEFT:
    case HAT_UP_LEFT:
    default:
        return false;
    }
}

bool JoyData::getPress(int key_enum, const SDL_Event& event) const
{
    if(index < 0)
        return false;

    switch(key_type[key_enum])
    {
    case BUTTON:
        if(event.type == SDL_JOYBUTTONDOWN)
        {
            return (event.jbutton.which == index && event.jbutton.button == key_index[key_enum]);
        }
        return false;
    case POS_AXIS:
        if(event.type == SDL_JOYAXISMOTION)
        {
            return (event.jaxis.which == index && event.jaxis.axis == key_index[key_enum] && event.jaxis.value > JOY_DEAD_ZONE);
        }
        return false;
    case NEG_AXIS:
        if(event.type == SDL_JOYAXISMOTION)
        {
            return (event.jaxis.which == index && event.jaxis.axis == key_index[key_enum] && event.jaxis.value < -JOY_DEAD_ZONE);
        }
        return false;
    case HAT_UP:
        return (event.jhat.which == index && event.jhat.hat == key_index[key_enum] && event.jhat.value & SDL_HAT_UP);
    case HAT_RIGHT:
        return (event.jhat.which == index && event.jhat.hat == key_index[key_enum] && event.jhat.value & SDL_HAT_RIGHT);
    case HAT_DOWN:
        return (event.jhat.which == index && event.jhat.hat == key_index[key_enum] && event.jhat.value & SDL_HAT_DOWN);
    case HAT_LEFT:
        return (event.jhat.which == index && event.jhat.hat == key_index[key_enum] && event.jhat.value & SDL_HAT_LEFT);

        // Diagonals are ignored because they are combinations of the cardinals
    case HAT_UP_RIGHT:
    case HAT_DOWN_RIGHT:
    case HAT_DOWN_LEFT:
    case HAT_UP_LEFT:
    default:
        return false;
    }
}

bool JoyData::hasButtonSet(int key_enum) const
{
    return (index >= 0 && key_type[key_enum] != NONE);
}

bool playerHasJoystick(int player_num)
{
    return (player_joy[player_num].index >= 0);
}

void disablePlayerJoystick(int player_num)
{
    player_joy[player_num].index = -1;
}

void resetJoystick(int player_num)
{
    // Reset joystick subsystem
    if(SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK)
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    // Set up joysticks
    for(int i = 0; i < MAX_NUM_JOYSTICKS; i++)
    {
        joysticks[i] = NULL;
    }

    int numjoy = SDL_NumJoysticks();
    for(int i = 0; i < numjoy; i++)
    {
        joysticks[i] = SDL_JoystickOpen(i);
        if(joysticks[i] == NULL)
            continue;
        // The joystick indices might change here.
        // FIXME: There's a chance that players will not have the joysticks they expect and
        // so they might have buttons, etc. that are out of range for the new joystick.
    }

    SDL_JoystickEventState(SDL_ENABLE);

    player_joy[player_num] = JoyData(player_num);
}

bool isPlayerHoldingKey(int player_index, int key_enum)
{
    if(player_joy[player_index].hasButtonSet(key_enum))
    {
        return player_joy[player_index].getState(key_enum);
    }
    else
    {
#ifndef USE_SDL2
        return keystates[player_keys[player_index][key_enum]];
#else
        return keystates[SDL_GetScancodeFromKey(player_keys[player_index][key_enum])];
#endif
    }
}

bool didPlayerPressKey(int player_index, int key_enum, const SDL_Event& event)
{
    if(player_joy[player_index].hasButtonSet(key_enum))
    {
        // This key is on the joystick, so check it.
        return player_joy[player_index].getPress(key_enum, event);
    }
    else
    {
        // If the player is using KEYBOARD or doesn't have a joystick button set for this key, then check the keyboard.
        if(event.type == SDL_KEYDOWN)
        {
            return (event.key.keysym.sym == player_keys[player_index][key_enum]);
        }
        return false;
    }
}

short query_key_press_event()
{
    return key_press_event;
}

void clear_key_press_event()
{
    key_press_event = 0;
}

short query_text_input_event()
{
    return text_input_event;
}

void clear_text_input_event()
{
    text_input_event = 0;
    free(raw_text_input);
    raw_text_input = NULL;
}

void enable_keyrepeat()
{
#ifndef USE_SDL2
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
#endif
}

void disable_keyrepeat()
{
#ifndef USE_SDL2
    SDL_EnableKeyRepeat(0,0);
#endif
}



//
// Mouse routines
//

void grab_mouse()
{
    SDL_ShowCursor(SDL_ENABLE);
}

void release_mouse()
{
    SDL_ShowCursor(SDL_DISABLE);
}

Sint32 * query_mouse()
{
    // The mouse_state thing is set using get_input_events, though
    // it should probably get its own function
    get_input_events(POLL);
    return mouse_state;
}


// Convert from scancode to ascii, ie, SDLK_a to 'A'
unsigned char convert_to_ascii(int scancode)
{
    switch (scancode)
    {
    case SDLK_a:
        return 'A';
    case SDLK_b:
        return 'B';
    case SDLK_c:
        return 'C';
    case SDLK_d:
        return 'D';
    case SDLK_e:
        return 'E';
    case SDLK_f:
        return 'F';
    case SDLK_g:
        return 'G';
    case SDLK_h:
        return 'H';
    case SDLK_i:
        return 'I';
    case SDLK_j:
        return 'J';
    case SDLK_k:
        return 'K';
    case SDLK_l:
        return 'L';
    case SDLK_m:
        return 'M';
    case SDLK_n:
        return 'N';
    case SDLK_o:
        return 'O';
    case SDLK_p:
        return 'P';
    case SDLK_q:
        return 'Q';
    case SDLK_r:
        return 'R';
    case SDLK_s:
        return 'S';
    case SDLK_t:
        return 'T';
    case SDLK_u:
        return 'U';
    case SDLK_v:
        return 'V';
    case SDLK_w:
        return 'W';
    case SDLK_x:
        return 'X';
    case SDLK_y:
        return 'Y';
    case SDLK_z:
        return 'Z';

    case SDLK_1:
        return '1';
    case SDLK_2:
        return '2';
    case SDLK_3:
        return '3';
    case SDLK_4:
        return '4';
    case SDLK_5:
        return '5';
    case SDLK_6:
        return '6';
    case SDLK_7:
        return '7';
    case SDLK_8:
        return '8';
    case SDLK_9:
        return '9';
    case SDLK_0:
        return '0';

    case SDLK_SPACE:
        return 32;
        //    case SDLK_BACKSPACE: return 8;
    case SDLK_RETURN:
        return 13;
    case SDLK_ESCAPE:
        return 27;
    case SDLK_PERIOD:
        return '.';
    case SDLK_COMMA:
        return ',';
    case SDLK_QUOTE:
        return '\'';
    case SDLK_BACKQUOTE:
        return '`';

    default:
        return 255;
    }
}
