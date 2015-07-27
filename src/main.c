#include "analog-timer.h"
#include <time.h>
#include <pebble.h>
#include "flips.h"

#define VIBES_STATUS 0
#define REVERT_STATUS 1
#define SCREEN_TYPE 2
#define DOWNMINS 3
#define DOWNSECS 4
#define RGB_RED 5
#define RGB_GREEN 6
#define RGB_BLUE 7
#define FLIPSCREEN 8

static Window *window;
static GRect bounds;
static GColor GColorWatchface, GColorContrast;

static Layer *window_layer, *s_simple_bg_layer, *s_date_layer, *s_hands_layer, *s_upicon_layer;
static TextLayer *s_day_label, *s_num_label, *s_timer_layer;

static GBitmap *watchface;
static BitmapLayer *watchface_layer;

static GPath *s_minute_arrow, *s_hour_arrow, *s_upiconr, *s_upiconl,*s_downiconr,*s_downiconl;
static char s_num_buffer[4], s_day_buffer[6], s_timer_buffer[] = "00:00";

static bool bt_ok = false;
static uint8_t battery_level;
static bool battery_plugged;

static GBitmap *icon_battery;
static GBitmap *icon_battery_charge;
static GBitmap *icon_bt;

static Layer *battery_layer;
static Layer *bt_layer;

static int minutes=0, seconds=0, downmins = 0, downsecs = 0;
static int rgb_red = 0, rgb_green = 0, rgb_blue = 0;  // default black face

static bool timer_started = false,
     up_timer = false,
     down_timer = false,
     timer_setup = false,
     timer_showing = false,
     min_timer = false,
     sec_timer = false,
     vibes_on = true,
     revert = false,
     flipscreen = false;

static int screen_type = 0;  //  Default = std black
/*
	0 = std black  (sb)
	1 = bold black (bb)
	2 = std white  (sw)
	3 = bold white (bw)
*/
static uint32_t short_vibe[] = {100};
static uint32_t double_vibe[] = {100, 100, 100};
static uint32_t end_vibe[] = {300, 200, 100, 100, 100};

//  Set count down timer vibration patterns
VibePattern shortvibe = {
    .durations = short_vibe,
    .num_segments = ARRAY_LENGTH(short_vibe),
};
VibePattern doublevibe = {
    .durations = double_vibe,
    .num_segments = ARRAY_LENGTH(double_vibe),
};
VibePattern endvibe = {
    .durations = end_vibe,
    .num_segments = ARRAY_LENGTH(end_vibe),
};

//  Up/Down long press timer for incrementing/decrementing minutes/seconds
void timer_callback(void *data) {
  if (min_timer) {
    minutes++;
    if (minutes > 99) {
      minutes = minutes - 100;
    }
  }
  if (sec_timer) {
    seconds--;
    if (seconds < 0) {
      seconds = seconds + 60;
    }
  }
  //  Display new time
  snprintf(s_timer_buffer,sizeof(s_timer_buffer),"%.2d:%.2d",minutes,seconds);
  text_layer_set_text(s_timer_layer, s_timer_buffer);
  if (min_timer || sec_timer) {
    app_timer_register(100, (AppTimerCallback) timer_callback, NULL); 
  }
}

//  Handle configuration selections
static void in_recv_handler(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);
  while (t) {
//APP_LOG(APP_LOG_LEVEL_INFO,"t->key = %d",(int)t->key);
    switch(t->key) {
      case VIBES_STATUS:
        if (strcmp(t->value->cstring, "on") == 0) {
          vibes_on = true;
        }
        else if (strcmp(t->value->cstring, "off") == 0) {
         vibes_on = false;
        }
        persist_write_bool(VIBES_STATUS, vibes_on); 
        break;
      case REVERT_STATUS:
        if (strcmp(t->value->cstring, "no") == 0) {
          revert = false;
        }
        else if (strcmp(t->value->cstring, "yes") == 0) {
         revert = true;
        }
        break;
      case SCREEN_TYPE:
        if (strcmp(t->value->cstring, "sb") == 0) {
          screen_type = 0;;
        }
        else if (strcmp(t->value->cstring, "bb") == 0) {
         screen_type = 1;
        }
        else if (strcmp(t->value->cstring, "sw") == 0) {
         screen_type = 2;
        }
        else if (strcmp(t->value->cstring, "bw") == 0) {
         screen_type = 3;
        }
        else {
          screen_type = 0;
        }
        break;
      case RGB_RED:
        rgb_red = atoi(t->value->cstring);
        break;
      case RGB_GREEN:
        rgb_green = atoi(t->value->cstring);
        break;
      case RGB_BLUE:
        rgb_blue = atoi(t->value->cstring);
        break;
      case FLIPSCREEN:
        if (strcmp(t->value->cstring, "no") == 0) {
          flipscreen = false;
        }
        else if (strcmp(t->value->cstring, "yes") == 0) {
         flipscreen = true;
        }
        break;
    }
    t = dict_read_next(iterator);
  }
  persist_write_bool(VIBES_STATUS, vibes_on); 
  persist_write_bool(REVERT_STATUS, revert); 
  persist_write_int(SCREEN_TYPE, screen_type); 
  persist_write_int(RGB_RED, rgb_red); 
  persist_write_int(RGB_GREEN, rgb_green); 
  persist_write_int(RGB_BLUE, rgb_blue); 
  persist_write_bool(FLIPSCREEN,flipscreen); 

  bitmap_layer_destroy(watchface_layer);
  gbitmap_destroy(watchface);

  switch(screen_type) {
    case 0:
      GColorWatchface = GColorBlack;
      GColorContrast = GColorWhite;
      watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_STD_B);
      break;
    case 1:
      GColorWatchface = GColorBlack;
      GColorContrast = GColorWhite;
      watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_BOLD_B);
      break;
    case 2:
      GColorWatchface = GColorWhite;  //  White standard
      GColorContrast = GColorBlack;
      watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_STD_W);
      break;
    case 3:
      GColorWatchface = GColorWhite;  //  White bold
      GColorContrast = GColorBlack;
      watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_BOLD_W);
      break;
  }

  #ifdef PBL_COLOR
    GColorWatchface = GColorFromRGB(rgb_red, rgb_green,rgb_blue);
  #endif

  text_layer_set_background_color(s_day_label, GColorWatchface);
  text_layer_set_text_color(s_day_label, GColorContrast);
  text_layer_set_background_color(s_num_label, GColorWatchface);
  text_layer_set_text_color(s_num_label, GColorContrast);
  text_layer_set_background_color(s_timer_layer,GColorContrast);
  text_layer_set_text_color(s_timer_layer,GColorWatchface);
  if (screen_type < 2) {
    icon_battery = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_ICON_W);
    icon_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGE_W);
    icon_bt = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_W);
  }
  else {
    icon_battery = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_ICON_B);
    icon_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGE_B);
    icon_bt = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_B);
  }
  layer_mark_dirty(battery_layer);
  layer_mark_dirty(bt_layer);
  watchface_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(watchface_layer,watchface);
#ifdef PBL_COLOR
  bitmap_layer_set_compositing_mode(watchface_layer,GCompOpSet);
#else
  bitmap_layer_set_compositing_mode(watchface_layer,GCompOpAssign);
#endif
  bitmap_layer_set_background_color(watchface_layer,GColorWatchface);
  layer_insert_below_sibling((Layer *)watchface_layer,(Layer *)s_date_layer);
}

//  Display the up/down arrow for timer selected
static void upicon_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWatchface);
  gpath_draw_filled(ctx, s_upiconr);
  gpath_draw_filled(ctx, s_upiconl);
  gpath_draw_filled(ctx, s_downiconr);
  gpath_draw_filled(ctx, s_downiconl);
  if (up_timer) {
    graphics_context_set_fill_color(ctx, GColorContrast);
    gpath_draw_filled(ctx, s_upiconr);
    gpath_draw_filled(ctx, s_upiconl);
  }
  if (down_timer) {
    graphics_context_set_fill_color(ctx, GColorContrast);
    gpath_draw_filled(ctx, s_downiconr);
    gpath_draw_filled(ctx, s_downiconl);
  }
}

//  Update the watch hands
static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorContrast);
  graphics_context_set_stroke_color(ctx, GColorWatchface);
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorWatchface);
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone);
  
  if (flipscreen) {
    flipHV(ctx);
  }
}

//  Update the date
static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_day_buffer, sizeof(s_day_buffer), "%a", t);
  text_layer_set_text(s_day_label, s_day_buffer);

  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  text_layer_set_text(s_num_label, s_num_buffer);
}

//  Update the current battery level
void battery_layer_update_callback(Layer *layer, GContext *ctx) {

#ifdef PBL_COLOR
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#endif

  if (!battery_plugged) {
    graphics_draw_bitmap_in_rect(ctx, icon_battery, GRect(0, 0, 24, 12));
    graphics_context_set_stroke_color(ctx, GColorWatchface);
    graphics_context_set_fill_color(ctx, GColorContrast);
    graphics_fill_rect(ctx, GRect(7, 4, (uint8_t)((battery_level / 100.0) * 11.0), 4), 0, GCornerNone);
  } else {
    graphics_draw_bitmap_in_rect(ctx, icon_battery_charge, GRect(0, 0, 24, 12));
  }
}

void battery_state_handler(BatteryChargeState charge) {
        battery_level = charge.charge_percent;
        battery_plugged = charge.is_plugged;
        layer_mark_dirty(battery_layer);
}

//  Update the bluetooth connection status
void bt_layer_update_callback(Layer *layer, GContext *ctx) {
  if (bt_ok) {
#ifdef PBL_COLOR
        graphics_context_set_compositing_mode(ctx, GCompOpSet);
#else
        graphics_context_set_compositing_mode(ctx, GCompOpAssign);
#endif
  }
  else {
        graphics_context_set_compositing_mode(ctx, GCompOpClear);
  }
  graphics_draw_bitmap_in_rect(ctx, icon_bt, GRect(0, 0, 9, 12));
}

void bt_connection_handler(bool connected) {
        bt_ok = connected;
        layer_mark_dirty(bt_layer);
}

//  Update the time and, if displayed, the timer
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (timer_started) {
    if (up_timer) {
      seconds++;
      if (seconds > 59) {
        seconds = 0;
        minutes++;
        if (minutes > 99) {
          minutes = minutes - 100;
        }
      }
    }
    else if (down_timer) {
      seconds--;
      if (seconds < 0) {
        minutes--;
        seconds = 59;
      }
      if  (vibes_on && minutes == 0 && seconds < 6) {
        switch (seconds) {
          case 5:
            vibes_enqueue_custom_pattern(shortvibe);
            break;
          case 4:
            vibes_enqueue_custom_pattern(shortvibe);
            break;
          case 3:
            vibes_enqueue_custom_pattern(shortvibe);
            break;
          case 2:
            vibes_enqueue_custom_pattern(doublevibe);
            break;
          case 1:
            vibes_enqueue_custom_pattern(doublevibe);
            break;
          case 0:
            vibes_enqueue_custom_pattern(endvibe);
            timer_started = false;
            up_timer = false;
            down_timer = false;
            minutes = 0;
            seconds = 0;
            break;
        }
      }
      else if (minutes == 0 && seconds == 0) {
        timer_started = false;
        up_timer = false;
        down_timer = false;
        minutes = 0;
        seconds = 0;
      } 
    }
//Format the buffer string using minutes and seconds as the source
    snprintf(s_timer_buffer,sizeof(s_timer_buffer),"%.2d:%.2d",minutes,seconds);
 
//Change the TextLayer text to show the new time!
    text_layer_set_text(s_timer_layer, s_timer_buffer);
  }

  layer_mark_dirty(window_get_root_layer(window));
}

void uptimer() {
  if (timer_showing){
    if (! timer_started) {
    //  Don't do anything if the timer has started
      if (timer_setup) {
        //  The up button is used to increment minutes
         minutes++;
        if (minutes > 99) {
          minutes = minutes - 100;
         }
         snprintf(s_timer_buffer,sizeof(s_timer_buffer),"%.2d:%.2d",minutes,seconds);
         text_layer_set_text(s_timer_layer, s_timer_buffer);
       }
      else {
//  The up button is being used to enter the up timer routine
        up_timer = true;
        down_timer = false;
        timer_setup = false;
        minutes = 0;
        seconds = 0;
        text_layer_set_text(s_timer_layer, "00:00");
        layer_set_update_proc(s_upicon_layer,upicon_proc);
      }  
    }
  }
}

void long_up_down_timer() {
  if (timer_showing && timer_setup) {
    min_timer = true;
    app_timer_register(500, (AppTimerCallback) timer_callback, NULL); 
  }
}

void long_up_up_timer() {
  min_timer = false;
}

//  The select button is used to (1) start a timer or (2) pause/continue the up timer
void select_click_handler(ClickRecognizerRef recognizer, void *context)
{
  if (timer_showing) {
    if (timer_started) {
      if (up_timer) {
  //  Only pause if the up timer is runnng
        timer_started = false;
      }
    }
    else {
  //  Start/restart the timer
      if (down_timer) {
//  Place the final time in the down timer time slot
        downmins = minutes;
        downsecs = seconds;
      }
      if (up_timer || down_timer) {
        timer_started = true;
        timer_setup = false;
        tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) handle_second_tick);
      }
    }
  }
  else {

    timer_showing = true;

//  Get last down timer time
    if (persist_exists(DOWNMINS)) {
      downmins = persist_read_int(DOWNMINS);
    }
    if (persist_exists(DOWNSECS)) {
      downsecs = persist_read_int(DOWNSECS);
    }

    layer_set_hidden((Layer*) s_timer_layer, false);
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) handle_second_tick);
  }  
}

//  Long select btton closes the timer dislay and swithes back to per minute processing
void select_long_click_handler(ClickRecognizerRef recognizer, void *context)
{
  if (timer_showing) {
//  Long select button removes thetimer from view
    timer_showing = false;
    timer_started = false;
    timer_setup = false;
    up_timer = false;
    down_timer = false;
    minutes = 0;
    seconds = 0;
    persist_write_int(DOWNMINS,downmins);
    persist_write_int(DOWNSECS,downsecs);
    text_layer_set_text(s_timer_layer, "");
    layer_set_hidden((Layer*) s_timer_layer, true);
    layer_set_update_proc(s_upicon_layer,upicon_proc);
    tick_timer_service_unsubscribe();
    tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) handle_second_tick);
    if (revert) {
      window_stack_pop_all(true);
    }
  }
}

void downtimer() {
  if (timer_showing) {
    if (! timer_started) {
      if (timer_setup) {
 //  The down button is used to decrement seconds
        seconds--;
        if (seconds < 0) {
          seconds = 59;
        }
        snprintf(s_timer_buffer,sizeof(s_timer_buffer),"%.2d:%.2d",minutes,seconds);
        text_layer_set_text(s_timer_layer, s_timer_buffer);
      }
      else {
    //  The down button is being used to select coount down timer
        up_timer = false;
        down_timer = true;
        timer_setup = true;
        minutes = downmins;
        seconds = downsecs;
        snprintf(s_timer_buffer,sizeof(s_timer_buffer),"%.2d:%.2d",minutes,seconds);
        text_layer_set_text(s_timer_layer, s_timer_buffer);
        layer_set_update_proc(s_upicon_layer,upicon_proc);
      }
    }
  }
}

void long_down_down_timer() {
  if (timer_showing && timer_setup) {
    sec_timer = true;
    app_timer_register(500, (AppTimerCallback) timer_callback, NULL); 
  }
}

void long_down_up_timer() {
  sec_timer = false;
}

  //  Set up the click handler
void up_click_handler(ClickRecognizerRef recognizer, void *context)
{
  if (flipscreen) {
    downtimer();
  }
  else {
    uptimer();
  }
}

//  The up long click routines are to speed up adding minutes 
void long_up_click_down_handler(ClickRecognizerRef recognizer, void *context)
{
  if (flipscreen) {
    long_down_down_timer();
  }
  else {
    long_up_down_timer();
  }
}

void long_up_click_up_handler(ClickRecognizerRef recognizer, void *context)
{
  if (flipscreen) {
    long_down_up_timer();
  }
  else {
    long_up_up_timer();
  }
}

//  Display the count down timer or decrement seconds
void down_click_handler(ClickRecognizerRef recognizer, void *context)
{
  if (flipscreen) {
    uptimer();
  }
  else {
    downtimer();
  }
}

void long_down_click_down_handler(ClickRecognizerRef recognizer, void *context)
{
  if (flipscreen) {
    long_up_down_timer();
  }
  else {
    long_down_down_timer();
  }
}

void long_down_click_up_handler(ClickRecognizerRef recognizer, void *context)
{
  if (flipscreen) {
    long_up_up_timer();
  }
  else {
    long_down_up_timer();
  }
}

void click_config_provider(void *context)
{
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_long_click_subscribe(BUTTON_ID_SELECT, 500, NULL, select_long_click_handler);
    window_long_click_subscribe(BUTTON_ID_UP, 500, long_up_click_down_handler, long_up_click_up_handler);
    window_long_click_subscribe(BUTTON_ID_DOWN, 500, long_down_click_down_handler, long_down_click_up_handler);
}


static void window_load(Window *window) {
  window_layer = window_get_root_layer(window);
  bounds = layer_get_bounds(window_layer);

//  Set up date layer
  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);

  s_day_label = text_layer_create(GRect(48, 114, 27, 20));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_text_color(s_day_label, GColorContrast);
  text_layer_set_background_color(s_day_label, GColorWatchface);
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));

  s_num_label = text_layer_create(GRect(75, 114, 18, 20));
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_text_color(s_num_label, GColorContrast);
  text_layer_set_background_color(s_num_label, GColorWatchface);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

  // Status setup
  if (screen_type < 2) {
    icon_battery = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_ICON_W);
    icon_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGE_W);
    icon_bt = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_W);
  }
  else {
    icon_battery = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_ICON_B);
    icon_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGE_B);
    icon_bt = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_B);
  }
  BatteryChargeState initial = battery_state_service_peek();
  battery_level = initial.charge_percent;
  battery_plugged = initial.is_plugged;
  battery_layer = layer_create(GRect(50,31,24,12)); //24*12
  layer_set_update_proc(battery_layer, &battery_layer_update_callback);

  bt_ok = bluetooth_connection_service_peek();
  bt_layer = layer_create(GRect(83,31,9,12)); //9*12
  layer_set_update_proc(bt_layer, &bt_layer_update_callback);

//  ---  Timer

  s_timer_layer = text_layer_create(GRect(42,105,60,30));
  text_layer_set_text_alignment(s_timer_layer, GTextAlignmentCenter);
  ResHandle font_handle = resource_get_handle(RESOURCE_ID_DIGITAL_24);
  text_layer_set_font(s_timer_layer, fonts_load_custom_font(font_handle));

//  Create up/down timer icons
  s_upiconr = gpath_create(&UPICONR);
  s_upiconl = gpath_create(&UPICONL);
  s_downiconr = gpath_create(&DOWNICONR);
  s_downiconl = gpath_create(&DOWNICONL);
  s_upicon_layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(s_upicon_layer,upicon_proc);

  text_layer_set_text(s_timer_layer,"");

//  Hands

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);

//  Set clock face
  if (persist_exists(SCREEN_TYPE)) {
    screen_type = persist_read_int(SCREEN_TYPE);
    switch(screen_type) {
      case 0:
        watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_STD_B);
        text_layer_set_text_color(s_timer_layer,GColorBlack);
        break;
      case 1:
        watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_BOLD_B);
        text_layer_set_text_color(s_timer_layer,GColorBlack);
        break;
      case 2:
        watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_STD_W);
        text_layer_set_text_color(s_timer_layer,GColorWhite);
        break;
      case 3:
        watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_BOLD_W);
        text_layer_set_text_color(s_timer_layer,GColorWhite);
        break;
      default:
        watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_STD_B);
        text_layer_set_text_color(s_timer_layer,GColorBlack);
        break;
    }
    text_layer_set_background_color(s_day_label, GColorWatchface);
    text_layer_set_text_color(s_day_label, GColorContrast);
    text_layer_set_background_color(s_num_label, GColorWatchface);
    text_layer_set_text_color(s_num_label, GColorContrast);
    text_layer_set_background_color(s_timer_layer,GColorContrast);
  }
  else {
    watchface = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_STD_B);
  }
  watchface_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(watchface_layer,watchface);
#ifdef PBL_COLOR
  bitmap_layer_set_compositing_mode(watchface_layer,GCompOpSet);
#else
  bitmap_layer_set_compositing_mode(watchface_layer,GCompOpAssign);
#endif
  bitmap_layer_set_background_color(watchface_layer, GColorWatchface);
  layer_add_child(window_layer,bitmap_layer_get_layer(watchface_layer));

//  Add child layes to window

  layer_add_child(window_layer, s_date_layer);
  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));
  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));
  layer_add_child(window_layer, battery_layer);
  layer_add_child(window_layer, bt_layer);
  layer_add_child(window_layer, s_upicon_layer);
  layer_add_child(window_layer,(Layer*) s_timer_layer);
  layer_set_hidden((Layer*) s_timer_layer, true);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {

  layer_destroy(s_upicon_layer);
  gpath_destroy(s_upiconl);
  gpath_destroy(s_upiconr);
  gpath_destroy(s_downiconl);
  gpath_destroy(s_downiconr);

  bitmap_layer_destroy(watchface_layer);
  gbitmap_destroy(watchface);

  gbitmap_destroy(icon_battery);
  gbitmap_destroy(icon_battery_charge);
  gbitmap_destroy(icon_bt);
  layer_destroy(battery_layer);
  layer_destroy(bt_layer);

  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);

  text_layer_destroy(s_timer_layer);
  layer_destroy(s_hands_layer);

  if (timer_showing) {
//  Long select button removes thetimer from view
    timer_showing = false;
    timer_started = false;
    timer_setup = false;
    up_timer = false;
    down_timer = false;
    minutes = 0;
    seconds = 0;
    persist_write_int(DOWNMINS,downmins);
    persist_write_int(DOWNSECS,downsecs);
  }
}

static void init() {

  app_message_register_inbox_received((AppMessageInboxReceived) in_recv_handler);
  app_message_open(app_message_inbox_size_maximum(),app_message_outbox_size_maximum());
  if (persist_exists(VIBES_STATUS)) {
    vibes_on = persist_read_bool(VIBES_STATUS);
  }
  if (persist_exists(REVERT_STATUS)) {
    revert = persist_read_bool(REVERT_STATUS);
  }
  if (persist_exists(SCREEN_TYPE)) {
      screen_type = persist_read_int(SCREEN_TYPE);
  }
  if (persist_exists(FLIPSCREEN)) {
      flipscreen = persist_read_bool(FLIPSCREEN);
  }
  if (persist_exists(RGB_RED)) {
    rgb_red = persist_read_int(RGB_RED);
  }
  if (persist_exists(RGB_GREEN)) {
    rgb_green = persist_read_int(RGB_GREEN);
  }
  if (persist_exists(RGB_BLUE)) {
    rgb_blue = persist_read_int(RGB_BLUE);
  }
//  Set up the color scheme
  switch (screen_type) {
    case 0:  // black standard
      GColorWatchface = GColorBlack;
      GColorContrast = GColorWhite;
      break;
    case 1:  //  Black bold
      GColorWatchface = GColorBlack;
      GColorContrast = GColorWhite;
      break;
    case 2:
      GColorWatchface = GColorWhite;  //  White standard
      GColorContrast = GColorBlack;
      break;
    case 3: 
      GColorWatchface = GColorWhite;  //  White bold
      GColorContrast = GColorBlack;
      break;
    default:
      GColorWatchface = GColorBlack;
      GColorContrast = GColorWhite;
      break;
  }
  #ifdef PBL_COLOR
    GColorWatchface = GColorFromRGB(rgb_red, rgb_green,rgb_blue);
  #endif
  window = window_create();
  #ifdef PBL_PLATFORM_APLITE
    window_set_fullscreen(window,true);
  #endif
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  s_day_buffer[0] = '\0';
  s_num_buffer[0] = '\0';

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  bluetooth_connection_service_subscribe(&bt_connection_handler);
  battery_state_service_subscribe (&battery_state_handler);
  window_set_click_config_provider(window, click_config_provider);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_second_tick);
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
