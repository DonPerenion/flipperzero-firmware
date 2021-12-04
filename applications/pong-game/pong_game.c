#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
  
const width = 64;
const height = 128;
const maxSpeed = 4; 
typedef struct {
    uint8_t x;
    uint8_t y;
    int8_t vx;
    int8_t vy;
    uint8_t size;
} Point;

typedef enum {
    GameStateLife,
    GameStateLastChance,
    GameStateGameOver,
} GameState;

typedef enum {
    DirectionUp,
    DirectionDown,
    Stop
} Direction;

typedef struct {
   // Direction currentMovement;
    Direction nextMovement; // if backward of currentMovement, ignore
    Point ball;
    Point pad_user;
    Point pad_comp;
    GameState state;
    uint8_t score;
} PongState; 

typedef enum {  //?????????????????
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct { //????????????????????
    EventType type;
    InputEvent input;
} SnakeEvent;

static void snake_game_render_callback(Canvas* const canvas, void* ctx) {
    const PongState* PongState = acquire_mutex((ValueMutex*)ctx, 25);
    if (PongState == NULL) {
        return;
    }

    // Before the function is called, the state is set with the canvas_reset(canvas)

    // Frame
    canvas_draw_frame(canvas, 0, 0, 128, 64);

    // Ball
    Point f = PongState->ball;
    f.x = f.x * 4 + 1;
    f.y = f.y * 4 + 1;
    canvas_draw_rframe(canvas, f.x, f.y, 64, 32, 2); // рисует кружок

    // 2 pad
    for (uint16_t i = 0; i < 16; i++) {
        Point p = PongState->pad_user;
        p.x = p.x * 4 + 2;
        p.y = p.y * 4 + 2;
        canvas_draw_box(canvas, p.x, p.y, 4, 4);
        Point p = PongState->pad_comp;
        p.x = p.x * 4 + 2;
        p.y = p.y * 4 + 2;
        canvas_draw_box(canvas, p.x, p.y, 4, 4);
    }

    // Game Over banner
    if (PongState->state == GameStateGameOver) {
        // Screen is 128x64 px
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 34, 20, 62, 24);

        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, 34, 20, 62, 24);

        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 37, 31, "Game Over");

        canvas_set_font(canvas, FontSecondary);
        char buffer[12];
        snprintf(buffer, sizeof(buffer), "Score: %u", PongState->score);
        canvas_draw_str_aligned(canvas, 64, 41, AlignCenter, AlignBottom, buffer);
    }

    release_mutex((ValueMutex*)ctx, snake_state);
}

static void snake_game_input_callback(InputEvent* input_event, osMessageQueueId_t event_queue) {
    furi_assert(event_queue);
    SnakeEvent event = { .type = EventTypeKey, .input = *input_event };
    osMessageQueuePut(event_queue, &event, 0, osWaitForever);
}

static void snake_game_update_timer_callback(osMessageQueueId_t event_queue) {
    furi_assert(event_queue);
    SnakeEvent event = { .type = EventTypeTick };
    osMessageQueuePut(event_queue, &event, 0, 0);
}

static void snake_game_init_game(PongState* const PongState) {

   // PongState->currentMovement = Stop;

    PongState->nextMovement = Stop;

    Point f = { 64, 32, -1, 0, 0 }; // x , y , vx , vy, size
    PongState->ball = f;
    Point f = { 4 , 32, 0 , 0 , 8 };
    PongState->pad_comp = f;
    Point f = { 124 , 32, 0 , 0, 8 };
    PongState->pad_user = f;
    PongState->state = GameStateLife;
}

//static Point snake_game_get_new_fruit(PongState const* const PongState) {
//    // 1 bit for each point on the playing field where the snake can turn
//    // and where the fruit can appear
//    uint16_t buffer[8];
//    memset(buffer, 0, sizeof(buffer));
//    uint8_t empty = 8 * 16;
//
//    for (uint16_t i = 0; i < PongState->len; i++) {
//        Point p = PongState->points[i];
//
//        if (p.x % 2 != 0 || p.y % 2 != 0) {
//            continue;
//        }
//        p.x /= 2;
//        p.y /= 2;
//
//        buffer[p.y] |= 1 << p.x;
//        empty--;
//    }
//    // Bit set if snake use that playing field
//
//    uint16_t newFruit = rand() % empty;
//
//    // Skip random number of _empty_ fields
//    for (uint8_t y = 0; y < 8; y++) {
//        for (uint16_t x = 0, mask = 1; x < 16; x += 1, mask <<= 1) {
//            if ((buffer[y] & mask) == 0) {
//                if (newFruit == 0) {
//                    Point p = {
//                        .x = x * 2,
//                        .y = y * 2,
//                    };
//                    return p;
//                }
//                newFruit--;
//            }
//        }
//    }
//    // We will never be here
//    Point p = { 0, 0 };
//    return p;
//}

static Direction snake_game_get_turn_snake(PongState const* const PongState) {
    switch (PongState->nextMovement) 
    {
        case DirectionUp:
            return DirectionUp;
        case DirectionDown:
            return DirectionDown;
    }
}

static Point snake_game_get_next_step(PongState const* const PongState) {
    Point next_step = PongState->pad_user;
    switch (PongState->nextMovement) {
        // y
        // |
        // |
        // +-------------------------x
    case DirectionUp:
        next_step.y++;
        break;
    case DirectionDown:
        next_step.y--;
        break;
    }
    return next_step;
}

static void snake_game_move_snake(PongState* const PongState, Point const next_step) {
    PongState->pad_user = next_step;
}

static void snake_game_process_game_step(PongState* const PongState) {
    if (PongState->state == GameStateGameOver) {
        return;
    }

    bool can_turn = (PongState->pad_user.y >= 0) && (PongState->pad_user.y <= 128); // ????????????????????????
    if (can_turn) {
        PongState->nextMovement = snake_game_get_turn_snake(PongState);
    }

    Point next_step = snake_game_get_next_step(snake_state);

    // Move comp.x towards the ball !!!(AI)!!!
    PongState->pad_comp.vx -= PongState->pad_comp.x - PongState->ball.x / 10.0;
    PongState->pad_comp.vx = uint16_t(PongState->pad_comp.vx * 0.83); //0.83

    // Apply comp's velocity to current location
    PongState->pad_comp.x += PongState->pad_comp.vx;

    // Add player boundaries (проверка на границы)
    if (PongState->pad_user.y < PongState->pad_user.size + 1) {
        PongState->pad_user.y = PongState->pad_user.size + 1;
    }
    else if (PongState->pad_user.y > width - PongState->pad_user.size - 2) {
        PongState->pad_user.y = width - PongState->pad_user.size - 2;
    }

    // Add comp boundaries (проверка на границы)
    if (PongState->pad_comp.y < PongState->pad_comp.size + 1) {
        PongState->pad_comp.y = PongState->pad_comp.size + 1;
    }
    else if (PongState->pad_comp.y > width - PongState->pad_comp.size - 2) {
        PongState->pad_comp.y = width - PongState->pad_comp.size - 2;
    }

    // Make sure ball doesn't exceed it's maximum speed
    if (PongState->ball.vy < -maxSpeed) {
        PongState->ball.vy = -maxSpeed;
    }
    else if (PongState->ball.vy > maxSpeed) {
        PongState->ball.vy = maxSpeed;
    }

    // Apply velocity to balls current location
    PongState->ball.x += PongState->ball.vx;
    PongState->ball.y += PongState->ball.vy;

    // Check if ball is hitting your pad
    if (PongState->ball.x >= PongState->pad_user.x - 1 && PongState->ball.x <= PongState->pad_user.x + 1) {
        if (PongState->ball.y > PongState->pad_user.y - PongState->pad_user.size && PongState->ball.y < PongState->pad_user.y + PongState->pad_user.size) { // если ударились
            PongState->ball.vx *= -1; // разворачиваем мячик
            PongState->ball.vy += (PongState->ball.y - PongState->pad_user.y) / 3; // смотря в какой части платформы ударились, меняем скорость мяча по x
            PongState->ball.x = PongState->pad_user.x - 1; 
            PongState->score++;
        }
    }

    // Check if ball is hitting the computer pad
    if (PongState->ball.x <= PongState->pad_comp.x + 1 && PongState->ball.x >= PongState->pad_comp.x - 1) {
        if (PongState->ball.y > PongState->pad_comp.y - PongState->pad_comp.size && PongState->ball.y < PongState->pad_comp.y + PongState->pad_comp.size) {
            PongState->ball.vx *= -1;
            PongState->ball.vy += (PongState->ball.y - PongState->pad_comp.y) / 3;
            PongState->ball.x = PongState->pad_comp.x + 1;
        }
    }

    // Check if ball has gone out of boundaries (a player won/lost)
    if (PongState->ball.x >= height) {
        //++comp_score;                                                                                                         // SCORES ????????????????????????
        reset(PongState->pad_user, PongState->pad_comp, PongState->ball, started, PongState->pad_user, comp_score);
    }
    else if (PongState->ball.x <= 0) {
        //++PongState->pad_user;
        reset(PongState->pad_user, PongState->pad_comp, PongState->ball, started, PongState->pad_user, comp_score);
    }
    // Что с мячиком после вылета
    if (PongState->ball.y <= 1) {
        PongState->ball.vy *= -1;
        PongState->ball.y = 1.0;
    }
    else if (PongState->ball.y >= width - 3) {
        PongState->ball.vy *= -1;
        PongState->ball.y = width - 3;
    }

    snake_game_move_snake(snake_state, next_step);
}

int32_t pong_game_app(void* p) {
    srand(DWT->CYCCNT);

    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(SnakeEvent), NULL);

    PongState* PongState = furi_alloc(sizeof(PongState));
    snake_game_init_game(PongState);

    ValueMutex state_mutex;
    if (!init_mutex(&state_mutex, PongState, sizeof(PongState))) {
        furi_log_print(FURI_LOG_ERROR, "cannot create mutex\r\n");
        free(PongState);
        return 255;
    }

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, snake_game_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, snake_game_input_callback, event_queue);

    osTimerId_t timer = osTimerNew(snake_game_update_timer_callback, osTimerPeriodic, event_queue, NULL);
    osTimerStart(timer, osKernelGetTickFreq() / 4);

    // Open GUI and register view_port
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    SnakeEvent event;
    for (bool processing = true; processing;) {
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);

        SnakeState* PongState = (PongState*)acquire_mutex_block(&state_mutex);

        if (event_status == osOK) {
            // press events
            if (event.type == EventTypeKey) {
                if (event.input.type == InputTypePress) {
                    switch (event.input.key) {
                    case InputKeyUp:
                        PongState->nextMovement = DirectionUp;
                        break;
                    case InputKeyDown:
                        PongState->nextMovement = DirectionDown;
                        break;
                    case InputKeyOk:
                        if (PongState->state == GameStateGameOver) {
                            snake_game_init_game(PongState);
                        }
                        break;
                    case InputKeyBack:
                        processing = false;
                        break;
                    }
                }
            }
            else if (event.type == EventTypeTick) {
                snake_game_process_game_step(PongState);
            }
        }
        else {
            // event timeout
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, PongState);
    }

    osTimerDelete(timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    osMessageQueueDelete(event_queue);
    delete_mutex(&state_mutex);
    free(PongState);

    return 0;
}
