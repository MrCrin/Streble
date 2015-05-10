#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
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


enum {
  KEY_MONTHMILES = 0,
  KEY_MONTHELEVATION = 1,
  KEY_TOTALMILES = 2,
  KEY_TOTALELEVATION = 3,
};

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create time and date buffer
  static char buffer[] = "00:00";
  static char datebuffer[] = "WEDNESDAY 00";
	 static char monthbuffer[] = "SEPTEMBER TOTALS";	

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
	
		//Set date and display on date layer
		strftime(datebuffer, sizeof(datebuffer), "%A %d", tick_time);
		text_layer_set_text(s_date_layer, datebuffer);
	
		//Set month and display on month layer
		strftime(monthbuffer, sizeof(monthbuffer), "%B Totals", tick_time);
		text_layer_set_text(s_month_layer, monthbuffer);
	
  // Display this time on time layer
  text_layer_set_text(s_time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	  update_time();
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  (void) context;
	 static char monthmiles_buffer[32];
	 static char monthelevation_buffer[32];
		static char totalmiles_buffer[32];
		static char totalelevation_buffer[32];

	 void process_tuple(Tuple *t) {
		
  //Get key
  int key = t->key;
 
  //Get string value
  char string_value[32];
  strcpy(string_value, t->value->cstring);
 
  //Decide what to do
  switch(key) {
    case KEY_MONTHMILES:
      snprintf(monthmiles_buffer, sizeof("000000 mi"), "%s mi", string_value);
      text_layer_set_text(s_monthmiles_layer, monthmiles_buffer);
			 case KEY_MONTHELEVATION:
      snprintf(monthelevation_buffer, sizeof("ft 000000"), "ft %s", string_value);
      text_layer_set_text(s_monthelevation_layer, monthelevation_buffer);
			 case KEY_TOTALMILES:
      snprintf(totalmiles_buffer, sizeof("000000 mi"), "%s mi", string_value);
      text_layer_set_text(s_totalmiles_layer, totalmiles_buffer);
    case KEY_TOTALELEVATION:
      snprintf(totalelevation_buffer, sizeof("ft 000000"), "ft %s", string_value);
      text_layer_set_text(s_totalelevation_layer, totalelevation_buffer);

      break;
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

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void batt_handler(BatteryChargeState charge_state) {
	 if (charge_state.charge_percent > 20) {
			 layer_set_hidden(bitmap_layer_get_layer(s_lowbatt_layer), true);
  } else {
			 layer_set_hidden(bitmap_layer_get_layer(s_lowbatt_layer), false);
  }
}

static void bt_handler(bool connected) {
  if (connected) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone is connected!");
			 layer_set_hidden(bitmap_layer_get_layer(s_nobt_layer), true);
			 vibes_short_pulse();
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone is not connected!");
			 layer_set_hidden(bitmap_layer_get_layer(s_nobt_layer), false);
			 vibes_long_pulse();
  }
}

static void main_window_load(Window *window) {
  // Create GBitmap, then set to created BitmapLayer
	 s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BG_IMAGE);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
	
	 // Create lowbatt icon
	 s_lowbatt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LOWBATT);
  s_lowbatt_layer = bitmap_layer_create(GRect(12, 48, 19, 10));
  bitmap_layer_set_bitmap(s_lowbatt_layer, s_lowbatt_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_lowbatt_layer));
 	batt_handler(battery_state_service_peek());
	
		// Create nobt icon
	 s_nobt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NOBT);
  s_nobt_layer = bitmap_layer_create(GRect(115, 45, 19, 11));
  bitmap_layer_set_bitmap(s_nobt_layer, s_nobt_bitmap);
	 layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_nobt_layer));
	 bt_handler(bluetooth_connection_service_peek());
	
		// Create time layer
  s_time_layer = text_layer_create(GRect(-1, 58, 148, 52));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	 layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
		s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_THEBOLDFONT_REGULAR_42));
  text_layer_set_font(s_time_layer, s_time_font);
	
		// Create date layer
  s_date_layer = text_layer_create(GRect(-2, 103, 146, 22));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	 layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
	 text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	
		// Create month layer
  s_month_layer = text_layer_create(GRect(-2, 126, 146, 22));
  text_layer_set_background_color(s_month_layer, GColorClear);
  text_layer_set_text_color(s_month_layer, GColorWhite);
  text_layer_set_text_alignment(s_month_layer, GTextAlignmentCenter);
	 layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_month_layer));
	 text_layer_set_font(s_month_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	
		// Create totals layer
  s_totals_layer = text_layer_create(GRect(-2, 16, 146, 22));
  text_layer_set_background_color(s_totals_layer, GColorClear);
  text_layer_set_text_color(s_totals_layer, GColorWhite);
  text_layer_set_text_alignment(s_totals_layer, GTextAlignmentCenter);
	 layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_totals_layer));
	 text_layer_set_font(s_totals_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	 text_layer_set_text(s_totals_layer, "All-Time Totals");
		
	 // Make sure the time is displayed from the start
	 update_time();
	
  // Create monthly miles Layer
  s_monthmiles_layer = text_layer_create(GRect(0, 145, 67, 25));
  text_layer_set_background_color(s_monthmiles_layer, GColorClear);
  text_layer_set_text_color(s_monthmiles_layer, GColorBlack);
  text_layer_set_text_alignment(s_monthmiles_layer, GTextAlignmentRight);
  text_layer_set_text(s_monthmiles_layer, "-");
	 text_layer_set_font(s_monthmiles_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_monthmiles_layer));
	
  // Create monthly elevation Layer
  s_monthelevation_layer = text_layer_create(GRect(76, 145, 65, 25));
  text_layer_set_background_color(s_monthelevation_layer, GColorClear);
  text_layer_set_text_color(s_monthelevation_layer, GColorBlack);
  text_layer_set_text_alignment(s_monthelevation_layer, GTextAlignmentLeft);
  text_layer_set_text(s_monthelevation_layer, "-");
	 text_layer_set_font(s_monthelevation_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_monthelevation_layer));
	
  // Create total miles Layer
  s_totalmiles_layer = text_layer_create(GRect(0, -2, 67, 25));
  text_layer_set_background_color(s_totalmiles_layer, GColorClear);
  text_layer_set_text_color(s_totalmiles_layer, GColorBlack);
  text_layer_set_text_alignment(s_totalmiles_layer, GTextAlignmentRight);
  text_layer_set_text(s_totalmiles_layer, "-");
	 text_layer_set_font(s_totalmiles_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_totalmiles_layer));
	
  // Create total elevation Layer
  s_totalelevation_layer = text_layer_create(GRect(76, -2, 65, 25));
  text_layer_set_background_color(s_totalelevation_layer, GColorClear);
  text_layer_set_text_color(s_totalelevation_layer, GColorBlack);
  text_layer_set_text_alignment(s_totalelevation_layer, GTextAlignmentLeft);
  text_layer_set_text(s_totalelevation_layer, "-");
	 text_layer_set_font(s_totalelevation_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_totalelevation_layer));
	 
	 // Register bluetooth and battery event listeners
	 bluetooth_connection_service_subscribe(bt_handler);
	 battery_state_service_subscribe(batt_handler);
}



static void main_window_unload(Window *window) {
  // Destroy TextLayer
		text_layer_destroy(s_time_layer);
	
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
	
  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
	
	 // Destroy stats elements
  text_layer_destroy(s_monthmiles_layer);
  text_layer_destroy(s_monthelevation_layer);
  text_layer_destroy(s_totalmiles_layer);
  text_layer_destroy(s_totalelevation_layer);
	
	 battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
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
}


static void deinit() {
		// Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

