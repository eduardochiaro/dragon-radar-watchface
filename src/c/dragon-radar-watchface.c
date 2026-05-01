#include <pebble.h>

static Window *s_window;
static Layer *s_canvas_layer;

// ----- Dragon Ball dynamic state -----
#define DBALL_MAX         7
#define BLINK_INTERVAL_MS 300
#define BLINK_TOTAL       6   // 6 toggles = 3 full on/off blinks, ends solid

static int       s_dball_count      = 0;
static uint8_t   s_dball_px[DBALL_MAX];   // x position 0-100 (percent of screen)
static uint8_t   s_dball_py[DBALL_MAX];   // y position 0-100 (percent of screen)
static int       s_dball_radius[DBALL_MAX];
static bool      s_dball_visible    = true;
static int       s_blink_remaining  = 0;
static AppTimer *s_blink_timer      = NULL;

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_time_shadow_layer;
static TextLayer *s_date_shadow_layer;

// Buffers
static char s_time_buffer[8];
static char s_date_buffer[16];

#define DBALL_RADIUS 6
static int       s_last_minute      = -1;
// -------------------------------------

static void prv_blink_callback(void *context) {
  s_dball_visible = !s_dball_visible;
  s_blink_remaining--;
  layer_mark_dirty(s_canvas_layer);

  if (s_blink_remaining > 0) {
    s_blink_timer = app_timer_register(BLINK_INTERVAL_MS, prv_blink_callback, NULL);
  } else {
    s_blink_timer = NULL;
    s_dball_visible = true;  // ensure solid at end
    layer_mark_dirty(s_canvas_layer);
  }
}

static void prv_update_time(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_time_buffer, sizeof(s_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);

  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %d", t);

  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_date_layer, s_date_buffer);
#if defined(PBL_COLOR)
  text_layer_set_text(s_time_shadow_layer, s_time_buffer);
  text_layer_set_text(s_date_shadow_layer, s_date_buffer);
#endif
}

static void prv_generate_balls(void) {
  s_dball_count = 1 + (rand() % 7);  // 1 to 7
  for (int i = 0; i < s_dball_count; i++) {
    s_dball_px[i]     = 10 + (rand() % 80);
    s_dball_py[i]     = 10 + (rand() % 80);
    s_dball_radius[i] = DBALL_RADIUS;
  }

  if (s_blink_timer) {
    app_timer_cancel(s_blink_timer);
    s_blink_timer = NULL;
  }

  s_dball_visible   = true;
  s_blink_remaining = BLINK_TOTAL;
  s_blink_timer     = app_timer_register(BLINK_INTERVAL_MS, prv_blink_callback, NULL);
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  prv_update_time();
  if (tick_time->tm_min == s_last_minute) return;
  s_last_minute = tick_time->tm_min;
  prv_generate_balls();
}

static void prv_canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Fill background
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorMayGreen, GColorWhite));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw 40x40 grid centered
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorDarkGray));
  graphics_context_set_stroke_width(ctx, 1);

  int x_offset = (bounds.size.w % 40) / 2;
  int y_offset = (bounds.size.h % 40) / 2;

  for (int x = x_offset; x <= bounds.size.w; x += 40) {
    graphics_draw_line(ctx, GPoint(x, 0), GPoint(x, bounds.size.h));
  }

  for (int y = y_offset; y <= bounds.size.h; y += 40) {
    graphics_draw_line(ctx, GPoint(0, y), GPoint(bounds.size.w, y));
  }

  bool is_large_screen = bounds.size.w >= 200;

  // Draw dragon balls (gold circles), respecting blink state
  if (s_dball_visible) {
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorLightGray));
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorRajah, GColorDarkGray));
    graphics_context_set_stroke_width(ctx, 1);
    for (int i = 0; i < s_dball_count; i++) {
      GPoint center = GPoint(
        (int)(s_dball_px[i] * bounds.size.w / 100),
        (int)(s_dball_py[i] * bounds.size.h / 100)
      );
      graphics_fill_circle(ctx, center, s_dball_radius[i]);
      graphics_draw_circle(ctx, center, s_dball_radius[i]);
    }
  }

  // Draw red triangles at center of each edge, pointing outward
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorBlack));
  int cx = bounds.size.w / 2;
  int cy = bounds.size.h / 2;
  int ts = is_large_screen ? 8 : 6;
  int te = is_large_screen ? 10 : 5;

  GPoint top_pts[] = { GPoint(cx, te), GPoint(cx - ts, te + ts), GPoint(cx + ts, te + ts) };
  GPathInfo top_info = { .num_points = 3, .points = top_pts };
  GPath *top_path = gpath_create(&top_info);
  gpath_draw_filled(ctx, top_path);
  gpath_destroy(top_path);

  GPoint bot_pts[] = { GPoint(cx, bounds.size.h - te), GPoint(cx - ts, bounds.size.h - te - ts), GPoint(cx + ts, bounds.size.h - te - ts) };
  GPathInfo bot_info = { .num_points = 3, .points = bot_pts };
  GPath *bot_path = gpath_create(&bot_info);
  gpath_draw_filled(ctx, bot_path);
  gpath_destroy(bot_path);

  GPoint left_pts[] = { GPoint(te, cy), GPoint(te + ts, cy - ts), GPoint(te + ts, cy + ts) };
  GPathInfo left_info = { .num_points = 3, .points = left_pts };
  GPath *left_path = gpath_create(&left_info);
  gpath_draw_filled(ctx, left_path);
  gpath_destroy(left_path);

  GPoint right_pts[] = { GPoint(bounds.size.w - te, cy), GPoint(bounds.size.w - te - ts, cy - ts), GPoint(bounds.size.w - te - ts, cy + ts) };
  GPathInfo right_info = { .num_points = 3, .points = right_pts };
  GPath *right_path = gpath_create(&right_info);
  gpath_draw_filled(ctx, right_path);
  gpath_destroy(right_path);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, prv_canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  srand(time(NULL));
  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);
  prv_generate_balls();

  bool is_large_screen = bounds.size.w >= 200;
  int16_t screen_w = bounds.size.w;
  int16_t screen_h = bounds.size.h;

  int16_t s_bottom_h = is_large_screen ? 125 : 85;

  int16_t text_position_h = screen_h - s_bottom_h;

   // Font selection based on screen size
  #if defined(PBL_PLATFORM_GABBRO) || defined(PBL_PLATFORM_EMERY)
    const char *time_font_key = FONT_KEY_LECO_60_NUMBERS_AM_PM;
  #else
    const char *time_font_key = FONT_KEY_LECO_36_BOLD_NUMBERS;
  #endif  

  const char *date_font_key = is_large_screen
    ? FONT_KEY_GOTHIC_28_BOLD
    : FONT_KEY_GOTHIC_24_BOLD;

  int16_t time_h = is_large_screen ? 60 : 36;
  int16_t date_h = is_large_screen ? 28 : 24;
  int16_t total_text_h = time_h + date_h;
  int16_t y_start = text_position_h + (s_bottom_h - total_text_h) / 2;

  // Time shadow layer (color only, black, 2px offset)
#if defined(PBL_COLOR)
  int16_t offset = is_large_screen ? 4 : 2;
  s_time_shadow_layer = text_layer_create(GRect(offset, y_start - 4 + offset, screen_w, time_h + 8));
  text_layer_set_background_color(s_time_shadow_layer, GColorClear);
  text_layer_set_text_color(s_time_shadow_layer, GColorBlack);
  text_layer_set_text_alignment(s_time_shadow_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_shadow_layer, fonts_get_system_font(time_font_key));
  layer_add_child(window_layer, text_layer_get_layer(s_time_shadow_layer));

  // Date shadow layer (color only, black, 2px offset)
  s_date_shadow_layer = text_layer_create(GRect(2, y_start + time_h - 6 + 2, screen_w, date_h + 4));
  text_layer_set_background_color(s_date_shadow_layer, GColorClear);
  text_layer_set_text_color(s_date_shadow_layer, GColorBlack);
  text_layer_set_text_alignment(s_date_shadow_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_shadow_layer, fonts_get_system_font(date_font_key));
  layer_add_child(window_layer, text_layer_get_layer(s_date_shadow_layer));
#endif

  // Time text layer
  s_time_layer = text_layer_create(GRect(0, y_start - 4, screen_w, time_h + 8));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(time_font_key));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Date text layer
  s_date_layer = text_layer_create(GRect(0, y_start + time_h - 6, screen_w, date_h + 4));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, fonts_get_system_font(date_font_key));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  prv_update_time();
}

static void prv_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  if (s_blink_timer) {
    app_timer_cancel(s_blink_timer);
    s_blink_timer = NULL;
  }
  layer_destroy(s_canvas_layer);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
#if defined(PBL_COLOR)
  text_layer_destroy(s_time_shadow_layer);
  text_layer_destroy(s_date_shadow_layer);
#endif
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}

