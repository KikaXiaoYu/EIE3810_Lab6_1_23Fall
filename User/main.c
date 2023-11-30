#include "stm32f10x.h"
#include "EIE3810_Buzzer.h"
#include "EIE3810_Clock.h"
#include "EIE3810_JOYPAD.h"
#include "EIE3810_Key.h"
#include "EIE3810_LED.h"
#include "EIE3810_Others.h"
#include "EIE3810_TFTLCD.h"
#include "EIE3810_Timer.h"
#include "EIE3810_USART.h"

#define ORG_FRE 72000000                                          // orginial frequency, will be divided
#define FIX_ARR 5000                                              // fixed arr for usage
#define TIM3_FRE 4                                                // frequency for TIM3
#define TIM4_FRE 16                                               // frequency for TIM4
#define SCREEN_WIDTH 480                                          // screen width
#define SCREEN_HEIGHT 800                                         // screen height
#define LOG_HEIGHT 20                                             // the log height
#define LOG_WIDTH 120                                             // the log width
#define EDGE_SIZE 30                                              // top and bottom edges, for showing information
#define PLAYER_A_Y (SCREEN_HEIGHT - (LOG_HEIGHT / 2 + EDGE_SIZE)) // log A's Y
#define PLAYER_B_Y (LOG_HEIGHT / 2 + EDGE_SIZE)                   // log B's Y
#define BALL_RADIUS 20                                            // ball's radius

enum game_state // indicating the game state
{
    RESET = -1,     // not started showing the screen
    INTRO = 0,      // introduce the game
    DIFF_LEVEL = 1, // select the difficulty level
    USART = 2,      // send the USART
    COUNT_DOWN = 3, // count down page
    PLAYING = 4,    // playing
    PAUSE = 5,      // pause the game
    GAME_OVER = 6   // game over
} g_game_state = RESET;

enum game_mode // indicating the game difficulty level
{
    EASY = 0,
    HARD = 1
} g_game_mode = EASY;

enum game_turn // indicating which player kicks the ball
{
    A = 0,
    B = 1
} g_game_turn = A;

u32 g_TIM3_local_beat = 0; // to count the time

u32 g_e_time = 0;   // elapsed time
u32 g_bounces = 0;  // bounces count
u32 g_ball_dir = 4; // ball direction (for B reference )
u32 dirs[8][2] = {
    {9, 2},
    {8, 5},
    {5, 8},
    {2, 9},
    {-2, 9},
    {-5, 8},
    {-8, 5},
    {-9, 2}};

/* indices when playing */
u16 g_Player_A_X = SCREEN_WIDTH / 2;
u16 g_Player_B_X = SCREEN_WIDTH / 2;
u16 g_ball_X = SCREEN_WIDTH / 2;
u16 g_ball_Y = EDGE_SIZE + LOG_HEIGHT + BALL_RADIUS;

/* indices record */
u16 g_Player_A_X_pre = SCREEN_WIDTH / 2;
u16 g_Player_B_X_pre = SCREEN_WIDTH / 2;
u16 g_ball_X_pre = SCREEN_WIDTH / 2;
u16 g_ball_Y_pre = EDGE_SIZE + LOG_HEIGHT + BALL_RADIUS;

/* further option */
u32 g_Player_A_speed = 1;
u32 g_Player_B_speed = 1;
u32 g_ball_speed = 1;

/* main function */
int main(void)
{
    /* initialization */
    EIE3810_clock_tree_init();                                          // clock tree
    EIE3810_Buzzer_Init();                                              // buzzer
    EIE3810_FourKeys_EXTIInit();                                        // four keys interrupt
    EIE3810_TIM3_Init(FIX_ARR - 1, ORG_FRE / (TIM3_FRE * FIX_ARR) - 1); // TIM3
    EIE3810_TIM4_Init(FIX_ARR - 1, ORG_FRE / (TIM4_FRE * FIX_ARR) - 1); // TIM4
    EIE3810_USART1_init(72, 14400);                                     // USART1
    EIE3810_USART1_EXTIInit();                                          // USART1 interrupt
    EIE3810_TFTLCD_Init();                                              // TFTLCD
    EIE3810_JOYPAD_Init();                                              // JOYPAD
    EIE3810_NVIC_SetPriorityGroup(5);                                   // priority group
    Delay(1000000);                                                     // 1s

    g_game_state = INTRO;         // introduction state
    game_state_INTRO_init();      // init introduction screen
    while (g_game_state == INTRO) // wait until not intro
        ;
    game_state_DIFF_LEVEL_init();      // select difficulty level
    while (g_game_state == DIFF_LEVEL) // wait unit not level
        ;
    game_state_USART_init();      // send USART
    while (g_game_state == USART) // wait until not USART
        ;
    game_state_COUNT_DOWN();           // count down
    while (g_game_state == COUNT_DOWN) // wait until count down finish
        ;
    game_state_PLAYING_init(); // init playing

    while (g_game_state != GAME_OVER) // wait until game over
        ;
    game_state_GAME_OVER_init(); // init game over screen
    while (1)
        ;
}

/* TIM3 interrupt handler
 * to count the time, move the ball, and update the screen
 */
void TIM3_IRQHandler(void)
{
    if (TIM3->SR & 1 << 0)
    {
        if (g_game_state == PLAYING) // when playing
        {
            /* count the elapsed time */
            g_TIM3_local_beat++;
            if (g_TIM3_local_beat == TIM3_FRE)
            {
                g_TIM3_local_beat = 0;
                g_e_time++;
            }
            /* move the ball */
            Ball_Move();
            if (g_game_state != GAME_OVER)
            {
                /* update the screen */
                PLAYING_update();
                /* record the previous indices */
                u16 g_Player_A_X_pre = g_Player_A_X;
                u16 g_Player_B_X_pre = g_Player_B_X;
                u16 g_ball_X_pre = g_ball_X;
                u16 g_ball_Y_pre = g_ball_Y;
            }
        }
    }
    TIM3->SR &= ~(1 << 0);
}

/* move the ball, do the collision and judge the game state */
void Ball_Move(void)
{
    u32 go_x = dirs[g_ball_dir][0];
    u32 go_y = dirs[g_ball_dir][1];
    if (g_game_turn == A)
        go_y = -go_y; // if A, then reverse the vertical direction
    g_ball_X += go_x * g_ball_speed;
    g_ball_Y += go_y * g_ball_speed;

    if (g_ball_Y < BALL_RADIUS + EDGE_SIZE + LOG_HEIGHT)
    {
        if ((g_ball_X - g_Player_B_X < LOG_WIDTH / 2) || (g_ball_X - g_Player_B_X > -LOG_WIDTH / 2))
        {
            g_ball_Y = BALL_RADIUS + EDGE_SIZE + LOG_HEIGHT;
            g_game_turn = B;
        }
        else
        {
            g_game_state = GAME_OVER;
        }
    }
    else if (g_ball_Y > SCREEN_HEIGHT - (BALL_RADIUS + EDGE_SIZE + LOG_HEIGHT))
    {
        if ((g_ball_X - g_Player_A_X < LOG_WIDTH / 2) || (g_ball_X - g_Player_A_X > -LOG_WIDTH / 2))
        {
            g_ball_Y = SCREEN_HEIGHT - (BALL_RADIUS + EDGE_SIZE + LOG_HEIGHT);
            g_game_turn = A;
        }
        else
        {
            g_game_state = GAME_OVER;
        }
    }
    else if (g_ball_X < BALL_RADIUS)
    {
        g_ball_X = BALL_RADIUS;
        g_ball_dir = 7 - g_ball_dir;
    }
    else if (g_ball_X > SCREEN_WIDTH - BALL_RADIUS)
    {
        g_ball_X = SCREEN_WIDTH - BALL_RADIUS;
        g_ball_dir = 7 - g_ball_dir;
    }
    else
    {
        return;
    }
    EIE3810_Toggle_Buzzer();
    Delay(100000);
    EIE3810_Toggle_Buzzer();
}

/* JOYPAD control */
void TIM4_IRQHandler(void)
{
    if (TIM4->SR & 1 << 0) // check if the capture/compare 1 interrupt flag is set
    {
        if (g_game_state == PLAYING)
        {
            int idx = EIE3810_JOYPAD_GetIndex();
            switch (idx)
            {
            case 3: // START
                g_game_state = PAUSE;
                Delay(500000);
                break;
            case 6: // left
                if (g_Player_B_X > LOG_WIDTH / 2)
                {
                    g_Player_B_X -= g_Player_A_speed;
                }
                else
                {
                    g_Player_B_X = LOG_WIDTH / 2;
                }
            case 7: // right
                if (g_Player_B_X < SCREEN_WIDTH - LOG_WIDTH / 2)
                {
                    g_Player_B_X += g_Player_B_speed;
                }
                else
                {
                    g_Player_B_X = SCREEN_WIDTH - LOG_WIDTH / 2;
                }
            default:
                break;
            }
        }
        else if (g_game_state == PAUSE)
        {
            int idx = EIE3810_JOYPAD_GetIndex();
            switch (idx)
            {
            case 3: // START
                g_game_state = PLAYING;
                Delay(500000);
                break;

            default:
                break;
            }
        }
    }
    TIM4->SR &= ~(1 << 0); // clear capture/compare 1 interrupt flag
}

/* init introduction screen */
void game_state_INTRO_init(void)
{
    EIE3810_TFTLCD_Clear(WHITE);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 - 80, "Welcome to mini Project!", WHITE, BLUE, 2);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 40, "This is the Final Lab.", WHITE, RED, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 0, "Are you ready?", WHITE, RED, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - (-40), "OK! Let's start.", WHITE, RED, 1);
}

/* init difficulty level screen */
void game_state_DIFF_LEVEL_init(void)
{
    EIE3810_TFTLCD_Clear(WHITE);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 80, "Please select the difficulty level:", WHITE, RED, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 40, "Easy", WHITE, BLUE, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 0, "Hard", BLUE, WHITE, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - (-40), "Press KEY0 to enter.", WHITE, RED, 1);
}

/* init USART sending screen */
void game_state_USART_init(void)
{
    EIE3810_TFTLCD_Clear(WHITE);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, "Use USART for a random direction.", WHITE, RED, 2);
}

/* starting counting down */
void game_state_COUNT_DOWN(void)
{
    EIE3810_TFTLCD_Clear(WHITE);
    for (int i = 3; i == -1; i--)
    {
        // EIE3810_TFTLCD_DrawRectangle(SCREEN_WIDTH / 2 - 40, 75, SCREEN_HEIGHT / 2 - 70, 140, WHITE);
        EIE3810_TFTLCD_SevenSegment(SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2 - 70, i, RED, WHITE);
        Delay(1000000);
    }
    g_game_state = PLAYING;
}

/* init playing screen */
void game_state_PLAYING_init(void)
{
    g_Player_A_X = SCREEN_WIDTH / 2;
    g_Player_B_X = SCREEN_WIDTH / 2;
    g_ball_X = SCREEN_WIDTH / 2;
    g_ball_Y = EDGE_SIZE + LOG_HEIGHT + BALL_RADIUS;
    g_e_time = 0;
    g_bounces = 0;
    EIE3810_TFTLCD_Clear(WHITE);
    EIE3810_TFTLCD_DrawRectangle(0, SCREEN_WIDTH, PLAYER_A_Y + LOG_HEIGHT / 2, 10, BLACK);
    EIE3810_TFTLCD_DrawRectangle(0, SCREEN_WIDTH, PLAYER_B_Y - LOG_HEIGHT / 2 - 10, 10, BLACK);
    PLAYING_update();
}

/* show game over screen */
void game_state_GAME_OVER_init(void)
{
    if (g_game_turn == A)
    {
        EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 80, "Player B loses the game!", RED, WHITE, 2);
    }
    else if (g_game_turn == B)
    {
        EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 80, "Player A loses the game!", RED, WHITE, 2);
    }
}

/* draw the elapsed time */
void draw_e_time(u16 color)
{
    int minutes = g_e_time / 60;
    int seconds = g_e_time % 60;
    int min_1st = minutes / 10;
    int min_2nd = minutes % 10;
    int sec_1st = seconds / 10;
    int sec_2nd = seconds % 10;
    EIE3810_TFTLCD_ShowString(5, 5, "Elapsed Time: ", color, WHITE, 1);
    EIE3810_TFTLCD_ShowChar1608(5 + 15 * 8, 5, '0' + min_1st, color, WHITE);
    EIE3810_TFTLCD_ShowChar1608(5 + 16 * 8, 5, '0' + min_2nd, color, WHITE);
    EIE3810_TFTLCD_ShowString(5 + 17 * 8, 5, "m", color, WHITE, 1);
    EIE3810_TFTLCD_ShowChar1608(5 + 18 * 8, 5, '0' + sec_1st, color, WHITE);
    EIE3810_TFTLCD_ShowChar1608(5 + 19 * 8, 5, '0' + sec_2nd, color, WHITE);
}

/* draw the bounces time */
void draw_bounces(u16 color)
{
    int boun_1st = g_bounces / 10;
    int boun_2nd = g_bounces % 10;
    EIE3810_TFTLCD_ShowString(5 + 24 * 8, 5, "Bounces: ", color, WHITE, 1);
    EIE3810_TFTLCD_ShowChar1608(5 + 25 * 8, 5, '0' + boun_1st, color, WHITE);
    EIE3810_TFTLCD_ShowChar1608(5 + 26 * 8, 5, '0' + boun_2nd, color, WHITE);
}

/* update the screen when playing */
void PLAYING_update(void)
{
    /* local clear */
    EIE3810_TFTLCD_DrawRectangle(g_Player_A_X_pre - LOG_WIDTH / 2, LOG_WIDTH, PLAYER_A_Y - LOG_HEIGHT / 2, LOG_HEIGHT, WHITE);
    EIE3810_TFTLCD_DrawRectangle(g_Player_B_X_pre - LOG_WIDTH / 2, LOG_WIDTH, PLAYER_B_Y - LOG_HEIGHT / 2, LOG_HEIGHT, WHITE);
    EIE3810_TFTLCD_DrawCircle(g_ball_X_pre, g_ball_Y_pre, BALL_RADIUS, 1, WHITE);
    /* local update */
    EIE3810_TFTLCD_DrawRectangle(g_Player_A_X - LOG_WIDTH / 2, LOG_WIDTH, PLAYER_A_Y - LOG_HEIGHT / 2, LOG_HEIGHT, BLACK);
    EIE3810_TFTLCD_DrawRectangle(g_Player_B_X - LOG_WIDTH / 2, LOG_WIDTH, PLAYER_B_Y - LOG_HEIGHT / 2, LOG_HEIGHT, BLACK);
    EIE3810_TFTLCD_DrawCircle(g_ball_X, g_ball_Y, BALL_RADIUS, 1, RED);
    /* info update */
    draw_e_time(BLACK);
    draw_bounces(BLACK);
}

/* KEY0, step the state and move right */
void EXTI4_IRQHandler(void)
{
    if (g_game_state == INTRO)
    {
        g_game_state = DIFF_LEVEL;
    }
    if (g_game_state == DIFF_LEVEL)
    {
        g_game_state = USART;
    }
    if (g_game_state == PLAYING)
    {
        if (g_Player_A_X < SCREEN_WIDTH - LOG_WIDTH / 2)
        {
            g_Player_A_X += g_Player_B_speed;;
        }
        else
        {
            g_Player_A_X = SCREEN_WIDTH - LOG_WIDTH / 2;
        }
    }

    EXTI->PR = 1 << 4; // Clear the pending status of EXTI4
}

/* KEY1 select HARD level, pause the game */
void EXTI3_IRQHandler(void)
{
    if (g_game_state == DIFF_LEVEL)
    {
        g_game_mode = HARD;
        EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 40, "Easy", BLUE, WHITE, 1);
        EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 0, "Hard", WHITE, BLUE, 1);
    }
    if (g_game_state == PLAYING)
    {
        g_game_state == PAUSE;
        Delay(500000);
    }
    else if (g_game_state == PAUSE)
    {
        g_game_state == PLAYING;
        Delay(500000);
    }

    EXTI->PR = 1 << 3; // Clear the pending status of EXTI3
}

/* KEY2, move left */
void EXTI2_IRQHandler(void)
{
    if (g_game_state == PLAYING)
    {
        if (g_Player_A_X > LOG_WIDTH / 2)
        {
            g_Player_A_X -= g_Player_B_speed;;
        }
        else
        {
            g_Player_A_X = LOG_WIDTH / 2;
        }
    }
    EXTI->PR = 1 << 2; // Clear the pending status of EXTI2
}

/* Key_Up, select the easy mode */
void EXTI0_IRQHandler(void)
{
    if (g_game_state == DIFF_LEVEL)
    {
        g_game_mode = EASY;
        EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 40, "Easy", WHITE, BLUE, 1);
        EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 0, "Hard", BLUE, WHITE, 1);
    }

    EXTI->PR = 1; // Clear the pending status of EXTI0
}

/* USART1, get the random number */
void USART1_IRQHandler(void)
{
    u32 num;

    if (g_game_state == USART)
    {
        if (USART1->SR & (1 << 5))
        {                     // If read data register is not empty
            num = USART1->DR; // Read the received data character

            EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, "The random number received is: ", WHITE, RED, 1);
            EIE3810_TFTLCD_ShowChar1608(SCREEN_WIDTH / 2 - 100 + 32 * 8, SCREEN_HEIGHT / 2 + 50, num + '0', WHITE, RED);
            Delay(5000000);
            g_game_state = COUNT_DOWN;
        }
    }
}