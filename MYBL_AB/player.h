#ifndef PLAYER_H
#define PLAYER_H

#include <Arduino.h>
#include "globals.h"
//#include "levels.h"
#include "vec2.h"

#define FIXED_POINT 5
#define PLAYER_SPEED_WALKING 1 << FIXED_POINT
#define PLAYER_SPEED_AIR 2
#define PLAYER_PARTICLES 3
#define PLAYER_JUMP_VELOCITY (2 << FIXED_POINT) - 2
#define GRAVITY 3
#define FRICTION 1 // for horizontal speed
#define MAX_XSPEED PLAYER_SPEED_WALKING + 1
#define MAX_YSPEED 3 * (1 << FIXED_POINT)
#define CAMERA_OFFSET 5

extern bool gridGetSolid(int8_t x, int8_t y);

struct Camera
{
  // x and y are 9.6 signed fixed vec2 values
  vec2 pos;
  vec2 offset;
};

struct Players
{
  public:
    // x and y are 9.6 signed fixed vec2 values
    vec2 pos;
    vec2 actualpos;
    vec2 speed;
    boolean isActive;
    boolean isImune;
    boolean direction;
    boolean isWalking;
    boolean isJumping;
    boolean isLanding;
    boolean isFloating;
    boolean isBalloon;
    
    boolean jumpLetGo;
    boolean isSucking;
    byte imuneTimer;
    byte jumpTimer;
    byte HP;
    byte frame;
    byte balloons;
    byte balloonOffset;
    vec2 particles[PLAYER_PARTICLES];
};

Players kid;
Camera cam;

void setKid()
{
  kid.pos.x = 0;
  kid.pos.y = 0;
  kid.actualpos.x = 128;
  kid.actualpos.y = 0;
  kid.speed.x = 0;
  kid.speed.y = 0;
  kid.isActive = true;
  kid.HP = 4;
  kid.isImune = true;
  kid.imuneTimer = 0;
  kid.jumpTimer = 0;
  kid.direction = FACING_RIGHT;
  kid.isWalking = false;
  kid.isJumping = false;
  kid.isLanding = false;
  kid.isFloating = false;
  kid.isBalloon = false;
  kid.jumpLetGo = true;
  kid.isSucking = false;
  kid.balloons = 3;
  kid.balloonOffset = 0;
  for (byte i = 0; i < PLAYER_PARTICLES; ++i)
    kid.particles[i] = vec2(random(16), random(16));
}

void checkKid()
{
  if (kid.isImune)
  {
    if (arduboy.everyXFrames(3)) kid.isActive = !kid.isActive;
    kid.imuneTimer++;
    if (kid.imuneTimer > 60)
    {
      kid.imuneTimer = 0;
      kid.isImune = false;
      kid.isActive = true;
    }
  }
  if (kid.HP < 2) gameState = STATE_GAME_OVER;
  if (arduboy.everyXFrames(8) && (kid.isWalking || kid.isSucking))
  {
    ++kid.frame;
    if (kid.frame % 2 == 0)
      arduboy.audio.tone(150, 20);
  }
  if (kid.frame > 3 || (!kid.isWalking && !kid.isSucking)) kid.frame = 0;

  // Kid is moving up
  if (kid.speed.y > 0 && !kid.isBalloon)
  {
    kid.isJumping = true;
    kid.isLanding = false;
    if (!kid.jumpLetGo && kid.jumpTimer > 0)
    {
      kid.speed.y += GRAVITY;
      kid.jumpTimer--;
    }
  }

  // Kid is moving down
  if (kid.speed.y < 0)
  {
    kid.isJumping = false;
    kid.isLanding = true;
  }

  // Update position---
  // -Solid checking
  int tx = (kid.pos.x + 6) >> 4;
  int ty = (kid.pos.y + 8) >> 4;
  boolean solidbelow = gridGetSolid(tx, (kid.pos.y + 16) >> 4);
  boolean solidabove = gridGetSolid(tx, (kid.pos.y - 1) >> 4);
  boolean solidleft = gridGetSolid((kid.pos.x - 1) >> 4, ty);
  boolean solidright = gridGetSolid((kid.pos.x + 13) >> 4, ty);
  int tx2 = (((kid.actualpos.x + kid.speed.x) >> FIXED_POINT) - 1 + (kid.speed.x > 0) * 14) >> 4;
  boolean solidH = gridGetSolid(tx2, (kid.pos.y + 2) >> 4)
                   || gridGetSolid(tx2, (kid.pos.y + 13) >> 4);
  int ty2 = (((kid.actualpos.y - kid.speed.y) >> FIXED_POINT) + (kid.speed.y < 0) * 17) >> 4;
  boolean solidV = gridGetSolid((kid.pos.x + 2) >> 4, ty2)
                   || gridGetSolid((kid.pos.x + 10) >> 4, ty2);

  // Gravity
  if (kid.speed.y > 0 || !solidbelow)
  {
    //kid.speed.y += GRAVITY;
    kid.speed.y = (kid.speed.y > -MAX_YSPEED) ? kid.speed.y - GRAVITY : -MAX_YSPEED;
    if (kid.isBalloon)
    {
      //cam.offset.y += 3;
      if (kid.balloonOffset > 0)
        kid.balloonOffset -= 2;
      else
        kid.speed.y = max(-((8 / kid.balloons) >> 1), kid.speed.y);
    }
  }

  // Friction
  /*if (abs(kid.speed.x) > FRICTION)
  {
    if (arduboy.everyXFrames(3))
    {
      if (kid.speed.x > 0) kid.speed.x -= FRICTION;
      else if (kid.speed.x < 0) kid.speed.x += FRICTION;
    }
  }
  else
  {
    kid.speed.x = 0;
  }*/

  // Kid on ground
  if (kid.speed.y <= 0 && (solidV || solidbelow))
  {
    if (kid.isLanding) arduboy.audio.tone(80, 30);
    kid.speed.y = 0;
    kid.speed.x = 0;
    kid.isLanding = false;
    kid.isJumping = false;
    kid.isBalloon = false;
    int8_t ysnap = (((kid.actualpos.y >> FIXED_POINT) + 12) >> 4);
    //if (!gridGetSolid(ysnap, kid.pos.x >> 4))
      kid.actualpos.y = ysnap << (FIXED_POINT + 4);

    // Fall off edge
    //if (abs(((kid.pos.x + 6) % 16) - 8) >= 4)
    if (!arduboy.pressed(RIGHT_BUTTON) && !arduboy.pressed(LEFT_BUTTON))
    {
      int8_t yy = (kid.pos.y + 16) >> 4;
      if (!gridGetSolid((kid.pos.x + 2) >> 4, yy))
        kid.actualpos.x -= FIXED_POINT << 2;
      else if (!gridGetSolid((kid.pos.x + 10) >> 4, yy))
        kid.actualpos.x += FIXED_POINT << 2;
    }
  }
  else
  {
    // Friction in air
    if (abs(kid.speed.x) > FRICTION)
    {
      if (arduboy.everyXFrames(3))
      {
        if (kid.speed.x > 0) kid.speed.x = max(0, kid.speed.x - FRICTION);//kid.speed.x -= FRICTION;
        else if (kid.speed.x < 0) kid.speed.x = min(0, kid.speed.x + FRICTION);//kid.speed.x += FRICTION;
      }
    }
    else
    {
      kid.speed.x = 0;
    }
  }

  // Move out of walls
  if (!gridGetSolid(tx, ty))
  {
    if (gridGetSolid((kid.pos.x) >> 4, ty))
      kid.actualpos.x += 8;
    else if (gridGetSolid((kid.pos.x + 11) >> 4, ty))
      kid.actualpos.x -= 8;
  }
//  if (gridGetSolid(tx, ty))
//  {
//    if (!gridGetSolid((kid.pos.x - 8) >> 4, ty))
//      kid.actualpos.x -= 8;
//    else if (!gridGetSolid((kid.pos.x + 19) >> 4, ty))
//      kid.actualpos.x += 8;
//    else if (!gridGetSolid(tx, (kid.pos.y - 8) >> 4))
//      kid.actualpos.y -= 8;
//    else if (!gridGetSolid(tx, (kid.pos.y + 25) >> 4))
//      kid.actualpos.y += 8;
//  }
  /*if (gridGetSolid((kid.pos.x) >> 4, (kid.pos.y + 8) >> 4))
    kid.actualpos.x += 8;
  else if (gridGetSolid((kid.pos.x + 11) >> 4, (kid.pos.y + 8) >> 4))
    kid.actualpos.x -= 8;
  else if (gridGetSolid((kid.pos.x + 6) >> 4, (kid.pos.y + 1) >> 4))
    kid.actualpos.y += 8;
  else if (gridGetSolid((kid.pos.x + 6) >> 4, (kid.pos.y + 14) >> 4))
    kid.actualpos.y -= 8;*/

  // -Y Position
  //if ((kid.speed.y > 0 && !solidV)
  //|| (kid.speed.y < 0 && !solidV))
  if (!solidV && kid.speed.y != 0)
  {
    kid.actualpos.y -= kid.speed.y;
  }
  else
  {
    if (solidV && kid.speed.y > 0)
    {
      kid.actualpos.y = ((kid.pos.y + 8) >> 4) << (FIXED_POINT + 4);
      kid.speed.y = 0;
    }
  }

  // -X Position
  //if ((kid.speed.x < 0 && !solidH) || (kid.speed.x > 0 && !solidH))
  if (kid.speed.x != 0)
  {
    if (!solidH)
    {
      kid.actualpos.x += kid.speed.x;
    }
    else
    {
      //if (kid.speed.x < 0)
      kid.speed.x = 0;
      kid.actualpos.x = ((((kid.pos.x + 6) >> 4) << 4) + ((!kid.direction) * 4)) << (FIXED_POINT);
      //kid.actualpos.x = (((kid.pos.x + 6) & 0xFFF0) + ((!kid.direction) * 4)) << FIXED_POINT;
      //kid.actualpos.x += ((kid.speed.x > 0) * 4) << FIXED_vec2;
    }
  }

  kid.pos = (kid.actualpos >> FIXED_POINT);

  if (kid.isSucking && arduboy.everyXFrames(3)) arduboy.audio.tone(320 + random(20), 20);
}

void updateCamera()
{
  // Camera offset
  if (cam.offset.x > 0) cam.offset.x--;
  else if (cam.offset.x < 0) cam.offset.x++;
  if (cam.offset.y > 0) cam.offset.y--;
  else if (cam.offset.y < 0) cam.offset.y++;

  vec2 cp;
  //kp = kid.pos;
  cp = (cam.pos + cam.offset);

  vec2 V;
  //vec2 V = (kid.pos - cam.pos + cam.offset) >> 3; // more bytes
  V.x = kid.pos.x - cp.x - 58;
  V.y = kid.pos.y - cp.y - 24;
  V = V >> 3;

  cam.pos += V;
  cam.pos.y = min(320, cam.pos.y);
}

void drawKid()
{
  if (kid.isActive)
  {
    vec2 kidcam;
    kidcam.x = kid.pos.x - cam.pos.x;
    kidcam.y = kid.pos.y - cam.pos.y;
    if (kid.isBalloon)
    {
      if (kid.balloons > 2) sprites.drawPlusMask(kidcam.x + 7 - 6 * kid.direction, kidcam.y - 12 + kid.balloonOffset, balloon_plus_mask, 1);
      if (kid.balloons > 1) sprites.drawPlusMask(kidcam.x + 1 - 6 * kid.direction, kidcam.y - 11 + kid.balloonOffset, balloon_plus_mask, 0);
      sprites.drawPlusMask(kidcam.x + 4 - 6 * kid.direction, kidcam.y - 9 + kid.balloonOffset, balloon_plus_mask, 1);
    }
    if (!kid.isSucking)
    {
      sprites.drawPlusMask
      (
        //kid.pos.x - cam.pos.x,
        //kid.pos.y - cam.pos.y,
        kidcam.x, kidcam.y,
        kidSpriteAlternative_plus_mask,
        kid.frame + 6 * kid.direction + 4 * kid.isJumping + 5 * (kid.isLanding || kid.isBalloon)
      );
    }
    else
    {
      sprites.drawPlusMask
      (
        kidcam.x, kidcam.y,
        kidSuck_plus_mask,
        (kid.frame % 2) + (2 * kid.direction)
      );
      for (byte i = 0; i < PLAYER_PARTICLES; ++i)
      {
        // Update
        if (kid.particles[i].y > 2) --kid.particles[i].y;
        else if (kid.particles[i].y < -2) ++kid.particles[i].y;
        kid.particles[i].x -= 2;
        if (kid.particles[i].x < 0)
        {
          kid.particles[i].x = 16;
          kid.particles[i].y = -4 + random(13);
        }

        // Draw
        if (kid.direction)
          arduboy.drawPixel(kidcam.x - kid.particles[i].x, kidcam.y + 10 + kid.particles[i].y, 0);
        else
          arduboy.drawPixel(kidcam.x + 15 + kid.particles[i].x, kidcam.y + 10 + kid.particles[i].y, 0);
      }
    }
  }
}


#endif
