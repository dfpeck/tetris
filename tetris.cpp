#include "tetris.hpp"
#include "tetris_log.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <random>
#include <stdexcept>


using namespace tetris;


/* Point Class Methods */

Point Point::operator+(const Point& right_op) const
{
  return Point(this->row + right_op.row, this->col + right_op.col);
}

Point& Point::operator+=(const Point& right_op)
{
  this->row += right_op.row;
  this->col += right_op.col;
  return *this;
}

Point Point::operator-(const Point& right_op) const
{
  return Point(this->row - right_op.row, this->col - right_op.col);
}

Point& Point::operator-=(const Point& right_op)
{
  this->row -= right_op.row;
  this->col -= right_op.col;
  return *this;
}


/* Playfield Class Methods */

std::array<TetriminoType, 10>& Playfield::operator[](short index)
{
  return grid[index];
}

const std::array<TetriminoType, 10>& Playfield::operator[](short index) const
{
  return grid[index];
}

TetriminoType& Playfield::operator[](const Point& point)
{
  return grid[point.row][point.col];
}

TetriminoType Playfield::operator[](const Point& point) const
{
  return grid[point.row][point.col];
}


/* Tetrimino Class Methods */

Tetrimino::Tetrimino(TetriminoType type_init)
  : type(type_init),
    facing(TetriminoFacing::NORTH)
{
  switch(type)
  {
    case TetriminoType::O:
      points = {
        Point(18, 4),
        Point(18, 5),
        Point(19, 4),
        Point(19, 5),
      };
      pivot = Point(19, 4);
      break;

    case TetriminoType::I:
      points = {
        Point(19, 3),
        Point(19, 4),
        Point(19, 5),
        Point(19, 6),
      };
      pivot = Point(19, 4);
      break;

    case TetriminoType::T:
      points = {
        Point(18, 4),
        Point(19, 3),
        Point(19, 4),
        Point(19, 5),
      };
      pivot = Point(19, 4);
      break;

    case TetriminoType::L:
      points = {
        Point(18, 5),
        Point(19, 3),
        Point(19, 4),
        Point(19, 5),
      };
      pivot = Point(19, 4);
      break;

    case TetriminoType::J:
      points = {
        Point(18, 3),
        Point(19, 3),
        Point(19, 4),
        Point(19, 5),
      };
      pivot = Point(19, 4);
      break;

    case TetriminoType::S:
      points = {
        Point(18, 4),
        Point(18, 5),
        Point(19, 3),
        Point(19, 4),
      };
      pivot = Point(19, 4);
      break;

    case TetriminoType::Z:
      points = {
        Point(18, 3),
        Point(18, 4),
        Point(19, 4),
        Point(19, 5),
      };
      pivot = Point(19, 4);
      break;
  }
}

bool Tetrimino::translate(const Point& delta, const Playfield& playfield)
{
  Point new_pivot = pivot + delta;
  std::array<Point, 4> new_points = points;
  for (Point& p : new_points)
  {
    p += delta;
    if (check_collision(p, playfield) != CollisionResult::NONE)
      return false;
  }

  pivot = new_pivot;
  points = new_points;
  return true;
}

bool Tetrimino::rotate_ccw(const Playfield& playfield)
{
  if (type == TetriminoType::O)
    return true;

  // Determine new facing
  TetriminoFacing new_facing;
  switch(facing)
  {
    case TetriminoFacing::NORTH:
      new_facing = TetriminoFacing::WEST;
      break;

    case TetriminoFacing::EAST:
      new_facing = TetriminoFacing::NORTH;
      break;

    case TetriminoFacing::SOUTH:
      new_facing = TetriminoFacing::EAST;
      break;

    case TetriminoFacing::WEST:
      new_facing = TetriminoFacing::SOUTH;
      break;
  }

  // Rotate points
  std::array<Point, 4> new_points = points;
  for (Point& p : new_points)
  {
    p -= pivot;
    p = Point(-p.col, p.row);
    p += pivot;
  }

  // Special handling for I tetrimino
  if (type == TetriminoType::I)
  {
    Point i_offset = Point(1, 0);
    for (Point& p : new_points)
      p += i_offset;
  }

  // Process super rotation system
  Point offset;
  bool clear = process_srs(type, new_points, playfield, facing, new_facing, offset);

  // Update tetrimino
  if (clear)
  {
    for (Point& p : new_points)
      p += offset;

    points = new_points;
    facing = new_facing;
    pivot += offset;
  }

  return clear;
}

bool Tetrimino::rotate_cw(const Playfield& playfield)
{
  if (type == TetriminoType::O)
    return true;

  // Determine new facing
  TetriminoFacing new_facing;
  switch(facing)
  {
    case TetriminoFacing::NORTH:
      new_facing = TetriminoFacing::EAST;
      break;

    case TetriminoFacing::EAST:
      new_facing = TetriminoFacing::SOUTH;
      break;

    case TetriminoFacing::SOUTH:
      new_facing = TetriminoFacing::WEST;
      break;

    case TetriminoFacing::WEST:
      new_facing = TetriminoFacing::NORTH;
      break;
  }

  // Update points
  std::array<Point, 4> new_points = points;
  for (Point& p : new_points)
  {
    p -= pivot;
    p = Point(p.col, -p.row);
    p += pivot;
  }

  // Special handling for I tetrimino
  if (type == TetriminoType::I)
  {
    Point i_offset = Point(0, 1);
    for (Point& p : new_points)
      p += i_offset;
  }

  // Process super rotation system
  Point offset;
  bool clear = process_srs(type, new_points, playfield, facing, new_facing, offset);

  // Update tetrimino
  if (clear)
  {
    for (Point& p : new_points)
      p += offset;

    points = new_points;
    facing = new_facing;
    pivot += offset;
  }

  return clear;
}

bool Tetrimino::hard_drop(const Playfield& playfield)
{
  short fall_distance = get_landing(playfield).pivot.row - pivot.row;
  return translate(Point(fall_distance, 0), playfield);
}

bool Tetrimino::is_landed(const Playfield& playfield) const
{
  for (const Point& p : points)
  {
    if (check_collision(p + Point(1, 0), playfield))
      return true;
  }

  return false;
}

Tetrimino Tetrimino::get_landing(const Playfield& playfield) const
{
  short distance_to_landing = 41;

  for (short row=0; row<40; row++)
  {
    for (const Point& p : points)
    {
      if (playfield[row][p.col] != TetriminoType::NONE
          && (row - p.row - 1 < distance_to_landing) && (row - p.row - 1 >= 0))
        distance_to_landing = row - p.row - 1;
    }
  }

  for (const Point& p : points)
  {
    if (39 - p.row < distance_to_landing)
      distance_to_landing = 39 - p.row;
  }

  Tetrimino landing = *this;
  landing.translate(Point(distance_to_landing, 0), playfield);
  return landing;
}


/* Bag Class Methods */

Bag::Bag()
{
  std::random_device rd;
  random_generator = std::default_random_engine(rd());
  extend_queue();
}

Tetrimino Bag::pop()
{
  Tetrimino next = tetrimino_queue.front();
  tetrimino_queue.pop();
  if (tetrimino_queue.size() < 7)
    extend_queue();

  return next;
}

void Bag::extend_queue()
{
  std::array<Tetrimino, 7> tetriminos {
    Tetrimino(TetriminoType::O),
    Tetrimino(TetriminoType::I),
    Tetrimino(TetriminoType::T),
    Tetrimino(TetriminoType::L),
    Tetrimino(TetriminoType::J),
    Tetrimino(TetriminoType::S),
    Tetrimino(TetriminoType::Z),
  };

  std::shuffle(tetriminos.begin(), tetriminos.end(), random_generator);

  for (Tetrimino t : tetriminos)
    tetrimino_queue.push(t);
}


/* Game Class Methods */

bool Game::try_command(Command command)
{
  switch(command)
  {
    case Command::DO_NOTHING:
      break;

    case Command::SHIFT_LEFT:
      active_tetrimino.translate(Point(0, -1), playfield);
      break;

    case Command::SHIFT_RIGHT:
      active_tetrimino.translate(Point(0, 1), playfield);
      break;

    case Command::ROTATE_CCW:
      active_tetrimino.rotate_ccw(playfield);
      break;

    case Command::ROTATE_CW:
      active_tetrimino.rotate_cw(playfield);
      break;

    case Command::SOFT_DROP:
      active_tetrimino.translate(Point(1, 0), playfield);
      break;

    case Command::HARD_DROP:
      active_tetrimino.hard_drop(playfield);
      break;
  }

  return true;
}

void Game::lock_active_tetrimino()
{
  for (const Point& p : active_tetrimino.points)
    playfield[p] = active_tetrimino.type;
}

void Game::clear_rows()
{
  for (int row=39; row>0; row--)
  {
    // Check if row is full
    bool row_full(true);
    for (short col=0; col<10; col++)
    {
      if (playfield[row][col] == TetriminoType::NONE)
      {
        row_full = false;
        break;
      }
    }

    // If row is full, clear it and lower upper rows
    if (row_full)
    {
      for (int rowc=row; rowc>0; rowc--)
      {
        for (int colc=0; colc<10; colc++)
          playfield[rowc][colc] = playfield[rowc-1][colc];
      }

      // Since the previously above row has been moved into the current row, that row
      // would be skipped were row allowed to decrement in the next iteration, so
      // increment row to counteract.
      ++row;
    }
  }
}

void Game::draw_new_tetrimino()
{
  active_tetrimino = bag.pop();
}

bool Game::is_game_over()
{
  return false; // TODO-DELETE TESTING
  for (const Point& p : active_tetrimino.points)
    if (check_collision(p, playfield))
      return true;

  return false;
}

std::chrono::duration<float> Game::get_drop_interval()
{
  return std::chrono::duration<float>(pow(0.8 - ((level-1) * 0.0007), level-1));
}


/* Free Functions */

short tetris::check_collision(const Point& point, const Playfield& playfield)
{
  short result = CollisionResult::NONE;

  if (point.row > 39)
    result |= CollisionResult::FLOOR;

  if (point.col < 0 || point.col > 9)
    result |= CollisionResult::WALL;

  if (playfield[point.row][point.col] != TetriminoType::NONE)
    result |= CollisionResult::MINO;

  return result;
}

short tetris::check_collision(const std::array<Point, 4>& points, const Playfield& playfield)
{
  short result = CollisionResult::NONE;
  for (const Point& p : points)
    result |= check_collision(p, playfield);

  return result;
}

bool tetris::process_srs(TetriminoType type,
                         const std::array<Point, 4>& points,
                         const Playfield playfield,
                         TetriminoFacing facing_before,
                         TetriminoFacing facing_after,
                         Point& offset)
{
  tetris_log::out << "Rotating "
                  << (short)facing_before
                  << " -> "
                  << (short)facing_after
                  << std::endl;

  if (!check_collision(points, playfield))
  {
    // New points already free of collision
    tetris_log::out << "SRS not needed" << std::endl;
    return true;
  }
  else
  {
    tetris_log::out << "Processing SRS" << std::endl;

    for (const Point& p : points)
    {
      tetris_log::out << "Point(" << p.row << "," << p.col << ")" << std::endl;
    }
    // New points not free of collision, process super rotation system
    for (short i=0; i<4; i++)
    {
      // Calculate and apply SRS offset
      offset = calculate_srs_offset(i, type, facing_before, facing_after);
      tetris_log::out << "Checking SRS offset " << i+1 << ": "
                      << offset.row << "," << offset.col
                      << std::endl;
      std::array<Point, 4> offset_points = points;
      for (Point& p : offset_points)
      {
        p += offset;
        tetris_log::out << "Point(" << p.row << "," << p.col << ")" << std::endl;
      }

      // Check resulting points for collisions
      if (!check_collision(offset_points, playfield))
      {
        tetris_log::out << "Using SRS offset " << i+1 << ": "
                        << offset.row << "," << offset.col
                        << std::endl;
        return true;
      }
    }
  }

  tetris_log::out << "No suitable SRS offset found" << std::endl;

  return false;
}

Point tetris::get_srs_offset_value(TetriminoType type, TetriminoFacing facing, short point_index)
{
  switch(type)
  {
    case tetris::TetriminoType::I:
      return I_SRS_OFFSET_VALUES.at(facing)[point_index];
      break;

    case tetris::TetriminoType::T:
    case tetris::TetriminoType::L:
    case tetris::TetriminoType::J:
    case tetris::TetriminoType::S:
    case tetris::TetriminoType::Z:
      return STANDARD_SRS_OFFSET_VALUES.at(facing)[point_index];
      break;

    default:
      throw std::invalid_argument("Invalid tetrimino type for SRS");
      break;
  }
}

Point tetris::calculate_srs_offset(short point_index,
                                   TetriminoType type,
                                   TetriminoFacing facing_before,
                                   TetriminoFacing facing_after)
{
  return (get_srs_offset_value(type, facing_before, point_index)
          - get_srs_offset_value(type, facing_after, point_index));
}
