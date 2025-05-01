#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>

#include <ncurses.h>

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

struct ProcInfo {
  int    pid{};
  char   state{};
  long   rss_kb{};
  std::string name;
  std::string cmdline;
};

enum class sField { PID, MEM, NAME };

static bool check(const char *s)
{
  while (*s)
    {
      if (!std::isdigit(*s++))
        return false;
    }
  return true;
}

static ProcInfo rProc(int pid)
{
  ProcInfo p;
  p.pid = pid;

  std::ifstream statf("/proc/" + std::to_string(pid) + "/stat");
  if (statf)
    {
      std::string comm;
      statf >> p.pid >> comm >> p.state;
      if (comm.size() >= 2)
        p.name = comm.substr(1, comm.size() - 2);

      long dummy;
      for (int i = 0; i < 20; ++i) statf >> dummy;
      long rss_pages;
      if (statf >> rss_pages)
        p.rss_kb = rss_pages * (long)sysconf(_SC_PAGESIZE) / 1024;
    }

  std::ifstream cmdf("/proc/" + std::to_string(pid) + "/cmdline");
  if (cmdf)
    {
      std::getline(cmdf, p.cmdline, '\0');
      std::replace(p.cmdline.begin(), p.cmdline.end(), '\0', ' ');
    }
  return p;
}

static std::vector<ProcInfo> lProcesses()
{
  std::vector<ProcInfo> procs;
  if (DIR *dir = opendir("/proc"))
    {
      while (dirent *ent = readdir(dir))
        {
          if (check(ent->d_name))
            procs.emplace_back(rProc(std::atoi(ent->d_name)));
        }
      closedir(dir);
    }
  return procs;
}

static void sProcesses(std::vector<ProcInfo>& vec, sField field)
{
  switch (field)
    {
      case sField::PID:
        std::sort(vec.begin(), vec.end(),
                   [](auto &a, auto &b) { return a.pid < b.pid; });
        break;
      case sField::MEM:
        std::sort(vec.begin(), vec.end(),
                   [](auto &a, auto &b) { return a.rss_kb > b.rss_kb; });
        break;
      case sField::NAME:
        std::sort(vec.begin(), vec.end(),
                   [](auto &a, auto &b) { return a.name < b.name; });
        break;
    }
}

static void colors()
{
  start_color();
  use_default_colors();
  init_pair(1, COLOR_CYAN,   -1);
  init_pair(2, COLOR_WHITE,  -1);
  init_pair(3, COLOR_YELLOW, -1);
  init_pair(4, COLOR_GREEN,  -1);
}

static void header(WINDOW *win, sField sort)
{
  wattron(win, COLOR_PAIR(1) | A_BOLD);
  mvwprintw(win, 0, 0, "PID    S   RSS(KB)   NAME");
  
  int title = 30;
  
  mvwprintw(win, 0, title, "CMDLINE");
  wattroff(win, COLOR_PAIR(1) | A_BOLD);

  std::string arrow = "▼";
  switch (sort)
    {
      case sField::PID:  mvwprintw(win, 0, 1,  "%s", arrow.c_str()); break;
      case sField::MEM:  mvwprintw(win, 0, 10, "%s", arrow.c_str()); break;
      case sField::NAME: mvwprintw(win, 0, 20, "%s", arrow.c_str()); break;
    }
}

static void popup(const ProcInfo& p)
{
  int maxy,maxx; getmaxyx(stdscr,maxy,maxx);
  int h = 10, w = maxx - 4;
  if (h > maxy-2) h = maxy-2;
  int y = (maxy - h) / 2;
  int x = 2;

  WINDOW *win = newwin(h, w, y, x);
  box(win, 0, 0);
  wattron(win, COLOR_PAIR(4));
  mvwprintw(win, 0, 2, " Process Infos ");
  wattroff(win, COLOR_PAIR(4));

  mvwprintw(win,2,2,"PID: %d", p.pid);
  mvwprintw(win,3,2,"State: %c", p.state);
  mvwprintw(win,4,2,"Mem RSS: %ld KB", p.rss_kb);
  mvwprintw(win,5,2,"Name: %s", p.name.c_str());
  mvwprintw(win,6,2,"Cmd: %s", p.cmdline.empty() ? "-" : p.cmdline.c_str());

  mvwprintw(win,h-2,w-18,"[Enter/q] close");
  wrefresh(win);

  int ch;
  while ((ch = getch()))
    {
      if (ch == '\n' || ch == 'q' || ch == 'Q')
          break;
    }
  delwin(win);
}

int main()
{
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, true);
  nodelay(stdscr, true);
  curs_set(0);
  if (has_colors()) colors();

  sField sf      = sField::PID;
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

      auto procs = lProcesses();
      sProcesses(procs, sf);

      int list = frameH - 2;
      int visible = list - hl - fl;
      if (visible < 1) visible = 1;

      if (selected >= (int)procs.size()) selected = (int)procs.size() - 1;
      if (selected < 0) selected = 0;

      if (selected < offset) offset = selected;
      else if (selected >= offset + visible) offset = selected - visible + 1;

      werase(view);
      header(view, sf);

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
                  sf = (sField)(((int)sf + 1) % 3);
                  break;
              case '\n':
                  if (!procs.empty()) popup(procs[selected]);
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
