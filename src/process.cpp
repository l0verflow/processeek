#include "process.h"

#include <fstream>
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

bool check(const char *s)
{
  while (*s)
    {
      if (!std::isdigit(*s++))
        return false;
    }
  return true;
}

ProcInfo readProcess(int pid)
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

std::vector<ProcInfo> listProcesses()
{
  std::vector<ProcInfo> procs;
  if (DIR *dir = opendir("/proc"))
    {
      while (dirent *ent = readdir(dir))
        {
          if (check(ent->d_name))
            procs.emplace_back(readProcess(std::atoi(ent->d_name)));
        }
      closedir(dir);
    }
  return procs;
}

void sortProcesses(std::vector<ProcInfo>& vec, SortField field)
{
  switch (field)
    {
      case SortField::PID:
        std::sort(vec.begin(), vec.end(),
                   [](auto &a, auto &b) { return a.pid < b.pid; });
        break;
      case SortField::MEM:
        std::sort(vec.begin(), vec.end(),
                   [](auto &a, auto &b) { return a.rss_kb > b.rss_kb; });
        break;
      case SortField::NAME:
        std::sort(vec.begin(), vec.end(),
                   [](auto &a, auto &b) { return a.name < b.name; });
        break;
    }
}
