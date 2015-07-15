#pragma once

#include "pebble.h"

static const GPathInfo MINUTE_HAND_POINTS = {
  5,
  (GPoint []) {
    { -4, 10 },
    { 4, 10 },
    { 4, -60 },
    { 0, -65 },
    { -4, -60 }
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  5, (GPoint []){
    {-5, 10},
    {4, 10},
    {4, -45},
    {0, -50},
    {-5, -45}
  }
};

static const GPathInfo UPICONR  = {
  3,
  (GPoint[]) {
    {109,105},
    {104,115},
    {114,115}
  }
};

static const GPathInfo UPICONL  = {
  3,
  (GPoint[]) {
    {35,105},
    {30,115},
    {40,115}
  }
};

static const GPathInfo DOWNICONR  = {
  3,
  (GPoint[]) {
    {109,135},
    {104,125},
    {114,125}
  }
};

static const GPathInfo DOWNICONL  = {
  3,
  (GPoint[]) {
    {35,135},
    {40,125},
    {30,125}
  }
};
