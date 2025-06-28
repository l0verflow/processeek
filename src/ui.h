#pragma once

#include <ncurses.h>
#include "process.h"

void initColors();
void displayHeader(WINDOW *win, SortField sort);
void showProcessPopup(const ProcInfo& p);
