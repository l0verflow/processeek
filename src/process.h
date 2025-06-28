#pragma once

#include <string>
#include <vector>

struct ProcInfo {
  int    pid{};
  char   state{};
  long   rss_kb{};
  std::string name;
  std::string cmdline;
};

enum class SortField { PID, MEM, NAME };

bool check(const char *s);
ProcInfo readProcess(int pid);
std::vector<ProcInfo> listProcesses();
void sortProcesses(std::vector<ProcInfo>& vec, SortField field);
