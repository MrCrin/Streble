#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_hour_layer;
static Layer *arc_layer;
static TextLayer *s_min_layer;
static TextLayer *s_date_layer;
static TextLayer *s_month_layer;
static TextLayer *s_totals_layer;
static GFont s_time_font;
static BitmapLayer *s_background_layer;
static BitmapLayer *s_lowbatt_layer;
static BitmapLayer *s_nobt_layer;
static GBitmap *s_background_bitmap;
static GBitmap *s_lowbatt_bitmap;
static GBitmap *s_nobt_bitmap;
static TextLayer *s_monthmiles_layer;
static TextLayer *s_monthelevation_layer;
static TextLayer *s_totalmiles_layer;
static TextLayer *s_totalelevation_layer;
static int init_complete;
static char monthmiles_buffer[32];
static char monthelevation_buffer[32];
static char totalmiles_buffer[32];
static char totalelevation_buffer[32];
static int goalprogress_buffer = 0;
static char goal_buffer[32];
static int goal;
static int percentage = 0;
static int degrees = 0;
AppTimer *timer;

enum {
  KEY_MONTHMILES = 0,
  KEY_MONTHELEVATION = 1,
  KEY_TOTALMILES = 2,
  KEY_TOTALELEVATION = 3,
	 KEY_GOALPROGRESS = 4,
	 KEY_GOAL = 5
};

/*DrawArc function thanks to Cameron MacFarland (https://gist.github.com/distantcam/5477225)*/
void graphics_draw_arc(GContext *ctx, GPoint p, int radius, int thickness, int start, int end) {
  start = start % 360;
  end = end % 360;

  while (start < 0) start += 360;
  while (end < 0) end += 360;

  if (end == 0) end = 360;
  
  float sslope = (float)cos_lookup(start * TRIG_MAX_ANGLE / 360) / (float)sin_lookup(start * TRIG_MAX_ANGLE / 360);
  float eslope = (float)cos_lookup(end * TRIG_MAX_ANGLE / 360) / (float)sin_lookup(end * TRIG_MAX_ANGLE / 360);

  if (end == 360) eslope = -1000000;

  int ir2 = (radius - thickness) * (radius - thickness);
  int or2 = radius * radius;

  for (int x = -radius; x <= radius; x++)
    for (int y = -radius; y <= radius; y++)
    {
      int x2 = x * x;
      int y2 = y * y;

      if (
        (x2 + y2 < or2 && x2 + y2 >= ir2) &&
        (
          (y > 0 && start < 180 && x <= y * sslope) ||
          (y < 0 && start > 180 && x >= y * sslope) ||
          (y < 0 && start <= 180) ||
          (y == 0 && start <= 180 && x < 0) ||
          (y == 0 && start == 0 && x > 0)
        ) &&
        (
          (y > 0 && end < 180 && x >= y * eslope) ||
          (y < 0 && end > 180 && x <= y * eslope) ||
          (y > 0 && end >= 180) ||
          (y == 0 && end >= 180 && x < 0) ||
          (y == 0 && start == 0 && x > 0)
        )
      )
        graphics_draw_pixel(ctx, GPoint(p.x + x, p.y + y));
    }
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create time and date buffer
  static char hourbuffer[] = "00";
  static char minbuffer[] = "00";	
  static char datebuffer[] = "WEDNESDAY 00";
	 static char monthbuffer[] = "SEPTEMBER TOTALS";	

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(hourbuffer, sizeof("00"), "%H", tick_time);
			 strftime(minbuffer, sizeof("00"), "%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(hourbuffer, sizeof("00"), "%I", tick_time);
			 strftime(minbuffer, sizeof("00"), "%M", tick_time);
  }
	
		//Set date and display on date layer
		strftime(datebuffer, sizeof(datebuffer), "%a %d", tick_time);
		text_layer_set_text(s_date_layer, datebuffer);
	
		//Set month and display on month layer
		strftime(monthbuffer, sizeof(monthbuffer), "%B", tick_time);
		text_layer_set_text(s_month_layer, monthbuffer);
	
  // Display this time on time layer
  text_layer_set_text(s_hour_layer, hourbuffer);
	 text_layer_set_text(s_min_layer, minbuffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	  update_time();
}

//On receiving AppMessage from phone
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  (void) context;
	 
	 void process_tuple(Tuple *t) {
			
				//Get key
				int key = t->key;

				//Get string value
				char string_value[32];
				strcpy(string_value, t->value->cstring);

				//Take the value of each key, enter it into the text layer
				if(key == KEY_MONTHMILES) {
								snprintf(monthmiles_buffer, sizeof("000000 mi"), "%s mi", string_value);
								text_layer_set_text(s_monthmiles_layer, monthmiles_buffer);
				}else if(key == KEY_MONTHELEVATION){
								snprintf(monthelevation_buffer, sizeof("ft 000000"), "ft %s", string_value);
								text_layer_set_text(s_monthelevation_layer, monthelevation_buffer);
				}else if(key == KEY_TOTALMILES){
								snprintf(totalmiles_buffer, sizeof("000000 mi"), "%s mi", string_value);
								text_layer_set_text(s_totalmiles_layer, totalmiles_buffer);
					}else if(key == KEY_TOTALELEVATION){
								snprintf(totalelevation_buffer, sizeof("ft 0000000"), "ft %s", string_value);
								text_layer_set_text(s_totalelevation_layer, totalelevation_buffer);
					}else if(key == KEY_GOALPROGRESS){
					   goalprogress_buffer = atoi(string_value);
					   persist_write_int(KEY_GOALPROGRESS, goalprogress_buffer);
				 }else if(key == KEY_GOAL){
					   snprintf(goal_buffer, sizeof("00000"), "%s", string_value);
					   persist_write_string(KEY_GOAL, goal_buffer);
				 }
		}
     
  //Get data
  Tuple *t = dict_read_first(iterator);
  while(t != NULL)
    {
      process_tuple(t);
      //Get next
      t = dict_read_next(iterator);
    }
  
}

static void arc_update_proc(Layer *this_layer, GContext *ctx) {
		if (persist_exists(KEY_GOAL)) {
			 persist_read_string(KEY_GOAL, goal_buffer, sizeof(goal_buffer));
	 }
	 if (persist_exists(KEY_GOALPROGRESS)) {
			 persist_read_int(KEY_GOALPROGRESS);
	 }
  // Draw arc
	 GPoint center = GPoint(72, 84);
	 goal = atoi(goal_buffer);
	 //goal = 50;//For emulator testing
	 percentage = (goalprogress_buffer *100) / goal;
	 degrees = percentage*3.6;
	 if(percentage==0){
		}else if(percentage<=25){
			graphics_context_set_stroke_color(ctx, GColorBlack);
			graphics_draw_arc(ctx, center, 72, 12, 270, 270+degrees);
			#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, GColorOrange);
   #elif PBL_BW
    graphics_context_set_stroke_color(ctx, GColorWhite);
   #endif
	  graphics_draw_arc(ctx, center, 71, 9, 270, 270+degrees);
		} else if (percentage>25 && percentage<100) {
			graphics_context_set_stroke_color(ctx, GColorBlack);
			graphics_draw_arc(ctx, center, 72, 12, 270, 360);
			graphics_draw_arc(ctx, center, 72, 12, 0, degrees-90);
			#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, GColorOrange);
   #elif PBL_BW
    graphics_context_set_stroke_color(ctx, GColorWhite);
   #endif
			graphics_draw_arc(ctx, center, 71, 9, 270, 360);
			graphics_draw_arc(ctx, center, 71, 9, 0, degrees-90);
		} else if (percentage>=100){
			graphics_context_set_stroke_color(ctx, GColorBlack);
			graphics_draw_arc(ctx, center, 72, 12, 0, 360);
			#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, GColorYellow);
   #elif PBL_BW
    graphics_context_set_stroke_color(ctx, GColorWhite);
   #endif
			graphics_draw_arc(ctx, center, 71, 9, 0, 360);
		}

}

//Appmessage debugging
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

//Battery icon, hides when there is more than charge that 20% and unhides when there isn't
static void batt_handler(BatteryChargeState charge_state) {
	 if (charge_state.charge_percent > 20) {
			 layer_set_hidden(bitmap_layer_get_layer(s_lowbatt_layer), true);
  } else {
			 layer_set_hidden(bitmap_layer_get_layer(s_lowbatt_layer), false);
  }
}

static void bt_handler(bool connected) {
  if (connected) { // Hide the disconnect icon if there is an active connection
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone is connected.");
			 layer_set_hidden(bitmap_layer_get_layer(s_nobt_layer), true);
			 if(init_complete){
			 vibes_short_pulse();
				}
  } else { // Unhide the disconnect icon if there is an active connection
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone is not connected.");
			 layer_set_hidden(bitmap_layer_get_layer(s_nobt_layer), false);
			 if(init_complete){
			 vibes_long_pulse();
				}
  }
}

void timer_callback(void *data) {
	   layer_set_hidden(text_layer_get_layer(s_month_layer), true);
	   layer_set_hidden(text_layer_get_layer(s_totals_layer), true);
	   layer_set_hidden(text_layer_get_layer(s_monthmiles_layer), true);
	   layer_set_hidden(text_layer_get_layer(s_monthelevation_layer), true);
	   layer_set_hidden(text_layer_get_layer(s_totalmiles_layer), true);
	   layer_set_hidden(text_layer_get_layer(s_totalelevation_layer), true);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
	   layer_set_hidden(text_layer_get_layer(s_month_layer), false);
	   layer_set_hidden(text_layer_get_layer(s_totals_layer), false);
	   layer_set_hidden(text_layer_get_layer(s_monthmiles_layer), false);
	   layer_set_hidden(text_layer_get_layer(s_monthelevation_layer), false);
	   layer_set_hidden(text_layer_get_layer(s_totalmiles_layer), false);
	   layer_set_hidden(text_layer_get_layer(s_totalelevation_layer), false);
	   timer = app_timer_register(6000, (AppTimerCallback) timer_callback, NULL);
}

static void main_window_load(Window *window) {
  // Create GBitmap, then set to created BitmapLayer
	 s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BG_IMAGE);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
	
	 // Create lowbatt icon
	 s_lowbatt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LOWBATT);
  s_lowbatt_layer = bitmap_layer_create(GRect(22, 78, 19, 10));
  bitmap_layer_set_bitmap(s_lowbatt_layer, s_lowbatt_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_lowbatt_layer));
 	batt_handler(battery_state_service_peek());

		// Create nobt icon
	 s_nobt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NOBT);
  s_nobt_layer = bitmap_layer_create(GRect(104, 78, 19, 11));
  bitmap_layer_set_bitmap(s_nobt_layer, s_nobt_bitmap);
	 layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_nobt_layer));
	 bt_handler(bluetooth_connection_service_peek());
	
		// Create hour and minute layers
	
		s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_NORMAFIXED_REGULAR_60));
	
  s_hour_layer = text_layer_create(GRect(0, 10, 144, 62));
  text_layer_set_background_color(s_hour_layer, GColorClear);
	 text_layer_set_text_color(s_hour_layer, GColorWhite);
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentCenter);
	 layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_hour_layer));
  text_layer_set_font(s_hour_layer, s_time_font);
	
	 s_min_layer = text_layer_create(GRect(0, 72, 144, 62));
  text_layer_set_background_color(s_min_layer, GColorClear);
	 text_layer_set_text_color(s_min_layer, GColorWhite);
  text_layer_set_text_alignment(s_min_layer, GTextAlignmentCenter);
	 layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_min_layer));
  text_layer_set_font(s_min_layer, s_time_font);
	
		// Create date layer
  s_date_layer = text_layer_create(GRect(0, 71, 144, 22));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	 layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
	 text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	
		// Create month layer
  s_month_layer = text_layer_create(GRect(0, 152, 144, 22));
  text_layer_set_background_color(s_month_layer, GColorClear);
  text_layer_set_text_color(s_month_layer, GColorWhite);
  text_layer_set_text_alignment(s_month_layer, GTextAlignmentCenter);
	 layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_month_layer));
	 text_layer_set_font(s_month_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	 layer_set_hidden(text_layer_get_layer(s_month_layer), true);
	
		// Create totals layer
  s_totals_layer = text_layer_create(GRect(0, -4, 144, 14));
  text_layer_set_background_color(s_totals_layer, GColorClear);
  text_layer_set_text_color(s_totals_layer, GColorWhite);
  text_layer_set_text_alignment(s_totals_layer, GTextAlignmentCenter);
	 layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_totals_layer));
	 text_layer_set_font(s_totals_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	 text_layer_set_text(s_totals_layer, "All");
	 layer_set_hidden(text_layer_get_layer(s_totals_layer), true);

  // Create monthly miles Layer
  s_monthmiles_layer = text_layer_create(GRect(0, 152, 67, 25));
  text_layer_set_background_color(s_monthmiles_layer, GColorClear);
  text_layer_set_text_color(s_monthmiles_layer, GColorWhite);
  text_layer_set_text_alignment(s_monthmiles_layer, GTextAlignmentLeft);
	 if (persist_exists(KEY_MONTHMILES)) { //If a value is stored in persistent storgae use it, otherwise add temporary text.
			persist_read_string(KEY_MONTHMILES, monthmiles_buffer, sizeof(monthmiles_buffer));
		 text_layer_set_text(s_monthmiles_layer, monthmiles_buffer);
  } else {
			text_layer_set_text(s_monthmiles_layer, "-");
		}
	 text_layer_set_font(s_monthmiles_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_monthmiles_layer));
	 layer_set_hidden(text_layer_get_layer(s_monthmiles_layer), true);
	
  // Create monthly elevation Layer
  s_monthelevation_layer = text_layer_create(GRect(79, 152, 65, 25));
  text_layer_set_background_color(s_monthelevation_layer, GColorClear);
  text_layer_set_text_color(s_monthelevation_layer, GColorWhite);
  text_layer_set_text_alignment(s_monthelevation_layer, GTextAlignmentRight);
	 if (persist_exists(KEY_MONTHELEVATION)) { //If a value is stored in persistent storgae use it, otherwise add temporary text.
			persist_read_string(KEY_MONTHELEVATION, monthelevation_buffer, sizeof(monthelevation_buffer));
		 text_layer_set_text(s_monthelevation_layer, monthelevation_buffer);
  } else {
			text_layer_set_text(s_monthelevation_layer, "-");
		}
	 text_layer_set_font(s_monthelevation_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_monthelevation_layer));
	 layer_set_hidden(text_layer_get_layer(s_monthelevation_layer), true);
	
  // Create total miles Layer
  s_totalmiles_layer = text_layer_create(GRect(0, -4, 67, 25));
  text_layer_set_background_color(s_totalmiles_layer, GColorClear);
  text_layer_set_text_color(s_totalmiles_layer, GColorWhite);
  text_layer_set_text_alignment(s_totalmiles_layer, GTextAlignmentLeft);
	 if (persist_exists(KEY_TOTALMILES)) { //If a value is stored in persistent storgae use it, otherwise add temporary text.
			persist_read_string(KEY_TOTALMILES, totalmiles_buffer, sizeof(totalmiles_buffer));
		 text_layer_set_text(s_totalmiles_layer, totalmiles_buffer);
  } else {
			text_layer_set_text(s_totalmiles_layer, "-");
		}
	 text_layer_set_font(s_totalmiles_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_totalmiles_layer));
	 layer_set_hidden(text_layer_get_layer(s_totalmiles_layer), true);
	
  // Create total elevation Layer
  s_totalelevation_layer = text_layer_create(GRect(79, -4, 65, 25));
  text_layer_set_background_color(s_totalelevation_layer, GColorClear);
  text_layer_set_text_color(s_totalelevation_layer, GColorWhite);
  text_layer_set_text_alignment(s_totalelevation_layer, GTextAlignmentRight);
	 if (persist_exists(KEY_TOTALELEVATION)) { //If a value is stored in persistent storage use it, otherwise add temporary text.
			persist_read_string(KEY_TOTALELEVATION, totalelevation_buffer, sizeof(totalelevation_buffer));
		 text_layer_set_text(s_totalelevation_layer, totalelevation_buffer);
  } else {
			text_layer_set_text(s_totalelevation_layer, "-");
		}
	 text_layer_set_font(s_totalelevation_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_totalelevation_layer));
	 layer_set_hidden(text_layer_get_layer(s_totalelevation_layer), true);
	
	 // Create Arc Layer
		Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  arc_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_add_child(window_layer, arc_layer);
  layer_set_update_proc(arc_layer, arc_update_proc);
				
	 // Make sure the time is displayed from the start
	 update_time();
	 
	 // Register bluetooth and battery event listeners
	 bluetooth_connection_service_subscribe(bt_handler);
	 battery_state_service_subscribe(batt_handler);
	 accel_tap_service_subscribe(tap_handler);
}

static void main_window_unload(Window *window) {
  // Destroy text layers
		text_layer_destroy(s_hour_layer);
	 text_layer_destroy(s_min_layer);
	 text_layer_destroy(s_date_layer);
		text_layer_destroy(s_month_layer);
	 text_layer_destroy(s_totals_layer);

	 //Destroy bitmap tings
	 gbitmap_destroy(s_lowbatt_bitmap);
	 gbitmap_destroy(s_nobt_bitmap);
	 bitmap_layer_destroy(s_lowbatt_layer);
	 bitmap_layer_destroy(s_nobt_layer);
 	gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
	
	 //Destroy arc
	 layer_destroy(arc_layer);
	
	 // Destroy stats elements
  text_layer_destroy(s_monthmiles_layer);
  text_layer_destroy(s_monthelevation_layer);
  text_layer_destroy(s_totalmiles_layer);
  text_layer_destroy(s_totalelevation_layer);
	
	 battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
	 accel_tap_service_unsubscribe();
}

static void init() {
		// Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
	
	 // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	 // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
	
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
  //Set init wait variable to true, used to prevent watch vibrating when watch face is first loaded
	 init_complete = 1;
}


static void deinit() {
		// Destroy Window
	 persist_write_string(KEY_MONTHMILES, monthmiles_buffer);
 	persist_write_string(KEY_MONTHELEVATION, monthelevation_buffer);
 	persist_write_string(KEY_TOTALMILES, totalmiles_buffer);
 	persist_write_string(KEY_TOTALELEVATION, totalelevation_buffer);
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

