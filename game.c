#include <gb/gb.h>
#include <stdint.h>
#include <rand.h>

#include "lib/gbt_player.h"
#include "map.c"
#include "cursor.c"

#include "game.h"
#include "lib/utils.h"
#include "globals.h"

// 0 - wihte
// 1 - black
// 2 - grey
// 3 - hard grey
UBYTE playground[] = {
	1,  1,  2,  2,  2,  2,  1,  1, 3,
	1,  2,  2,  2,  2,  2,  2,  1, 3,
	2,  2,  2,  2,  2,  2,  2,  2, 3,
	2,  2,  1,  2,  1,  2,  2,  2, 3,
	2,  2,  1,  2,  1,  2,  2,  2, 3,
	2,  2,  2,  1,  2,  2,  2,  1, 3,
	1,  2,  2,  2,  2,  2,  1,  1, 3,
	1,  2,  2,  2,  2,  2,  1,  1, 3,
	1,  2,  1,  2,  1,  2,  1,  1, 3
};

UBYTE playground_old[9*9];

extern const unsigned char * song_game_Data[];

// functions declarations
void draw_xy(BYTE x, BYTE y, UBYTE color);
UBYTE get_color(BYTE x, BYTE y);
void set_color(BYTE x, BYTE y, UBYTE color);
uint8_t get_tile_ul(UBYTE square_color, UBYTE neighbour_color);
uint8_t get_tile_ur(UBYTE square_color, UBYTE neighbour_color);
uint8_t get_tile_dl(UBYTE square_color, UBYTE neighbour_color);
uint8_t get_tile_dr(UBYTE square_color, UBYTE neighbour_color);
void draw_all();

void move_cursor(int x, int y, uint8_t state);
void place_cursor(int x, int y);

void fill_all();
void fill_void();
uint8_t delete_zone(int x, int y);
void fall_y();
void fall_x();
void physics_engine();
uint8_t compare_playgrounds();
void copy_playgrounds();
void init_playgrounds();
uint8_t rand_range();

// global variables
uint8_t cursor_x, cursor_y, cursor_state, action_state, tics;
uint8_t combo, bomb, bonus, next_bomb;
uint16_t new_bomb;

void run_game(void)
{
	uint8_t i, j;
	
	// load map data
	set_bkg_tiles(0, 0, 20, 18, mergez);
	// Turns on the background layer
	SHOW_BKG;

	// init sound player
	disable_interrupts();
    gbt_play(song_game_Data, 1, 7);
    gbt_loop(1);
    set_interrupts(VBL_IFLAG);
    enable_interrupts();
	
	// init seed for random
	initrand(DIV_REG);
	
	//initialize vars
	i = 0;
	j = 0;
	cursor_x = 4;
	cursor_y = 4;
	cursor_state = WAIT;
	action_state = WAIT;
	score = 0;
	bomb = 10;
	new_bomb = 64;
	next_bomb = 64;
	tics = 0;
	
	// sprite load
	SPRITES_8x8;
    set_sprite_data(0, 1, cursor);
	set_sprite_tile(0, 0);
	place_cursor(cursor_x, cursor_y);
	SHOW_SPRITES;
	
	// Turns the display back on. 
	DISPLAY_ON;
	
	text_print_string_bkg(14, 1, "SCORE");
	text_print_string_bkg(1, 15, "CHAIN");
	text_print_char_bkg(1, 1, '\\');
	text_print_string_bkg(1, 2, "NEXT");
	print_uint8_bkg(1, 16, 0, 2);	//combo
	print_uint8_bkg(2, 1, bomb, 2);
	print_uint16_bkg(14, 2, score, 5);
	print_uint8_bkg(1, 3, next_bomb, 2);
	
	init_playgrounds();
	draw_all();
	
	fade_in(50);
	
    // Loop forever
    while(bomb) {

		// Game main loop processing goes here
		physics_engine();
		draw_all();
		copy_playgrounds();
		
		if(tics){
			if(tics==1){
				// delete bouns on screen
				
				text_print_string_bkg(1, 2, "NEXT");
				print_uint8_bkg(1, 3, next_bomb, 2);
				text_print_string_bkg(3, 16, "   ");
			}
			tics--;
		}
		
		if(cursor_state == WAIT){
			if(joypad() & J_UP) {
				cursor_state = UP;
			}
			if(joypad() & J_DOWN) {
				cursor_state = DOWN;
			}
			if(joypad() & J_RIGHT) {
				cursor_state = RIGHT;
			}
			if(joypad() & J_LEFT) {
				cursor_state = LEFT;
			}
			move_cursor(cursor_x, cursor_y, cursor_state);
		}
		place_cursor(cursor_x, cursor_y);
		
		if(action_state == WAIT) {
			if(joypad() & J_A) {
				action_state = ACTION_A;
			}
		}
		
		// if no directions are pressed, resert cursor_state
		if(!(joypad() & 0x0FU) && action_state == WAIT){
			cursor_state = WAIT;
		}
		
		// Done processing, yield CPU and wait for start of next frame
        wait_vbl_done();
		gbt_update();
    }
	if(score > hi_score){hi_score = score;}
	fade_out(50);
	title_art = rand_range();
	gbt_stop();
	clear_bkg();
	HIDE_SPRITES;
	DISPLAY_OFF;
	HIDE_BKG;
}

void draw_xy(BYTE x, BYTE y, UBYTE color){
	UBYTE neighbour;
	uint8_t x_new, y_new;
	x_new = 1 + x + y;
	y_new = 8 - x + y;
	neighbour = get_color(x,  y-1);
	set_bkg_tile_xy(x_new, y_new, get_tile_ul(color, neighbour));
	neighbour = get_color(x+1,  y);
	set_bkg_tile_xy(x_new+1, y_new, get_tile_ur(color, neighbour));
	neighbour = get_color(x-1,  y);
	set_bkg_tile_xy(x_new, y_new+1, get_tile_dl(color, neighbour));
	neighbour = get_color(x,  y+1);
	set_bkg_tile_xy(x_new+1, y_new+1, get_tile_dr(color, neighbour));
}

UBYTE get_color(BYTE x, BYTE y){
	if((x >= 0 && x < 9) && (y >= 0 && y < 9)){
		return playground[y*9+x];
	}else{
		return 0;
	}
}

void set_color(BYTE x, BYTE y, UBYTE color){
	playground[y*9+x] = color;
}

// up - left
uint8_t get_tile_ul(UBYTE square_color, UBYTE neighbour_color){
	if (square_color == 0){
		switch (neighbour_color){
			case 0 :
				return 0;
			break;
			case 1 :
				return 7;
			break;
			case 2 :
				return 14;
			break;
			case 3 :
				return 19;
			break;
		}
	}
	if (square_color == 1){
		switch (neighbour_color){
			case 0 :
				return 6;
			break;
			case 1 :
				return 1;
			break;
			case 2 :
				return 10;
			break;
			case 3 :
				return 22;
			break;
		}
	}
	if (square_color == 2){
		switch (neighbour_color){
			case 0 :
				return 15;
			break;
			case 1 :
				return 11;
			break;
			case 2 :
				return 2;
			break;
			case 3 :
				return 26;
			break;
		}
	}
	if (square_color == 3){
		switch (neighbour_color){
			case 0 :
				return 18;
			break;
			case 1 :
				return 23;
			break;
			case 2 :
				return 27;
			break;
			case 3 :
				return 3;
			break;
		}
	}
	return 0;
}
// up - right
uint8_t get_tile_ur(UBYTE square_color, UBYTE neighbour_color){
	if (square_color == 0){
		switch (neighbour_color){
			case 0 :
				return 0;
			break;
			case 1 :
				return 5;
			break;
			case 2 :
				return 12;
			break;
			case 3 :
				return 3;
			break;
		}
	}
	if (square_color == 1){
		switch (neighbour_color){
			case 0 :
				return 4;
			break;
			case 1 :
				return 1;
			break;
			case 2 :
				return 8;
			break;
			case 3 :
				return 20;
			break;
		}
	}
	if (square_color == 2){
		switch (neighbour_color){
			case 0 :
				return 13;
			break;
			case 1 :
				return 9;
			break;
			case 2 :
				return 2;
			break;
			case 3 :
				return 24;
			break;
		}
	}
	if (square_color == 3){
		switch (neighbour_color){
			case 0 :
				return 16;
			break;
			case 1 :
				return 21;
			break;
			case 2 :
				return 25;
			break;
			case 3 :
				return 3;
			break;
		}
	}
	return 0;
}
// down - left
uint8_t get_tile_dl(UBYTE square_color, UBYTE neighbour_color){
	if (square_color == 0){
		switch (neighbour_color){
			case 0 :
				return 0;
			break;
			case 1 :
				return 4;
			break;
			case 2 :
				return 13;
			break;
			case 3 :
				return 16;
			break;
		}
	}
	if (square_color == 1){
		switch (neighbour_color){
			case 0 :
				return 5;
			break;
			case 1 :
				return 1;
			break;
			case 2 :
				return 9;
			break;
			case 3 :
				return 21;
			break;
		}
	}
	if (square_color == 2){
		switch (neighbour_color){
			case 0 :
				return 12;
			break;
			case 1 :
				return 8;
			break;
			case 2 :
				return 2;
			break;
			case 3 :
				return 25;
			break;
		}
	}
	if (square_color == 3){
		switch (neighbour_color){
			case 0 :
				return 17;
			break;
			case 1 :
				return 20;
			break;
			case 2 :
				return 24;
			break;
			case 3 :
				return 3;
			break;
		}
	}
	return 0;
}
// down - right
uint8_t get_tile_dr(UBYTE square_color, UBYTE neighbour_color){
	if (square_color == 0){
		switch (neighbour_color){
			case 0 :
				return 0;
			break;
			case 1 :
				return 6;
			break;
			case 2 :
				return 15;
			break;
			case 3 :
				return 18;
			break;
		}
	}
	if (square_color == 1){
		switch (neighbour_color){
			case 0 :
				return 7;
			break;
			case 1 :
				return 1;
			break;
			case 2 :
				return 11;
			break;
			case 3 :
				return 23;
			break;
		}
	}
	if (square_color == 2){
		switch (neighbour_color){
			case 0 :
				return 14;
			break;
			case 1 :
				return 10;
			break;
			case 2 :
				return 2;
			break;
			case 3 :
				return 27;
			break;
		}
	}
		if (square_color == 3){
		switch (neighbour_color){
			case 0 :
				return 19;
			break;
			case 1 :
				return 22;
			break;
			case 2 :
				return 26;
			break;
			case 3 :
				return 3;
			break;
		}
	}
	return 0;
}

void draw_all(){
	uint8_t i, j;
	for(i=0; i<9; i++){
		for(j=0; j<9; j++){
			 if(playground[j*9+i]!=playground_old[j*9+i]){
				draw_xy(i, j, get_color(i, j));
			}
		}
	}
}

// move the custor on the virtual playground
void move_cursor(int x, int y, uint8_t state){
	if(state == UP && y > 0){
		cursor_y--;
	}
	if(state == DOWN && y < 8){
		cursor_y++;
	}
	if(state == LEFT && x > 0){
		cursor_x--;
	}
	if(state == RIGHT && x < 8){
		cursor_x++;
	}
}

// place the custor on the real playground
void place_cursor(int x, int y){
	move_sprite(0, 20 + x*8 + y*8, 84 - x*8 + y*8);
}

void fill_all(){
	uint8_t i, j;
	for(i=0; i<9; i++){
		for(j=0; j<9; j++){
			playground[j*9+i] = rand_range();
		}
	}
}

void fill_void(){
	uint8_t i, j;
	for(i=0; i<9; i++){
		for(j=0; j<9; j++){
			if(playground[j*9+i] == 0){
				playground[j*9+i] = rand_range();
			}
		}
	}
}

// I now recursiv function looks like a bad idea on Gameboy
// 
uint8_t delete_zone(int x, int y){
	UBYTE zone_color;
	uint8_t destroyed;
	destroyed = 1;
	zone_color = get_color(x, y);
	if(zone_color!=0){		// avoid infinite loop by reject the void deleting ;)
		set_color(x, y, 0);
		if(zone_color == get_color(x-1, y)){
			destroyed += delete_zone(x-1, y);
		}
		if(zone_color == get_color(x, y-1)){
			destroyed += delete_zone(x, y-1);
		}
		if(zone_color == get_color(x+1, y)){
			destroyed += delete_zone(x+1, y);
		}
		if(zone_color == get_color(x, y+1)){
			destroyed += delete_zone(x, y+1);
		}
	}
	return destroyed;
}

void fall_y(){
	uint8_t i, j;
	UBYTE square_color;
	for(j=8; j>0; j--){
		for(i=0; i<9; i++){
			if(get_color(i, j) == 0){
				square_color = get_color(i, j-1);
				set_color(i, j, square_color);
				set_color(i, j-1, 0);
			}
		}
	}
}
void fall_x(){
	uint8_t i, j;
	UBYTE square_color;
	for(i=0; i<8; i++){
		for(j=0; j<9; j++){
			if(get_color(i, j) == 0){
				square_color = get_color(i+1, j);
				set_color(i, j, square_color);
				set_color(i+1, j, 0);
			}
		}
	}
}

void physics_engine(){
	if(action_state == FILL){
		fill_void();
		action_state = WAIT;
	}
	if(action_state == FALL_X){
		copy_playgrounds();
		fall_x();
		if(compare_playgrounds()){
			action_state = FILL;
			bomb--;
			print_uint8_bkg(2, 1, bomb, 2);
		}
	}
	if(action_state == FALL_Y){
		copy_playgrounds();
		fall_y();
		if(compare_playgrounds()){
			action_state = FALL_X;
		}
	}
	if(action_state == ACTION_A){
		combo = delete_zone(cursor_x,cursor_y);
		print_uint8_bkg(1, 16, combo, 2);
		bonus = combo / 8;
		score += combo + bonus;
		print_uint16_bkg(14, 2, score, 5);
		if(score >= new_bomb){
			new_bomb += NEW_BOMB_STEP;
			bomb++;
			tics = 60;
			text_print_string_bkg(1, 2, "NEW\\");
			text_print_string_bkg(1, 3, "  ");
		}
		//print_uint16_bkg(14, 3, new_bomb, 5);
		next_bomb = new_bomb - score;
		if(!tics){
			text_print_string_bkg(1, 2, "NEXT");
			print_uint8_bkg(1, 3, next_bomb, 2);
		}
		if(bonus>0){
			text_print_char_bkg(3, 16, '+');
			print_uint8_bkg(4, 16, bonus, 2);
			tics = 60;
		}
		action_state = FALL_Y;
		
	}
}

// retrun 1 if playgrounds are identicals, 0 ether
uint8_t compare_playgrounds(){
	uint8_t i;
	for(i=0; i<81; i++){
		if(playground[i]!=playground_old[i]){
			return 0;
			break;
		}
	}
	return 1;
}
void copy_playgrounds(){
	uint8_t i;
	for(i=0; i<81; i++){
		playground_old[i]=playground[i];
	}
}
void init_playgrounds(){
	uint8_t i;
	for(i=0; i<81; i++){
		playground_old[i]=0;
	}
	fill_all();
}

uint8_t rand_range(){
	uint8_t res;
	res = 0;
	while(!res){
		res = rand() & 3;
	}
	return res;
}