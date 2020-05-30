/* ************************************************************************** */
/**
 * @file     buffer.c
 * @author   Edward Parkinson
 * @date     April 2020
 *
 * @brief
 *
 * Functions for adding and display various text buffers.
 *
 * ************************************************************************** */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#include "atomix.h"

/* ************************************************************************** */
/**
 * @brief  Clean up a text buffer, generally once it has been printed.
 *
 * @details
 *
 * Free the memory for all of the lines in the DISPLAY buffer, before freeing
 * the memory for DISPLAY.lines and re-initialising DISPLAY.
 *
 * ************************************************************************** */

void
clean_up_display (Display_t *buffer)
{
  int i;

  for (i = 0; i < buffer->nlines; ++i)
    free (buffer->lines[i].chars);
  free (buffer->lines);

  buffer->nlines = buffer->maxlen = 0;
  buffer->lines = NULL;
}

/* ************************************************************************** */
/**
 * @brief  Add a fmt string to the DISPLAY buffer.
 *
 * @param[in]  fmt  The formatted string
 * @param[in]  ...  The arguments for the formatted string
 *
 * @details
 *
 * This function assumes that the string being added is a single, entire, line.
 * A new line character is required at the end, but isn't mandatory. I think
 * this way you can slowly construct a single "display" line over multiple
 * strings stored in DISPLAY ... though, this isn't the intended way to use this
 * function.
 *
 * Uses a bit of a hack, vsnprintf, to figure out the storage requirement for
 * the string. The pointer for the new string is added to the DISPLAY buffer.
 *
 * PLS PLS PLS DO NOT HAVE A \n NEWLINE CHARACTER IN *fmt
 *
 * TODO check for \n or \r\n etc.
 *
 * ************************************************************************** */

void
add_display (Display_t *buffer, char *fmt, ...)
{
  int line_index;
  int len;
  va_list va, va_c;

  buffer->nlines++;
  buffer->lines = realloc (buffer->lines, buffer->nlines * sizeof (Line_t));
  line_index = buffer->nlines - 1;

   if (buffer->lines == NULL)
    exit_atomix (EXIT_FAILURE, "Unable to add additional line to the display buffer");

  buffer->lines[line_index].len = 0;
  buffer->lines[line_index].chars = NULL;

  /*
   * May be playing it extra safe here, but make a copy of va as va is not
   * necessarily preserved after vsnprintf...
   */

  va_start (va, fmt);
  va_copy (va_c, va);

  len = vsnprintf (NULL, 0, fmt, va);  // Hack: write 0 to NULL to determine length :-)
  buffer->lines[line_index].chars = malloc (len * sizeof (char) + 1);
  len = vsprintf (buffer->lines[line_index].chars, fmt, va_c);
  buffer->lines[line_index].chars[len] = '\0';  // Redundant, but just in case
  buffer->lines[line_index].len = len;

  if (buffer->maxlen < len)
    buffer->maxlen = len + 1;

  logfile ("%s\n", buffer->lines[line_index].chars);

  va_end (va);
  va_end (va_c);
}

/* ************************************************************************* */
/**
 * @brief  Add a line of dashes to the DISPLAY buffer.
 *
 * @param[in]  len  The number of dashes to draw.
 *
 * @details
 *
 * ************************************************************************** */

void
add_sep_display (const int len)
{
  int i;
  char line[len + 1];

  for (i = 0; i < len; ++i)
    line[i] = '-';
  line[len] = '\0';

  display_add (line);
}

/* ************************************************************************** */
/**
 * @brief  Add a message to display the current line of the buffer being viewed.
 *
 * @param[in]  win           The Window_t containing the buffer being displayed
 * @param[in]  current_line  The current line of the buffer at the top of the
 *                           window
 * @param[in]  total_lines   The total number of lines in the buffer
 *
 * @details
 *
 * When the maximum window scroll has been reached, END will be displayed
 * instead of the line number.
 *
 * TODO variable END line
 *
 * ************************************************************************** */

void
update_current_line_progress (Window_t win, int current_line, int total_lines)
{
  int len;
  char line_message[LINELEN];

  if (current_line + win.nrows - 2 > total_lines)
  {
    len = sprintf (line_message, "| END  / %-4d |", total_lines);
  }
  else
  {
    len = sprintf (line_message, "| %-4d / %-4d |", current_line, total_lines);
  }

  mvwprintw (win.window, win.nrows - 1, win.ncols - len - 2, line_message);

}

/* ************************************************************************** */
/**
 * @brief  Scroll the text buffer up and down.
 *
 * @param[in]  buffer             The buffer to scroll through
 * @param[in]  win                The Window_t which the buffer is being
 *                                displayed to
 * @param[in]  persistent_header  If true, a header will be stuck at the top
 *                                of win
 * @param[in]  header_row         The number of rows in the buffer to be
 *                                persistent
 *
 * @details
 *
 * The main purpose of this function is to allow the text buffer to be scrolled
 * both horizontally and vertically. The screen is only updated if a key has
 * been pressed which will scroll to avoid screen flicker.
 *
 * The persistent header functionality of this function allows the first
 * header_row lines in the buffer to become frozen at the top of the window. The
 * purpose of this it to act as a persistent header to make scrolling less
 * confusing when has scrolled very far and no longer has the original header
 * for reference.
 *
 * ************************************************************************** */

void
scroll_display (Display_t *buffer, Window_t win, bool persistent_header, int header_rows)
{
  int i, j;
  int ch;
  int srow_origin, scol_origin;
  int bline_start, bcol_start;
  int srow, scol;
  bool screen_position_moved;
  WINDOW *window = win.window;

  bline_start = bcol_start = 0;
  srow_origin = scol_origin = 1;
  screen_position_moved = false;

  /*
   * If there is no header, then for safety we set the number of rows to
   * 0
   */

  if (!persistent_header)
    header_rows = 0;

  /*
   * The buffer and row origin have to be incremented otherwise the persistent
   * header will be shown twice or will be overwritten by some other text in
   * the buffer
   */

  if (persistent_header)
  {
    bline_start += header_rows;
    srow_origin += header_rows;
  }

  update_status_bar ("press q or F1 to exit text view or use the ARROW KEYS to navigate");

  while ((ch = wgetch (window)))
  {
    if (ch == 'q' || ch == KEY_F(1))
      break;

    /*
     * Only allow key control when the buffer is large enough to scroll off
     * the screen
     */

    if (buffer->nlines > win.nrows - 2)
    {
      switch (ch)
      {
        case KEY_RESIZE:
          redraw_screen (SIGWINCH);
          screen_position_moved = true;
          break;
        case KEY_UP:
          bline_start--;
          screen_position_moved = true;
          break;
        case KEY_DOWN:
          bline_start++;
          screen_position_moved = true;
          break;
        case KEY_RIGHT:
          bcol_start++;
          screen_position_moved = true;
          break;
        case KEY_LEFT:
          bcol_start--;
          screen_position_moved = true;
          break;
        case KEY_NPAGE:
          bline_start += win.nrows - 2;
          screen_position_moved = true;
          break;
        case KEY_PPAGE:
          bline_start -= win.nrows - 2;
          screen_position_moved = true;
          break;
        case KEY_HOME:
          bline_start = 0;
          screen_position_moved = true;
          break;
        case KEY_END:
          bline_start = buffer->nlines - win.nrows + 2;
          screen_position_moved = true;
          break;
        default:
          break;
      }

      if (screen_position_moved)
      {
        wclear (window);

        if (bline_start < header_rows)
          bline_start = header_rows;
        if (bline_start + win.nrows - 2 > buffer->nlines)
          bline_start = buffer->nlines - win.nrows + 2;

        if (bcol_start < 0)
          bcol_start = 0;
        if (bcol_start + win.ncols - 2 > buffer->maxlen)
          bcol_start = AtomixConfiguration.current_col;

        AtomixConfiguration.current_line = bline_start;
        AtomixConfiguration.current_col = bcol_start;

        /*
         * Draw a persistent header as the user scrolls if enabled
         */

        if (persistent_header)
        {
          for (i = 0, srow = 1; i < header_rows && srow < win.nrows - 1; ++i, ++srow)
          {
            for (j = bcol_start, scol = scol_origin; j < buffer->lines[i].len && scol < win.ncols - 1; ++j, ++scol)
            {
              mvwprintw (window, srow, scol, "%c", buffer->lines[i].chars[j]);
            }
          }
        }

        /*
         * Write the buffer to screen, taking into account any header
         */

        for (i = bline_start, srow = srow_origin; i < buffer->nlines && srow < win.nrows - 1; ++i, ++srow)
        {
          for (j = bcol_start, scol = scol_origin; j < buffer->lines[i].len && scol < win.ncols - 1; ++j, ++scol)
          {
            mvwprintw (window, srow, scol, "%c", buffer->lines[i].chars[j]);
          }
        }

        update_current_line_progress (win, bline_start + 1 - header_rows, buffer->nlines - header_rows);
        wrefresh (window);
      }

      screen_position_moved = false;
    }
  }
}

/* ************************************************************************** */
/**
 * @brief  Print a buffer into CONTENT_WINDOW window.
 *
 * @param[in]  *buffer            The buffer to be displayed to screen.
 * @param[in]  scroll             Allows the buffer to be scrolled, allowed
 *                                values are SCROLL_ENABLE or SCROLL_DISABLE
 * @param[in]  persistent_header  If true, a header will be stuck at the top
 *                                of win
 * @param[in]  header_row         The number of rows in the buffer to be
 *                                persistent
 *
 * @details
 *
 * Simply iterates through all of the lines and prints them one by one before
 * refreshing the screen. If DISPLAY.nlines is zero, or if the lines pointer in
 * DISPLAY is NULL, an error message is displayed instead.
 *
 * The persistent header functionality of this function allows the first
 * header_row lines in the buffer to become frozen at the top of the window. The
 * purpose of this it to act as a persistent header to make scrolling less
 * confusing when has scrolled very far and no longer has the original header
 * for reference.
 *
 * ************************************************************************** */

void
display_buffer (Display_t *buffer, int scroll, bool persisent_header, int header_rows)
{
  int i, j;
  int line_index;
  WINDOW *window = CONTENT_VIEW_WINDOW.window;

  wclear (window);

  if (buffer->nlines == 0 || buffer->lines == NULL)
  {
    bold_message (CONTENT_VIEW_WINDOW, 1, 1, "No text in %s buffer to show", buffer->name);
    wrefresh (window);
  }
  else
  {
    for (i = 0; i < buffer->nlines && i < CONTENT_VIEW_WINDOW.nrows - 2; ++i)
    {
      line_index = i;
      for (j = 0; j < buffer->lines[line_index].len && j < CONTENT_VIEW_WINDOW.ncols - 2; ++j)
      {
        mvwprintw (window, i + 1, j + 1, "%c", buffer->lines[line_index].chars[j]);
      }
    }

    update_current_line_progress (CONTENT_VIEW_WINDOW, 1, buffer->nlines);
    wrefresh (window);

    if (scroll == SCROLL_ENABLE)
      scroll_display (buffer, CONTENT_VIEW_WINDOW, persisent_header, header_rows);
  }
}
