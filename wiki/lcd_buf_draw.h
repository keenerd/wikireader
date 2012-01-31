/*
 * Copyright (c) 2009 Openmoko Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
  Assumptions:

  1. Support up to 9 fonts - the default font and font 0 to font 7.  For
  example, the normal 12-pixel font is used as the default font.

  2. There is a default line space (e.g. 16 pixels).

  3. The characters in the line are horizontally aligned to the bottom
  of the line by default.  Most characters got 2 blank pixel space at
  the bottom with some exceptions.  For example, "_" only got one
  blank pixel line and "gjpqy" got no blank pixel space.  A 12-pixel
  character actually takes 14 pixels from top to the bottom on
  Windows and I think it's a good practice.  To save the storage, we
  can still bitmap them in 12 pixels, but the font API should be
  smart enough to return 14 pixels by padding with blank pixels at
  the top or the bottom.

  4. Each character in the line is adjacent to the previous character
  without space.  The font API should pad one blank pixel space at
  the right of the character.  (In some Chinese fonts, the blank
  pixel space is at the left of the character and some characters
  leave no blank space at all.  Since we are dealing with English
  wiki, it is less important.)

  5. If characters overlap, the glyphs of all characters retain.  (For
  example, space does not overwrite the overlapped character since it
  got no glyphs.)

  6. New line is adjacent to the previous line.

  7. The article content is coded in UTF-8.  Some UTF-8 control
  characters are used as escape characters.

  1 - escape character 0 (space line), the byte after it contains the
  pixels of the space line

  2 - escape character 1 (new line with the default font and the default
  line space)

  3 - escape character 2 (new line with the same font and line space as
  the previous line, not considering the font changes and
  adjustments)

  4 - escape character 3 (new line with non-default font and line space)
  Definition of the byte after it:
  bit0~2 - font id (1 ~ 7)
  bit3~7 - pixels of the line space

  5 - escape character 4 (change font)

  The font change (including y alignment adjustment) is reset after
  the escape character 1, 3, 4 or 5.

  Definition of the byte after it:

  bit0~2 - font id (1 ~ 7)

  bit3~7 - adjustment to the default y alignment
  (0: 0; 0x01~0x0F: 1~15; 0x10~0x1F: -16~-1)

  6 - escape character 5 (reset to the default font)

  7 - escape character 6 (reset to the default y alignment)

  8 - escape character 7 (forward), the byte after it stands for the
  pixels of move right (as the starting point of the next character).

  The next character or drawing line starts at the position after the movement.
  The rightmost position after the movement is the rightmost pixel of the screen.

  9 - escape character 8 (backward), the byte after it stands for the
  pixels to move left (from the original starting point of the next
  character).

  The next character or line starts at the position after the movement.

  The leftmost position after the movement is the leftmost pixel of the screen.

  10 - escape character 9 (alignment y adjustment), the byte after
  it stand for the pixels to adjust to the default vertical
  alignment (0x01~0x7F: 1~127; 0x80~0xFF: -128~-1)

  It takes effect for all lines after that until another excape
  character 9 or escape character 6 is encountered.

  11 - escape character 10 (drawing horizontal line from right to left,
  1 pixel thick), the next byte after it stands for the length of
  the line in pixels.

  The line starts at 1 pixel left to the starting position of the
  next character.

  Move forward/backward or set the alignment adjustment before
  drawing lines as necessary.

  The horizontal line does not change the x position of
  the next character.

  12 - escape character 11 (drawing vertical line from bottom to top, 1
  pixel thick), the next byte after it stands for the length of the
  line in pixels.

  The vertical line starts at the current x position (the
  starting position of the next character) and the bottom of the
  current y alignment.

  Move forward/backward or set the y alignment adjustment before
  drawing lines as necessary.

  The vertical line does not change the y position of the next character.

  13 - escape character 12 (single pixel horizontal line), the
  horizontal line occupies from the left-most pixel to the
  right-most pixel.
  The line is 1 pixel high.


  14 - escape character 13 (single pixel vertical line), the vertical
  line pccupies from the top to the bottom of the current line
  space.

  The vertical line pccupies 1 pixel width.

  15 - escape character 14 (bitmap), the next byte after it stands for
  the width of the bitmap in pixels and the 3rd and 4th bytes sand for the height.

*/

#ifndef _LCD_BUF_DRAW_H
#define _LCD_BUF_DRAW_H

#include "grifo.h"
#include "bmf.h"

#define ESC_0_SPACE_LINE		1
#define ESC_1_NEW_LINE_DEFAULT_FONT	2
#define ESC_2_NEW_LINE_SAME_FONT        3
#define ESC_3_NEW_LINE_WITH_FONT	4
#define ESC_4_CHANGE_FONT		5
#define ESC_5_RESET_TO_DEFAULT_FONT	6
#define ESC_6_RESET_TO_DEFAULT_ALIGN	7
#define ESC_7_FORWARD			8
#define ESC_8_BACKWARD			9
#define ESC_9_Y_ADJUSTMENT		10
#define ESC_10_HORIZONTAL_LINE		11
#define ESC_11_VERTICAL_LINE		12
#define ESC_12_FULL_HORIZONTAL_LINE	13
#define ESC_13_FULL_VERTICAL_LINE	14
#define ESC_14_BITMAP			15
#define MAX_ESC_CHAR ESC_14_BITMAP

#define MAX_STATIC_ARTICLE_ID 2
#define LINE_SPACE_ADDON 1
#define LCD_BUF_WIDTH_PIXELS 240
#define LCD_LEFT_MARGIN 6
#define LCD_EXTRA_LEFT_MARGIN_FOR_IMAGE 2
#define LCD_TOP_MARGIN 6
#define LCD_BUF_HEIGHT_PIXELS 128 * 1024
#define LINK_X_DIFF_ALLOWANCE 10
#define LINK_Y_DIFF_ALLOWANCE 5
#define INITIAL_HIGHLIGHT_THRESHOLD 8
#define INITIAL_ARTICLE_SCROLL_THRESHOLD 8
#define ARTICLE_MOVING_SCROLL_THRESHOLD 1
#define LINK_DEACTIVATION_MOVE_THRESHOLD 1
#define SMOOTH_SCROLL_ACTIVATION_OFFSET_LOW_THRESHOLD 10
#define SMOOTH_SCROLL_ACTIVATION_OFFSET_HIGH_THRESHOLD 200
#define SMOOTH_SCROLL_ACTIVATION_SPPED_THRESHOLD 100
#define LIST_SMOOTH_SCROLL_SPEED_FACTOR 5
#define ARTICLE_SMOOTH_SCROLL_SPEED_FACTOR 3
#define LINK_ACTIVATION_TIME_THRESHOLD 0.1
#define SAME_CLICK_TIME_THRESHOLD 0.3
#define LCD_BUF_WIDTH_BYTES LCD_BUFFER_WIDTH/8
#define LANGUAGE_LINK_WIDTH 23
#define LANGUAGE_LINK_HEIGHT 21
#define LANGUAGE_LINK_WIDTH_GAP 6
#define LANGUAGE_LINK_HEIGHT_GAP 6
#define HIGHLIGHT_SCROLLING_SPOT_HEIGHT 6
#define SCROLL_PAGE_JUMP 190

// The italic fonts may not define the bitmaps for all characters.
// The characters that are minssing in the italic fonts will try to return the bitmaps of it's supplement_font file.
// The supplement_font file is assigned in init_lcd_draw_buf().
#define ITALIC_FONT_IDX			1
#define DEFAULT_FONT_IDX		2
#define TITLE_FONT_IDX			3
#define SUBTITLE_FONT_IDX		4
// The above are the primary fonts for the article body.  The index of any one of them needs to be under 7.
#define DEFAULT_ALL_FONT_IDX	5
#define TITLE_ALL_FONT_IDX		6
#define SUBTITLE_ALL_FONT_IDX	7

#define SEARCH_HEADING_FONT_IDX	TITLE_FONT_IDX
#define SEARCH_LIST_FONT_IDX	DEFAULT_FONT_IDX
#define MESSAGE_FONT_IDX		BOLD_FONT_IDX
#define H2_FONT_IDX				SUBTITLE_FONT_IDX
#define H3_FONT_IDX 			SUBTITLE_FONT_IDX
#define H4_FONT_IDX				SUBTITLE_FONT_IDX
#define H5_FONT_IDX				SUBTITLE_FONT_IDX
#define LICENSE_TEXT_FONT		ITALIC_FONT_IDX
#define FILE_BUFFER_SIZE		(512 * 1024)

typedef struct _LCD_DRAW_BUF
{
	unsigned char *screen_buf;
	long current_x;
	long current_y;
	pcffont_bmf_t *pPcfFont;
	int drawing;
	int line_height;
	int actual_height;
	int y_adjustment;
	int x_adjustment;
} LCD_DRAW_BUF;

#define MAX_RESULT_LIST 512
#define MAX_ARTICLE_LINKS 2048
#define MAX_EXTERNAL_LINKS 128
#define SPACE_BEFORE_LICENSE_TEXT 40
#define SPACE_AFTER_LICENSE_TEXT 5
#define MAX_ARTICLES_PER_COMPRESSION 256
#define MAX_LINES_PER_ARTICLE (24 * 1024)
#define HIGHTLIGHT_X_DIFF_ALLOWANCE 0
#define HIGHTLIGHT_Y_DIFF_ALLOWANCE 0

/* Structure of a single article in a file with mutiple articles */
/* byte 0~3: (long) offset from the beginning of the article header to the start of article text */
/* byte 4~5: (short) number of ARTICLE_LINK blocks */
/* byte 6~7: (short) number of EXTERNAL_LINK blocks */
/* section for a number of ARTICLE_LINK blocks */
/* section for a number of EXTERNAL_LINK blocks */
/* section for a number of external link strings */
/* section for the article string */
typedef struct __attribute__ ((packed)) _ARTICLE_HEADER
{
	uint32_t offset_article;
	uint16_t article_link_count;
	uint16_t external_link_count;
} ARTICLE_HEADER;

typedef struct __attribute__ ((packed)) _ARTICLE_LINK
{
	uint32_t start_xy; /* byte 0: x; byte 1~3: y */
	uint32_t end_xy;
	uint32_t article_id;
} ARTICLE_LINK;

typedef struct __attribute__ ((packed)) _EXTERNAL_LINK
{
	unsigned char *link_str;
} EXTERNAL_LINK;

typedef struct __attribute__ ((packed)) _CONCAT_ARTICLE_INFO
{
	uint32_t article_id;
	uint32_t offset_article;
	uint32_t article_len;
} CONCAT_ARTICLE_INFO;

typedef struct _ARTICLE_RENDER_INFO
{ // each line of the article got an entry
	uint32_t start_y;
	uint32_t end_y;
	const unsigned char *pBuf; // pointer to file_buffer of the first character of the line
	pcffont_bmf_t *pPcfFont;
} ARTICLE_RENDER_INFO, *PARTICLE_RENDER_INFO;

void init_lcd_draw_buf();
char* FontFile(int id);
void buf_draw_UTF8_str(const unsigned char **sUTF8);
void buf_draw_horizontal_line(unsigned long start_x, unsigned long end_x);
void buf_draw_vertical_line(unsigned long start_y, unsigned long end_y);
void buf_draw_char(ucs4_t u);
int get_UTF8_char_width(int idxFont, const unsigned char **pContent, long *lenContent, int *nCharBytes);
int render_article_with_pcf();
int render_history_with_pcf();
int render_wiki_selection_with_pcf();
void restore_search_list_page(void);
int render_search_result_with_pcf();
void display_article_with_pcf(int y_move);
void init_file_buffer();
int div_wiki(int a,int b);
int GetFontLinespace(int font);
void display_link_article(long idx_article);
void display_retrieved_article(long idx_article);
void display_str(const unsigned char *str);
void open_article_link(int x,int y);
void open_article_link_with_link_number(int article_link_number);
void scroll_article(void);
int draw_bmf_char(ucs4_t u,int font,int x,int y, int inverted, int b_clear);
int buf_draw_bmf_char(unsigned char *buf, int buf_width_pixels, int buf_width_bytes,
		      ucs4_t u,int font,int x,int y, int inverted, int b_clear);
int isArticleLinkSelected(int x,int y);
int check_invert_link(void);
void set_article_link_number(int num, unsigned long);
void reset_article_link_number(void);
int get_activated_article_link_number(void);
void init_invert_link(void);
void invert_link(int article_link_number);
int load_init_article(long);
void show_scroll_bar(int);
void msg_on_lcd(int x, int y, char *fmt, ...);
void msg_on_lcd_clear(int x, int y);
void buf_draw_UTF8_str_in_copy_buffer(unsigned char *framebuffer_copy,const unsigned char **pUTF8,int start_x,int end_x,int start_y,int end_y,int offset_x,int font_idx);
int get_external_str_pixel_width(const unsigned char *pUTF8, int font_idx);
void get_external_str_pixel_rectangle(const unsigned char *pIn, int font_idx, int *start_x, int *start_y, int *end_x, int *end_y);
int extract_str_fitting_width(const unsigned char **pIn, unsigned char *pOut, int width, int font_idx);
void lcd_buffer_set_pixel(unsigned char *membuffer,int x, int y);
void lcd_buffer_clear_pixel(unsigned char *membuffer,int x, int y);
void extract_title_from_article(unsigned char *article_buf, unsigned char *title);

extern LCD_DRAW_BUF lcd_draw_buf;
extern pcffont_bmf_t pcfFonts[FONT_COUNT];
extern const unsigned char *article_buf_pointer;
void clear_article_pos_info();
bool lcd_draw_highlight(int start_x, int start_y, int end_x, int end_y,
			int *invert_start_x, int *invert_end_x,
			int *invert_start_y_top, int *invert_start_y_bottom, int *invert_end_y_top, int *invert_end_y_bottom,
			unsigned char *search_string_actual, bool bRepaint);
int lcd_draw_get_cur_y_pos();
unsigned char *lcd_draw_get_cur_buffer();
void load_all_fonts();
void draw_progress_bar(int progressCount, int limit);
#endif /* _LCD_BUF_DRAW_H */
