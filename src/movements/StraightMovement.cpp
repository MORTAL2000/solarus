/*
 * Copyright (C) 2006-2016 Christopho, Solarus - http://www.solarus-games.org
 *
 * Solarus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Solarus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "solarus/movements/StraightMovement.h"
#include "solarus/entities/Entity.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lowlevel/System.h"
#include "solarus/lowlevel/Geometry.h"
#include "solarus/lowlevel/Debug.h"
#include <cmath>

namespace Solarus {

/**
 * \brief Constructor.
 * \param ignore_obstacles true to ignore obstacles of the map
 * (used only when there is a map and the movement is attached to an entity of this map)
 * \param smooth true to adjust the trajectory if an obstacle is reached
 * (only when ignore_obstacles is false)
 */
StraightMovement::StraightMovement(bool ignore_obstacles, bool smooth):
  Movement(ignore_obstacles),
  angle(0),
  x_speed(0),
  y_speed(0),
  next_move_date_x(System::now()),
  next_move_date_y(System::now()),
  x_delay(0),
  y_delay(0),
  x_move(0),
  y_move(0),
  max_distance(0),
  finished(false),
  smooth(smooth) {

}

/**
 * \brief Notifies this movement that the object it controls has changed.
 */
void StraightMovement::notify_object_controlled() {

  Movement::notify_object_controlled();
  initial_xy = get_xy();
}

/**
 * \brief Returns the x speed of the object.
 * \return the x speed of the entity, between -100 and 100
 */
double StraightMovement::get_x_speed() const {
  return x_speed;
}

/**
 * \brief Returns the y speed of the object.
 * \return the y speed of the entity, between -100 and 100
 */
double StraightMovement::get_y_speed() const {
  return y_speed;
}

/**
 * \brief Returns the total speed of the object.
 *
 * The speed is calculated as sqrt(x_speed^2 + y_speed^2).
 *
 * \return the speed in pixels per second
 */
double StraightMovement::get_speed() const {
  return std::sqrt(x_speed * x_speed + y_speed * y_speed);
}

/**
 * \brief Sets the x speed.
 * \param x_speed the x speed of the object in pixels per second
 */
void StraightMovement::set_x_speed(double x_speed) {

  if (std::abs(x_speed) <= 1E-6) {
    x_speed = 0;
  }

  this->x_speed = x_speed;
  uint32_t now = System::now();

  // compute x_delay, x_move and next_move_date_x
  if (x_speed == 0) {
    x_move = 0;
  }
  else {
    if (x_speed > 0) {
      x_delay = (uint32_t) (1000 / x_speed);
      x_move = 1;
    }
    else {
      x_delay = (uint32_t) (1000 / (-x_speed));
      x_move = -1;
    }
    set_next_move_date_x(now + x_delay);
  }
  angle = Geometry::get_angle(0.0, 0.0, x_speed * 100.0, y_speed * 100.0);
  initial_xy = get_xy();
  finished = false;

  notify_movement_changed();
}

/**
 * \brief Sets the y speed.
 * \param y_speed the y speed of the object in pixels per second
 */
void StraightMovement::set_y_speed(double y_speed) {

  if (std::abs(y_speed) <= 1E-6) {
    y_speed = 0;
  }

  this->y_speed = y_speed;
  uint32_t now = System::now();

  // compute y_delay, y_move and next_move_date_y
  if (y_speed == 0) {
    y_move = 0;
  }
  else {
    if (y_speed > 0) {
      y_delay = (uint32_t) (1000 / y_speed);
      y_move = 1;
    }
    else {
      y_delay = (uint32_t) (1000 / (-y_speed));
      y_move = -1;
    }
    set_next_move_date_y(now + y_delay);
  }
  angle = Geometry::get_angle(0.0, 0.0, x_speed * 100.0, y_speed * 100.0);
  initial_xy = get_xy();
  finished = false;

  notify_movement_changed();
}

/**
 * \brief Changes the speed, keeping the same direction of the movement.
 *
 * x_speed and y_speed are recomputed so that the movement direction is unchanged.
 *
 * \param speed the new speed
 */
void StraightMovement::set_speed(double speed) {

  // compute the new speed vector
  double old_angle = this->angle;
  set_x_speed(speed * std::cos(old_angle));
  set_y_speed(-speed * std::sin(old_angle));
  this->angle = old_angle;

  notify_movement_changed();
}

/**
 * \brief Returns whether the speed is not zero.
 * \return true if the object is moving, false otherwise
 */
bool StraightMovement::is_started() const {
  return x_speed != 0 || y_speed != 0;
}

/**
 * \brief Sets the speed to zero.
 */
void StraightMovement::stop() {

  double old_angle = this->angle;
  set_x_speed(0);
  set_y_speed(0);
  x_move = 0;
  y_move = 0;
  this->angle = old_angle;

  notify_movement_changed();
}

/**
 * \brief Sets the date of the next change of the x coordinate.
 * \param next_move_date_x the date in milliseconds
 */
void StraightMovement::set_next_move_date_x(uint32_t next_move_date_x) {

  if (is_suspended()) {
    uint32_t delay = next_move_date_x - System::now();
    this->next_move_date_x = get_when_suspended() + delay;
  }
  else {
    this->next_move_date_x = next_move_date_x;
  }
}

/**
 * \brief Sets the date of the next change of the y coordinate.
 * \param next_move_date_y the date in milliseconds
 */
void StraightMovement::set_next_move_date_y(uint32_t next_move_date_y) {

  if (is_suspended()) {
    uint32_t delay = next_move_date_y - System::now();
    this->next_move_date_y = get_when_suspended() + delay;
  }
  else {
    this->next_move_date_y = next_move_date_y;
  }
}

/**
 * \brief Computes and returns the direction of the speed vector.
 * \return the current angle of the speed vector in degrees
 */
double StraightMovement::get_angle() const {
  return angle;
}

/**
 * \brief Changes the direction of the movement vector, keeping the same speed.
 *
 * x_speed and y_speed are recomputed so that the total speed is unchanged.
 *
 * \param angle the new movement direction in radians
 */
void StraightMovement::set_angle(double angle) {

  if (!is_stopped()) {
    double speed = get_speed();
    set_x_speed(speed * std::cos(angle));
    set_y_speed(-speed * std::sin(angle));
  }
  this->angle = angle;

  notify_movement_changed();
}

/**
 * \brief Returns the distance after which the movement stops.
 * \return the maximum distance in pixels (0 means no limit)
 */
int StraightMovement::get_max_distance() const {
  return max_distance;
}

/**
 * \brief Sets the distance after which the movement stops.
 * \param max_distance the maximum distance in pixels (0 means no limit)
 */
void StraightMovement::set_max_distance(int max_distance) {
  this->max_distance = max_distance;
}

/**
 * \brief Returns whether the movement is finished.
 *
 * This functions returns true when max_distance is reached.
 *
 * \return true if the movement is finished
 */
bool StraightMovement::is_finished() const {
  return finished;
}

/**
 * \brief Stops the movement and marks it finished.
 */
void StraightMovement::set_finished() {

  stop();
  this->finished = true;
}

/**
 * \brief Returns the direction a sprite controlled by this movement should take.
 * \return the direction to use to display the object controlled by this movement (0 to 3)
 */
int StraightMovement::get_displayed_direction4() const {

  int direction = (Geometry::radians_to_degrees(angle) + 45 + 360) / 90;
  return direction % 4;
}

/**
 * \brief Returns true if the object is about to try to move.
 *
 * This function returns true if x_move is not equal to zero
 * and next_move_date_x is past, or the same thing for y.
 *
 * \return true if the entity is about to try to move
 */
bool StraightMovement::has_to_move_now() const {

  uint32_t now = System::now();

  return (x_move != 0 && now >= next_move_date_x)
    || (y_move != 0 && now >= next_move_date_y);
}

/**
 * \brief Suspends or resumes the movement.
 *
 * This function is called by the entity when the game is suspended or resumed.
 *
 * \param suspended true to suspend the movement, false to resume it
 */
void StraightMovement::set_suspended(bool suspended) {

  Movement::set_suspended(suspended);

  if (!suspended) {

    // recalculate the next move date
    if (get_when_suspended() != 0) {
      uint32_t diff = System::now() - get_when_suspended();
      next_move_date_x += diff;
      next_move_date_y += diff;
    }
  }
}

/**
 * \brief Returns whether the movement adjusts its trajectory when
 * an obstacle is reached.
 * \return true if the movement is smooth
 */
bool StraightMovement::is_smooth() const {
  return this->smooth;
}

/**
 * \brief Sets whether the movement adjusts its trajectory when
 * an obstacle is reached.
 * \param smooth true if the movement is smooth
 */
void StraightMovement::set_smooth(bool smooth) {
  this->smooth = smooth;
}

/**
 * \brief Updates the x position of the entity if it wants to move
 * (smooth version).
 */
void StraightMovement::update_smooth_x() {

  if (x_move != 0) {  // The entity wants to move on x.

    // By default, next_move_date_x will be incremented by x_delay,
    // unless we modify below the movement in such a way that the
    // x speed needs to be fixed.
    uint32_t next_move_date_x_increment = x_delay;

    if (!test_collision_with_obstacles(x_move, 0)) {

      translate_x(x_move);  // Make the move.

      if (y_move != 0 && test_collision_with_obstacles(0, y_move)) {
        // If there is also a y move, and if this y move is illegal,
        // we still allow the x move and we give it all the speed.
        next_move_date_x_increment = (int) (1000.0 / get_speed());
      }
    }
    else {
      if (y_move == 0) {
        // The move on x is not possible and there is no y move:
        // let's try to add a move on y to make a diagonal move,
        // but only if the wall is really diagonal: otherwise, the hero
        // could bypass sensors.

        if (!test_collision_with_obstacles(x_move, 1)    // Can move diagonally and:
            && (test_collision_with_obstacles(0, -1) ||  // the wall is really diagonal
                test_collision_with_obstacles(0, 1))     // or we don't have a choice anyway.
        ) {
          translate_xy(x_move, 1);
          next_move_date_x_increment = (int) (x_delay * Geometry::SQRT_2);  // Fix the speed.
        }
        else if (!test_collision_with_obstacles(x_move, -1)
            && (test_collision_with_obstacles(0, 1) ||
                test_collision_with_obstacles(0, -1))
        ) {
          translate_xy(x_move, -1);
          next_move_date_x_increment = (int) (x_delay * Geometry::SQRT_2);
        }
        else {

          // The diagonal moves didn't work either.
          // So we look for a place (up to 8 pixels up and down)
          // where the required move would be allowed.
          // If we find a such place, then we move towards this place.

          bool moved = false;
          for (int i = 1; i <= 8 && !moved; i++) {

            if (!test_collision_with_obstacles(x_move, i) && !test_collision_with_obstacles(0, 1)) {
              translate_y(1);
              moved = true;
            }
            else if (!test_collision_with_obstacles(x_move, -i) && !test_collision_with_obstacles(0, -1)) {
              translate_y(-1);
              moved = true;
            }
          }
        }
      }
      else {
        // The move on x is not possible, but there is also a vertical move.
        if (!test_collision_with_obstacles(0, y_move)) {
          // Do the vertical move right now, don't wait uselessly.
          update_y();
        }
        else {
          // The x move is not possible and neither is the y move.
          // Last chance: do both x and y moves in one step.
          // This case is only necessary in narrow diagonal passages.
          // We do it as a last resort, because we want separate x and y
          // steps whenever possible: otherwise, the hero could bypass sensors.
          if (!test_collision_with_obstacles(x_move, y_move)) {
            translate_xy(x_move, y_move);
            next_move_date_y += y_delay;  // Delay the next update_smooth_y() since we just replaced it.
          }
        }
      }
    }
    next_move_date_x += next_move_date_x_increment;
  }
}

/**
 * \brief Updates the y position of the entity if it wants to move
 * (smooth version).
 */
void StraightMovement::update_smooth_y() {

  if (y_move != 0) {  // The entity wants to move on y.

    // By default, next_move_date_y will be incremented by y_delay,
    // unless we modify the movement in such a way that the
    // y speed needs to be fixed.
    uint32_t next_move_date_y_increment = y_delay;

    if (!test_collision_with_obstacles(0, y_move)) {

      translate_y(y_move);  // Make the move.

      if (x_move != 0 && test_collision_with_obstacles(x_move, 0)) {
        // If there is also an x move, and if this x move is illegal,
        // we still allow the y move and we give it all the speed.
        next_move_date_y_increment = (int) (1000.0 / get_speed());
      }
    }
    else {
      if (x_move == 0) {
        // The move on y is not possible and there is no x move:
        // let's try to add a move on x to make a diagonal move,
        // but only if the wall is really diagonal: otherwise, the hero
        // could bypass sensors.

        if (!test_collision_with_obstacles(1, y_move)    // Can move diagonally and:
            && (test_collision_with_obstacles(-1, 0) ||  // the wall is really diagonal
                test_collision_with_obstacles(1, 0))     // or we don't have a choice anyway.
        ) {
          translate_xy(1, y_move);
          next_move_date_y_increment = (int) (y_delay * Geometry::SQRT_2);  // Fix the speed.
        }
        else if (!test_collision_with_obstacles(-1, y_move)
            && (test_collision_with_obstacles(1, 0) ||
                test_collision_with_obstacles(-1, 0))
        ) {
          translate_xy(-1, y_move);
          next_move_date_y_increment = (int) (y_delay * Geometry::SQRT_2);
        }
        else {
          // The diagonal moves didn't work either.
          // So we look for a place (up to 8 pixels on the left and on the right)
          // where the required move would be allowed.
          // If we find a such place, then we move towards this place.

          bool moved = false;
          for (int i = 1; i <= 8 && !moved; i++) {

            if (!test_collision_with_obstacles(i, y_move) && !test_collision_with_obstacles(1, 0)) {
              translate_x(1);
              moved = true;
            }
            else if (!test_collision_with_obstacles(-i, y_move) && !test_collision_with_obstacles(-1, 0)) {
              translate_x(-1);
              moved = true;
            }
          }
        }
      }
      else {
        // The move on y is not possible, but there is also a horizontal move.
        if (!test_collision_with_obstacles(x_move, 0)) {
          // Do the horizontal move right now, don't wait uselessly.
          update_x();
        }
        else {
          // The y move is not possible and neither is the x move.
          // Last chance: do both x and y moves in one step.
          // This case is only necessary in narrow diagonal passages.
          // We do it as a last resort, because we want separate x and y
          // steps whenever possible: otherwise, the hero could bypass sensors.
          if (!test_collision_with_obstacles(x_move, y_move)) {
            translate_xy(x_move, y_move);
            next_move_date_x += x_delay;  // Delay the next update_smooth_x() since we just replaced it.
          }
        }
      }
    }
    next_move_date_y += next_move_date_y_increment;
  }
}

/**
 * \brief Updates the x position of the entity if it wants to move
 * (non-smooth version).
 */
void StraightMovement::update_non_smooth_x() {

  if (x_move != 0) {

    // make the move only if there is no collision
    uint32_t now = System::now();
    int dy = now >= next_move_date_y ? y_move : 0;
    if (!test_collision_with_obstacles(x_move, dy)) {
      translate_x(x_move);
    }
    next_move_date_x += x_delay;
  }
}

/**
 * \brief Updates the y position of the entity if it wants to move
 * (non-smooth version).
 */
void StraightMovement::update_non_smooth_y() {

  if (y_move != 0) { // if it's time to try a move

    // make the move only if there is no collision
    uint32_t now = System::now();
    int dx = now >= next_move_date_x ? x_move : 0;
    if (!test_collision_with_obstacles(dx, y_move)) {
      translate_y(y_move);
    }
    next_move_date_y += y_delay;
  }
}

/**
 * \brief Updates the x position of the entity if it wants to move.
 */
void StraightMovement::update_x() {

  if (is_smooth()) {
    update_smooth_x();
  }
  else {
    update_non_smooth_x();
  }
}

/**
 * \brief Updates the y position of the entity if it wants to move.
 */
void StraightMovement::update_y() {

  if (is_smooth()) {
    update_smooth_y();
  }
  else {
    update_non_smooth_y();
  }
}

/**
 * \brief Updates the position of the object controlled by this movement.
 *
 * This function is called repeatedly.
 */
void StraightMovement::update() {

  if (!is_suspended()) {
    uint32_t now = System::now();

    bool x_move_now = x_move != 0 && now >= next_move_date_x;
    bool y_move_now = y_move != 0 && now >= next_move_date_y;

    while (x_move_now || y_move_now) { // while it's time to move

      // save the current coordinates
      Point old_xy = get_xy();

      if (x_move_now) {
        // it's time to make an x move

        if (y_move_now) {
          // but it's also time to make a y move

          if (next_move_date_x <= next_move_date_y) {
            // x move first
            update_x();
            if (now >= next_move_date_y) {
              update_y();
            }
          }
          else {
            // y move first
            update_y();
            if (now >= next_move_date_x) {
              update_x();
            }
          }
        }
        else {
          update_x();
        }
      }
      else {
        update_y();
      }

      if (!is_suspended() && get_entity() != nullptr && !finished) {

        // the movement was successful if the entity's coordinates have changed
        // and the movement was not stopped
        bool success = (get_xy() != old_xy)
            && (x_move != 0 || y_move != 0);

        if (!success) {
          notify_obstacle_reached();
        }
      }

      now = System::now();

      if (!finished && max_distance != 0
          && Geometry::get_distance(initial_xy, get_xy()) >= max_distance) {
        set_finished();
      }
      else {
        x_move_now = x_move != 0 && now >= next_move_date_x;
        y_move_now = y_move != 0 && now >= next_move_date_y;
      }
    }
  }

  // Do this at last so that Movement::update() knows whether we are finished.
  Movement::update();
}

/**
 * \brief Returns the name identifying this type in Lua.
 * \return the name identifying this type in Lua
 */
const std::string& StraightMovement::get_lua_type_name() const {
  return LuaContext::movement_straight_module_name;
}

}

