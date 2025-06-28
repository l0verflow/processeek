#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <ncurses.h>

#include "process.h"
#include "ui.h"

int main()
{
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, true);
  nodelay(stdscr, true);
  curs_set(0);
  if (has_colors()) initColors();

  SortField sf   = SortField::PID;
  int selected   = 0;
  int offset     = 0;
  bool quit      = false;

  const int hl   = 1;
  const int fl   = 1;
  const int rMs  = 800;

  while (!quit)
    {
      int maxy,maxx; getmaxyx(stdscr,maxy,maxx);
      int frameW = (int)(maxx * 0.9);
      int frameH = (int)(maxy * 0.9);
      if (frameW > maxx-4) frameW = maxx-4;
      if (frameH > maxy-4) frameH = maxy-4;
      if (frameW < 40) frameW = maxx-4;
      if (frameH < 10) frameH = maxy-4;

      int startx = (maxx - frameW) / 2;
      int starty = (maxy - frameH) / 2;

      WINDOW *frame = newwin(frameH, frameW, starty, startx);
      box(frame, 0, 0);
      WINDOW *view  = derwin(frame, frameH-2, frameW-2, 1, 1);

      auto procs = listProcesses();
      sortProcesses(procs, sf);

      int list = frameH - 2;
      int visible = list - hl - fl;
      if (visible < 1) visible = 1;

      if (selected >= (int)procs.size()) selected = (int)procs.size() - 1;
      if (selected < 0) selected = 0;

      if (selected < offset) offset = selected;
      else if (selected >= offset + visible) offset = selected - visible + 1;

      werase(view);
      displayHeader(view, sf);

      for (int i = 0; i < visible && i+offset < (int)procs.size(); ++i)
        {
          const auto &p = procs[i+offset];
          bool is_sel = (i+offset == selected);

          if (is_sel) wattron(view, COLOR_PAIR(3) | A_REVERSE);
          else        wattron(view, COLOR_PAIR(2));

          mvwprintw(view, i+hl, 0,"%5d %c %8ld %-15.15s ",
                    p.pid, p.state, p.rss_kb, p.name.c_str());

          int col_cmd = 30;
          std::string cmd = p.cmdline.empty() ? "-" : p.cmdline;
          if ((int)cmd.size() > frameW - 3 - col_cmd)
              cmd = cmd.substr(0, frameW - col_cmd - 6) + "...";
          mvwprintw(view, i+hl, col_cmd, "%s", cmd.c_str());

          if (is_sel) wattroff(view, COLOR_PAIR(3) | A_REVERSE);
          else        wattroff(view, COLOR_PAIR(2));
        }

      wattron(view, A_DIM);
      mvwprintw(view, frameH-2 -1, 0,
                "PgUp/PgDn scroll  s change order Enter infos  q quit");
      wattroff(view, A_DIM);

      wnoutrefresh(frame);
      wnoutrefresh(view);
      doupdate();

      int ch = getch();
      if (ch == ERR)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(rMs));
        }
      else
        {
          switch (ch) {
              case 'q': case 'Q':
                  quit = true; break;
              case KEY_UP:
                  selected--; break;
              case KEY_DOWN:
                  selected++; break;
              case KEY_PPAGE:
                  selected -= visible; break;
              case KEY_NPAGE:
                  selected += visible; break;
              case 's': case 'S':
                  sf = SortField(((int)sf + 1) % 3);
                  break;
              case '\n':
                  if (!procs.empty()) showProcessPopup(procs[selected]);
                  break;
              default:
                  break;
            }
        }

      delwin(view);
      delwin(frame);
    }

  endwin();
  return 0;
}
