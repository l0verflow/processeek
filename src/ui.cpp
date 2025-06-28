#include "ui.h"

void initColors()
{
  start_color();
  use_default_colors();
  init_pair(1, COLOR_CYAN,   -1);
  init_pair(2, COLOR_WHITE,  -1);
  init_pair(3, COLOR_YELLOW, -1);
  init_pair(4, COLOR_GREEN,  -1);
}

void displayHeader(WINDOW *win, SortField sort)
{
  wattron(win, COLOR_PAIR(1) | A_BOLD);
  mvwprintw(win, 0, 0, "PID    S   RSS(KB)   NAME");
  
  int title = 30;
  
  mvwprintw(win, 0, title, "CMDLINE");
  wattroff(win, COLOR_PAIR(1) | A_BOLD);

  std::string arrow = "▼";
  switch (sort)
    {
      case SortField::PID:  mvwprintw(win, 0, 1,  "%s", arrow.c_str()); break;
      case SortField::MEM:  mvwprintw(win, 0, 10, "%s", arrow.c_str()); break;
      case SortField::NAME: mvwprintw(win, 0, 20, "%s", arrow.c_str()); break;
    }
}

void showProcessPopup(const ProcInfo& p)
{
  int maxy, maxx; getmaxyx(stdscr, maxy, maxx);
  int h = 10, w = maxx - 4;
  if (h > maxy-2) h = maxy-2;
  int y = (maxy - h) / 2;
  int x = 2;

  WINDOW *win = newwin(h, w, y, x);
  box(win, 0, 0);
  wattron(win, COLOR_PAIR(4));
  mvwprintw(win, 0, 2, " Process Infos ");
  wattroff(win, COLOR_PAIR(4));

  mvwprintw(win, 2, 2, "PID: %d", p.pid);
  mvwprintw(win, 3, 2, "State: %c", p.state);
  mvwprintw(win, 4, 2, "Mem RSS: %ld KB", p.rss_kb);
  mvwprintw(win, 5, 2, "Name: %s", p.name.c_str());
  mvwprintw(win, 6, 2, "Cmd: %s", p.cmdline.empty() ? "-" : p.cmdline.c_str());

  mvwprintw(win, h-2, w-18, "[Enter/q] close");
  wrefresh(win);

  int ch;
  while ((ch = getch()))
    {
      if (ch == '\n' || ch == 'q' || ch == 'Q')
          break;
    }
  delwin(win);
}
