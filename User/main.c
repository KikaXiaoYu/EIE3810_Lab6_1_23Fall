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

#define ORG_FRE 72000000
#define FIX_ARR 5000
#define TIM3_FRE 4
#define TIM4_FRE 16
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 800
#define LOG_HEIGHT 20
#define LOG_WIDTH 120
#define EDGE_SIZE 30
#define PLAYER_A_Y (SCREEN_HEIGHT - (LOG_HEIGHT / 2 + EDGE_SIZE))
#define PLAYER_B_Y (LOG_HEIGHT / 2 + EDGE_SIZE)
#define BALL_RADIUS 20

enum game_state
{
    RESET = -1,
    INTRO = 0,
    DIFF_LEVEL = 1,
    USART = 2,
    COUNT_DOWN = 3,
    PLAYING = 4,
    PAUSE = 5,
    GAME_OVER = 6
} g_game_state = RESET;

enum game_mode
{
    EASY = 0,
    HARD = 1
} g_game_mode = EASY;

u32 g_TIM3_local_count = 0;

u16 g_Player_A_X = SCREEN_WIDTH / 2;
u16 g_Player_B_X = SCREEN_WIDTH / 2;
u16 g_ball_X = SCREEN_WIDTH / 2;
u16 g_ball_Y = EDGE_SIZE + LOG_HEIGHT + BALL_RADIUS;
u32 g_e_time = 0;
u32 g_bounces = 0;
u32 g_ball_dir = 0;

u16 g_Player_A_X_pre = SCREEN_WIDTH / 2;
u16 g_Player_B_X_pre = SCREEN_WIDTH / 2;
u16 g_ball_X_pre = SCREEN_WIDTH / 2;
u16 g_ball_Y_pre = EDGE_SIZE + LOG_HEIGHT + BALL_RADIUS;
u16 g_ball_dir_pre = 0;

int main(void)
{
    /* initialization */
    EIE3810_clock_tree_init();
    EIE3810_Buzzer_Init();
    EIE3810_FourKeys_EXTIInit();
    EIE3810_TIM3_Init(FIX_ARR - 1, ORG_FRE / (TIM3_FRE * FIX_ARR) - 1);
    EIE3810_TIM3_Init(FIX_ARR - 1, ORG_FRE / (TIM4_FRE * FIX_ARR) - 1);
    // EIE3810_USART1_init(72, 14400);
    // EIE3810_USART1_EXTIInit();
    EIE3810_TFTLCD_Init();
    EIE3810_JOYPAD_Init();
    /* priority group */
    EIE3810_NVIC_SetPriorityGroup(5);
    Delay(1000000); // 1s

    g_game_state = INTRO;
    game_state_INTRO_init();
    while (g_game_state == INTRO)
        ;
    game_state_DIFF_LEVEL_init();
    while (g_game_state == DIFF_LEVEL)
        ;
    game_state_USART_init();
    while (g_game_state == USART)
        ;
    game_state_COUNT_DOWN();
    while (g_game_state == COUNT_DOWN)
        ;
    game_state_PLAYING_init();

    while (1)
        ;
}

void TIM3_IRQHandler(void)
{
    if (TIM3->SR & 1 << 0) // check if the capture/compare 1 interrupt flag is set
    {
        if (g_game_state == PLAYING)
        {
            g_TIM3_local_count++;
            if (g_TIM3_local_count == TIM3_FRE)
            {
                g_TIM3_local_count = 0;
                g_e_time++;
            }

            PLAYING_update();
            u16 g_Player_A_X_pre = g_Player_A_X;
            u16 g_Player_B_X_pre = g_Player_B_X;
            u16 g_ball_X_pre = g_ball_X;
            u16 g_ball_Y_pre = g_ball_Y;
            u16 g_ball_dir_pre = g_ball_dir;
        }
    }
    TIM3->SR &= ~(1 << 0); // clear capture/compare 1 interrupt flag
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
            case 6:
                /* left */
                if (g_Player_B_X > LOG_WIDTH / 2)
                {
                    g_Player_B_X -= 1;
                }
                else
                {
                    g_Player_B_X = LOG_WIDTH / 2;
                }
            case 7:
                /* right */
                if (g_Player_B_X < SCREEN_WIDTH - LOG_WIDTH / 2)
                {
                    g_Player_B_X += 1;
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

void game_state_INTRO_init(void)
{
    EIE3810_TFTLCD_Clear(WHITE);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 - 80, "Welcome to mini Project!", WHITE, BLUE, 2);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 40, "This is the Final Lab.", WHITE, RED, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 0, "Are you ready?", WHITE, RED, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - (-40), "OK! Let's start.", WHITE, RED, 1);
}

void game_state_DIFF_LEVEL_init(void)
{
    EIE3810_TFTLCD_Clear(WHITE);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 80, "Please select the difficulty level:", WHITE, RED, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 40, "Easy", WHITE, BLUE, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 0, "Hard", BLUE, WHITE, 1);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - (-40), "Press KEY0 to enter.", WHITE, RED, 1);
}

void game_state_USART_init(void)
{
    EIE3810_TFTLCD_Clear(WHITE);
    EIE3810_TFTLCD_ShowString(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, "Use USART for a random direction.", WHITE, RED, 2);
}

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

void draw_bounces(u16 color)
{
    int boun_1st = g_bounces / 10;
    int boun_2nd = g_bounces % 10;
    EIE3810_TFTLCD_ShowString(5 + 24 * 8, 5, "Bounces: ", color, WHITE, 1);
    EIE3810_TFTLCD_ShowChar1608(5 + 25 * 8, 5, '0' + boun_1st, color, WHITE);
    EIE3810_TFTLCD_ShowChar1608(5 + 26 * 8, 5, '0' + boun_2nd, color, WHITE);
}

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

// Key0
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
            g_Player_A_X += 1;
        }
        else
        {
            g_Player_A_X = SCREEN_WIDTH - LOG_WIDTH / 2;
        }
    }

    EXTI->PR = 1 << 4; // Clear the pending status of EXTI4
}

// Key1
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

// Key2
void EXTI2_IRQHandler(void)
{
    if (g_game_state == PLAYING)
    {
        if (g_Player_A_X > LOG_WIDTH / 2)
        {
            g_Player_A_X -= 1;
        }
        else
        {
            g_Player_A_X = LOG_WIDTH / 2;
        }
    }
    EXTI->PR = 1 << 2; // Clear the pending status of EXTI2
}

// Key_Up
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