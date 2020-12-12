#include "tetris_control.hpp"
#include "tetris_game.hpp"
#include "tetris_log.hpp"
#include "tetris_ui.hpp"
#include <locale.h>
#include <ncurses.h>
#include <fstream>
#include <iostream>


using namespace tetris;


std::ofstream log::out;


int main()
{
  log::out.open("tetris.log");

  ui::init_ui();

  control::play_game();

  // Close ncurses window and exit
  endwin();
  // std::cout << "Game over!" << std::endl;
  // std::cout << "Score: " << game.score << std::endl;
  return 0;
}
