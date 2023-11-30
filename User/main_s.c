#include "stm32f10x.h"
#include "EIE3810_Clock.h"
#include "EIE3810_TFTLCD.h"
#include "EIE3810_USART.h"
#include "EIE3810_Key.h"
#include "EIE3810_JOYPAD.h"
#include "EIE3810_Interrupt.h"
#include "EIE3810_Timer.h"
#include "EIE3810_Buzzer.h"

#define PLAYING 0
#define PAUSED 1
#define WIN_A 2 // Player A win (bottom)
#define WIN_B 3 // Player B win (upper)

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 800
#define BOUND_LEFT BALL_RADIUS
#define BOUND_RIGHT SCREEN_WIDTH - BALL_RADIUS
#define BOUND_UP BAR_WIDTH + BALL_RADIUS + 1
#define BOUND_DOWN SCREEN_HEIGHT - BAR_WIDTH - BALL_RADIUS - 1

#define BALL_RADIUS 10

#define BAR_LEN 80
#define BAR_WIDTH 10
#define BAR_A_Y SCREEN_HEIGHT - BAR_WIDTH
#define BAR_B_Y 0

#define COLOR_BG WHITE
#define COLOR_BALL RED
#define COLOR_BAR BLACK

#define COLLIDE_UP 1
#define COLLIDE_DOWN 2
#define COLLIDE_LEFT 3
#define COLLIDE_RIGHT 4

#define LCD_POSX_ROUND 10
#define LCD_POSY_ROUND SCREEN_HEIGHT / 2
#define LCD_POSX_TIME 10
#define LCD_POSY_TIME LCD_POSY_ROUND + 30

#define DIFFICULTY_EASY 0
#define DIFFICULTY_HARD 1

const int STEPS[8][2] = {
	// {{x, y}}
	{10, -6},
	{8, -8},
	{5, -10},
	{2, -11},
	{-2, -11},
	{-5, -10},
	{-8, -8},
	{-10, -6}};

u8 g_page_num = 1; // which page the LCD is currently displayed
int g_step_x, g_step_y;
int g_ball_x, g_ball_y;
int g_barA_x, g_barB_x; // left most x positions

u32 g_time = 0;
u32 g_bounce_count = 0;

u8 g_game_state = PAUSED;

u8 g_difficulty = DIFFICULTY_EASY; // 0: easy, 1: hard

void Delay(u32);
void display_keys(u8 keys);
void update_page(void);
void initialize_game(void);
void count_down_timer(int num, u16 color);

void draw_ball_on_LCD(void);
void remove_ball_on_LCD(u16 x, u16 y);

void draw_barA_on_LCD(void);
void remove_barA_on_LCD(void);

void draw_barB_on_LCD(void);
void remove_barB_on_LCD(void);

u8 is_ball_out_bound(int step_x, int step_y);

void gtime_to_ms(u32 gtime, u32 *minutes, u32 *seconds);

void update_time_on_LCD(u32 gtime);
void update_round_on_LCD(u32 round);

int main(void)
{
	EIE3810_clock_tree_init();
	EIE3810_NVIC_SetPriorityGroup(5); // Set PRIGROUP

	EIE3810_TIM3_Init(4999, 3599); // Initialize TIM3; The frequency is 72MHz / 3600 / 5000 = 4Hz
	EIE3810_TIM4_Init(4999, 1799); // Initialize TIM4

	EIE3810_USART1_init(72, 14400); // baud rate is 14400
	EIE3810_USART1_EXTIInit();

	EIE3810_Buzzer_Init();

	EIE3810_Key0_EXTIInit();  // Initialize Key0 as an interrupt input
	EIE3810_Key1_EXTIInit();  // Initialize Key1 as an interrupt input
	EIE3810_Key2_EXTIInit();  // Initialize Key2 as an interrupt input
	EIE3810_KeyUp_EXTIInit(); // Initialize KeyUp as an interrupt input

	EIE3810_TFTLCD_Init();
	EIE3810_TFTLCD_Clear(COLOR_BG);

	JOYPAD_Init();

	while (1)
	{
		g_page_num = 1; // intro page
		g_game_state = PAUSED;
		update_page();
		Delay(1000000);

		g_page_num = 2; // select difficulty page
		g_difficulty = DIFFICULTY_EASY;
		update_page();
		while (g_page_num == 2)
			;

		update_page(); // USART page
		while (g_page_num == 3)
			;
		Delay(20000000);

		update_page();
		while (g_page_num == 4)
			; // count down timer

		update_page();
		while (g_page_num == 5)
			; // game playing
		Delay(10000);

		update_page();
		Delay(20000000); // delay for another round
	}
}

void Delay(u32 count) // Use looping for delay
{
	u32 i;
	for (i = 0; i < count; i++)
		;
}

// update the LCD based on the page number
void update_page()
{
	EIE3810_TFTLCD_Clear(COLOR_BG);
	if (g_page_num == 1)
	{
		EIE3810_TFTLCD_ShowStringBig(100, 100, "Welcome to mini Project!", WHITE, BLUE);
		Delay(10000000);
		EIE3810_TFTLCD_ShowStringSmall(120, 150, "This is the Final Lab.", WHITE, RED);
		Delay(10000000);
		EIE3810_TFTLCD_ShowStringSmall(120, 200, "Are you ready?", WHITE, RED);
		Delay(10000000);
		EIE3810_TFTLCD_ShowStringSmall(120, 250, "OK! Let's start.", WHITE, RED);
		Delay(10000000);
	}
	else if (g_page_num == 2)
	{
		EIE3810_TFTLCD_ShowStringSmall(100, 100, "Please select the difficulty level:", WHITE, RED);
		EIE3810_TFTLCD_ShowStringSmall(100, 150, "Easy", WHITE, BLUE);
		EIE3810_TFTLCD_ShowStringSmall(100, 200, "Hard", BLUE, WHITE);
		EIE3810_TFTLCD_ShowStringSmall(100, 250, "Press KEY0 to enter.", WHITE, RED);
	}
	else if (g_page_num == 3)
	{
		EIE3810_TFTLCD_ShowStringBig(50, 100, "Use USART for a random direction", WHITE, RED);
	}
	else if (g_page_num == 4)
	{
		count_down_timer(3, BLUE);
		g_page_num++;
	}
	else if (g_page_num == 5)
	{
		// initialize the game from the random direction
		initialize_game();
	}
	else if (g_page_num == 6)
	{
		EIE3810_TFTLCD_Clear(COLOR_BG);
		if (g_game_state == WIN_A)
		{
			EIE3810_TFTLCD_ShowStringSmall(120, SCREEN_HEIGHT / 2, "Player A Wins", WHITE, RED);
		}
		else if (g_game_state == WIN_B)
		{
			EIE3810_TFTLCD_ShowStringSmall(120, SCREEN_HEIGHT / 2, "Player B Wins", WHITE, RED);
		}
	}
}

// 1. Move bar 2. JOYPAD pause & continue
void TIM4_IRQHandler(void)
{
	if (TIM4->SR & (1 << 0))
	{ // Update interrupt pending: If the registers are update
		if (g_page_num == 5)
		{
			u8 input_joypad = JOYPAD_Read();

			if (g_game_state == PLAYING)
			{
				// LEFT
				if ((input_joypad >> 6) & 0x01)
				{
					remove_barB_on_LCD();
					g_barB_x -= 5;
					if (g_barB_x < 0)
					{
						g_barB_x = 0;
					}
					draw_barB_on_LCD();
				}
				// RIGHT
				if ((input_joypad >> 7) & 0x01)
				{
					remove_barB_on_LCD();
					g_barB_x += 5;
					if (g_barB_x > (SCREEN_WIDTH - BAR_LEN))
					{
						g_barB_x = (SCREEN_WIDTH - BAR_LEN);
					}
					draw_barB_on_LCD();
				}
				// START
				if ((input_joypad >> 3) & 0x01)
				{
					g_game_state = PAUSED;
				}
			}
			else if ((g_game_state == PAUSED) & ((input_joypad >> 3) & 0x01))
			{
				g_game_state = PLAYING;
			}
		}
	}
	TIM4->SR &= ~(1 << 0); // Clear the Update interrupt flag
}

// 1. Update g_time and g_bounce_count
// 2. Move ball, check game status & update bounce times
// 3. Update time and round number on LCD
void TIM3_IRQHandler(void)
{
	if (TIM3->SR & (1 << 0))
	{ // Update interrupt pending: If the registers are update
		if (g_game_state == PLAYING)
		{
			g_time++;
			remove_ball_on_LCD(g_ball_x, g_ball_y);

			// update ball's position & round number
			u8 ball_out_bound = is_ball_out_bound(g_step_x, g_step_y);
			switch (ball_out_bound)
			{
			case COLLIDE_UP:
			{
				g_bounce_count++;
				g_ball_x += g_step_x;
				g_ball_y = BOUND_UP;
				// collide with bar
				if ((g_ball_x >= g_barB_x) && (g_ball_x <= g_barB_x + BAR_LEN))
				{
					g_step_y *= (-1);
				}
				else
				{ // collide with bound
					g_game_state = WIN_A;
					g_page_num++;
				}
				break;
			}
			case COLLIDE_DOWN:
			{
				g_bounce_count++;
				g_ball_x += g_step_x;
				g_ball_y = BOUND_DOWN;
				// collide with bar
				if ((g_ball_x >= g_barA_x) && (g_ball_x <= g_barA_x + BAR_LEN))
				{
					g_step_y *= (-1);
				}
				else
				{ // collide with bound
					g_game_state = WIN_B;
					g_page_num++;
				}
				break;
			}
			case COLLIDE_LEFT:
			{
				g_ball_x = BOUND_LEFT;
				g_ball_y += g_step_y;
				g_step_x *= (-1);
				BUZZER_ON;
				Delay(100000);
				BUZZER_OFF;

				break;
			}
			case COLLIDE_RIGHT:
			{
				g_ball_x = BOUND_RIGHT;
				g_ball_y += g_step_y;
				g_step_x *= (-1);
				BUZZER_ON;
				Delay(100000);
				BUZZER_OFF;
				break;
			}
			case 0:
			{
				g_ball_x += g_step_x;
				g_ball_y += g_step_y;
				break;
			}
			default:
			{
				// Never reached
			}
			}

			draw_ball_on_LCD();
			
			// update round & time on LCD
			update_time_on_LCD(g_time);
			update_round_on_LCD(g_bounce_count);
		}
	}
	TIM3->SR &= ~(1 << 0); // Clear the Update interrupt flag
}

// Key0
void EXTI4_IRQHandler(void)
{
	if (g_page_num == 2)
	{
		g_page_num++;
	}
	else if ((g_page_num == 5) && (g_game_state == PLAYING))
	{
		remove_barA_on_LCD();
		g_barA_x += 5;
		if (g_barA_x > (SCREEN_WIDTH - BAR_LEN))
		{
			g_barA_x = (SCREEN_WIDTH - BAR_LEN);
		}
		draw_barA_on_LCD();
		Delay(1000);
	}

	EXTI->PR = 1 << 4; // Clear the pending status of EXTI4
	
}

// Key1
void EXTI3_IRQHandler(void)
{
	if (g_page_num == 2)
	{
		// Hard
		EIE3810_TFTLCD_ShowStringSmall(100, 150, "Easy", BLUE, WHITE);
		EIE3810_TFTLCD_ShowStringSmall(100, 200, "Hard", WHITE, BLUE);
		g_difficulty = DIFFICULTY_HARD;
	}
	else if (g_page_num == 5)
	{
		if (g_game_state == PLAYING)
		{
			g_game_state = PAUSED;
		}
		else if (g_game_state == PAUSED)
		{
			g_game_state = PLAYING;
		}
		Delay(1000);
	}

	EXTI->PR = 1 << 3; // Clear the pending status of EXTI3
}

// Key2
void EXTI2_IRQHandler(void)
{
	if ((g_page_num == 5) && (g_game_state == PLAYING))
	{
		remove_barA_on_LCD();
		g_barA_x -= 5;
		if (g_barA_x < 0)
		{
			g_barA_x = 0;
		}
		draw_barA_on_LCD();
		Delay(1000);
	}
	EXTI->PR = 1 << 2; // Clear the pending status of EXTI2
}

// Key_Up
void EXTI0_IRQHandler(void)
{
	if (g_page_num == 2)
	{
		// Easy
		EIE3810_TFTLCD_ShowStringSmall(100, 150, "Easy", WHITE, BLUE);
		EIE3810_TFTLCD_ShowStringSmall(100, 200, "Hard", BLUE, WHITE);
		g_difficulty = DIFFICULTY_EASY;
	}

	EXTI->PR = 0x01; // Clear the pending status of EXTI0
}

// USART1
void USART1_IRQHandler(void)
{
	u32 buffer;

	if (g_page_num == 3)
	{
		if (USART1->SR & (1 << 5))
		{						 // If read data register is not empty
			buffer = USART1->DR; // Read the received data character

			if (buffer < 8)
			{
				EIE3810_TFTLCD_ShowStringSmall(100, 140, "The random number received is: ", WHITE, RED);
				EIE3810_TFTLCD_ShowChar1608(348, 140, buffer + '0', WHITE, RED);
				g_step_x = STEPS[buffer][0];
				g_step_y = STEPS[buffer][1];

				if (g_difficulty == DIFFICULTY_HARD)
				{
					g_step_x *= 2;
					g_step_y *= 2;
				}

				g_page_num++;
			}
		}
	}
}

// The 3-second countdown timer
void count_down_timer(int num, u16 color)
{
	for (; num >= 0; num--)
	{
		EIE3810_TFTLCD_SevenSegment(240 - 37, 400 + 70, num, color, COLOR_BG);
		Delay(10000000);
	}
}

void initialize_game(void)
{
	g_time = 0;
	g_bounce_count = 0;
	update_time_on_LCD(g_time);
	update_round_on_LCD(g_bounce_count);

	g_ball_x = (BOUND_LEFT + BOUND_RIGHT) / 2;
	g_ball_y = BOUND_DOWN - BALL_RADIUS;
	g_barA_x = (SCREEN_WIDTH - BAR_LEN) / 2;
	g_barB_x = (SCREEN_WIDTH - BAR_LEN) / 2;
	draw_ball_on_LCD();
	draw_barA_on_LCD();
	draw_barB_on_LCD();
	g_game_state = PLAYING;
}

void draw_ball_on_LCD(void)
{
	EIE3810_TFTLCD_DrawCircle(g_ball_x, g_ball_y, BALL_RADIUS, CIRCLE_FULL, COLOR_BALL);
}

void remove_ball_on_LCD(u16 x, u16 y)
{
	EIE3810_TFTLCD_DrawCircle(x, y, BALL_RADIUS, CIRCLE_FULL, COLOR_BG);
}

void draw_barA_on_LCD(void)
{
	EIE3810_TFTLCD_DrawRectangle(g_barA_x, BAR_LEN, BAR_A_Y, BAR_WIDTH, COLOR_BAR);
}

void remove_barA_on_LCD(void)
{
	EIE3810_TFTLCD_DrawRectangle(g_barA_x, BAR_LEN, BAR_A_Y, BAR_WIDTH, COLOR_BG);
}

void draw_barB_on_LCD(void)
{
	EIE3810_TFTLCD_DrawRectangle(g_barB_x, BAR_LEN, BAR_B_Y, BAR_WIDTH, COLOR_BAR);
}

void remove_barB_on_LCD(void)
{
	EIE3810_TFTLCD_DrawRectangle(g_barB_x, BAR_LEN, BAR_B_Y, BAR_WIDTH, COLOR_BG);
}

// Return 0 if not collide bounds, 1 if collide with upper bound,
// 2 if collide with lower bound, 3 if collide with left bound, 
// 4 if collide with right bound
u8 is_ball_out_bound(int step_x, int step_y) {
	if (g_ball_x + step_x <= BOUND_LEFT) {
		return COLLIDE_LEFT;
	}
	if (g_ball_x + step_x >= BOUND_RIGHT) {
		return COLLIDE_RIGHT;
	}
	if (g_ball_y + step_y <= BOUND_UP) {
		return COLLIDE_UP;
	}
	if (g_ball_y + step_y >= BOUND_DOWN) {
		return COLLIDE_DOWN;
	}
	return 0;
}

// Translate g_time to minutes and seconds
void gtime_to_ms(u32 gtime, u32 *minutes, u32 *seconds)
{
	// This inversion is determined by the TIM3 frequency.
	// Value should be changed if TIM3 timer frequency changes
	u32 total_seconds = gtime / 4;

	*minutes = total_seconds / 60;
	*seconds = total_seconds % 60;
}

// Display the time on the LCD
void update_time_on_LCD(u32 gtime)
{
	u32 minutes, seconds;
	u32 seconds_tens, seconds_ones; // tens & ones place of "seconds"
	gtime_to_ms(gtime, &minutes, &seconds);
	seconds_tens = seconds / 10;
	seconds_ones = seconds % 10;

	EIE3810_TFTLCD_ShowStringSmall(LCD_POSX_TIME, LCD_POSY_TIME, "Time", BLUE, WHITE);
	EIE3810_TFTLCD_ShowChar1608(LCD_POSX_TIME + 6 * 8, LCD_POSY_TIME, '0' + minutes, BLUE, WHITE); // leave 6 char's space
	EIE3810_TFTLCD_ShowChar1608(LCD_POSX_TIME + 7 * 8, LCD_POSY_TIME, 'm', BLUE, WHITE);

	EIE3810_TFTLCD_ShowChar1608(LCD_POSX_TIME + 8 * 8, LCD_POSY_TIME, '0' + seconds_tens, BLUE, WHITE);
	EIE3810_TFTLCD_ShowChar1608(LCD_POSX_TIME + 9 * 8, LCD_POSY_TIME, '0' + seconds_ones, BLUE, WHITE);
}

// Display the bounce number on the LCD
void update_round_on_LCD(u32 round)
{
	u32 round_tens = round / 10;
	u32 round_ones = round % 10; // tens & ones place of "round"
	EIE3810_TFTLCD_ShowStringSmall(LCD_POSX_ROUND, LCD_POSY_ROUND, "Round", BLUE, WHITE);
	EIE3810_TFTLCD_ShowChar1608(LCD_POSX_ROUND + 6 * 8, LCD_POSY_ROUND, '0' + round_tens, BLUE, WHITE); // leave 6 char's space
	EIE3810_TFTLCD_ShowChar1608(LCD_POSX_ROUND + 7 * 8, LCD_POSY_ROUND, '0' + round_ones, BLUE, WHITE);
}
