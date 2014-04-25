#include "pebble.h"

Window *window;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
TextLayer *title_text;
TextLayer *bat_layer;
Layer *line_layer;
static Layer *layer;
GBitmap *image;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";
  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  
  text_layer_set_text(bat_layer, battery_text);
}

static void layer_update_callback(Layer *me, GContext* ctx) {
  // We make sure the dimensions of the GRect to draw into
  // are equal to the size of the bitmap--otherwise the image
  // will automatically tile. Which might be what *you* want.

  GRect bounds = image->bounds;

  graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 144-64, 0 }, .size = bounds.size });

}

void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorClear);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxxx 00";

  char *time_format;


  // TODO: Only update the date when it's changed.
  strftime(date_text, sizeof(date_text), "%b %e", tick_time);
  text_layer_set_text(text_date_layer, date_text);


  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);
  
  handle_battery(battery_state_service_peek());
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  gbitmap_destroy(image);
  window_destroy(window);
}

void handle_init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorWhite);

  Layer *window_layer = window_get_root_layer(window);

  text_date_layer = text_layer_create(GRect(3, 100, 144-2, 120));
  text_layer_set_text_color(text_date_layer, GColorBlack);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAO_20)));
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  text_time_layer = text_layer_create(GRect(2, 120, 144-2, 168-120));
  text_layer_set_text_color(text_time_layer, GColorBlack);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAO_32)));
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));
  
  title_text = text_layer_create(GRect(3, 3, 144-10, 168-10));
  text_layer_set_text_color(title_text, GColorBlack);
  text_layer_set_background_color(title_text, GColorClear);
  text_layer_set_font(title_text, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAO_32)));
  layer_add_child(window_layer, text_layer_get_layer(title_text));
  text_layer_set_text(title_text, "Sword\nArt\nOnline");
  text_layer_set_text_alignment(title_text, GTextAlignmentLeft);
  
  bat_layer = text_layer_create(GRect(120, 3, /* width */ 149, 168 /* height */));
  text_layer_set_text_color(bat_layer, GColorBlack);
  text_layer_set_background_color(bat_layer, GColorClear);
  text_layer_set_font(bat_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SAO_14)));
  text_layer_set_text_alignment(bat_layer, GTextAlignmentLeft);
  text_layer_set_text(bat_layer, "");

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  battery_state_service_subscribe(&handle_battery);
  // TODO: Update display here to avoid blank display on launch?
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_minute_tick(current_time, MINUTE_UNIT);
  
  // Init the layer for display the image
  window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  layer = layer_create(bounds);
  layer_set_update_proc(layer, layer_update_callback);
  layer_add_child(window_layer, layer);
  layer_add_child(window_layer, text_layer_get_layer(bat_layer));

  image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ELU);
}


int main(void) {
  handle_init();

  app_event_loop();
  
  handle_deinit();
}