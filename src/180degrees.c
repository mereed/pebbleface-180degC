/*
Copyright (C) 2014 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "pebble.h"
#include "effect_layer.h"

EffectLayer* effect_layer;
EffectLayer* effect_layer_2;

static AppSync sync;
static uint8_t sync_buffer[128];

static int invert;
static int bluetoothvibe;
static int hourlyvibe;
static int hidesec;
//static int hidedate;
static int hide_batt;
static int hide_date;
static int hide_weather;

static bool appStarted = false;

static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_CLEAR_DAY,
  RESOURCE_ID_CLEAR_NIGHT,
  RESOURCE_ID_WINDY,
  RESOURCE_ID_COLD,
  RESOURCE_ID_PARTLY_CLOUDY_DAY,
  RESOURCE_ID_PARTLY_CLOUDY_NIGHT,
  RESOURCE_ID_HAZE,
  RESOURCE_ID_CLOUD,
  RESOURCE_ID_RAIN,
  RESOURCE_ID_SNOW,
  RESOURCE_ID_HAIL,
  RESOURCE_ID_CLOUDY,
  RESOURCE_ID_STORM,
  RESOURCE_ID_FOG,
  RESOURCE_ID_NA,
};

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,
  INVERT_COLOR_KEY = 0x1,   
  BLUETOOTHVIBE_KEY = 0x2,
  HOURLYVIBE_KEY = 0x3,
  STYLE_KEY = 0x4,
  HIDE_BATT_KEY = 0x5,
  HIDE_DATE_KEY = 0x6,
  HIDE_WEATHER_KEY = 0x7,
  WEATHER_TEMPERATURE_KEY = 0x8
};

static Window *window;
static Layer *window_layer;
static Layer *weather_holder;

BitmapLayer *icon_layer;
GBitmap *icon_bitmap = NULL;
TextLayer *temp_layer;

BitmapLayer *layer_batt_img;
GBitmap *img_battery_100;
GBitmap *img_battery_90;
GBitmap *img_battery_80;
GBitmap *img_battery_70;
GBitmap *img_battery_60;
GBitmap *img_battery_50;
GBitmap *img_battery_40;
GBitmap *img_battery_30;
GBitmap *img_battery_20;
GBitmap *img_battery_10;
GBitmap *img_battery_charge;
int charge_percent = 0;

static GBitmap *separator_image;
static BitmapLayer *separator_layer;

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

#define TOTAL_DATE_DIGITS 2	
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

#define TOTAL_TIME_DIGITS 4
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

const int TINY_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_TINY_0,
  RESOURCE_ID_IMAGE_TINY_1,
  RESOURCE_ID_IMAGE_TINY_2,
  RESOURCE_ID_IMAGE_TINY_3,
  RESOURCE_ID_IMAGE_TINY_4,
  RESOURCE_ID_IMAGE_TINY_5,
  RESOURCE_ID_IMAGE_TINY_6,
  RESOURCE_ID_IMAGE_TINY_7,
  RESOURCE_ID_IMAGE_TINY_8,
  RESOURCE_ID_IMAGE_TINY_9
};

static GBitmap *month_image;
static BitmapLayer *month_layer;

const int MONTH_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_MONTH_JAN,
  RESOURCE_ID_IMAGE_MONTH_FEB,
  RESOURCE_ID_IMAGE_MONTH_MAR,
  RESOURCE_ID_IMAGE_MONTH_APR,
  RESOURCE_ID_IMAGE_MONTH_MAY,
  RESOURCE_ID_IMAGE_MONTH_JUN,
  RESOURCE_ID_IMAGE_MONTH_JUL,
  RESOURCE_ID_IMAGE_MONTH_AUG,
  RESOURCE_ID_IMAGE_MONTH_SEP,
  RESOURCE_ID_IMAGE_MONTH_OCT,
  RESOURCE_ID_IMAGE_MONTH_NOV,
  RESOURCE_ID_IMAGE_MONTH_DEC
};

#define TOTAL_SECONDS_DIGITS 2
static GBitmap *seconds_digits_images[TOTAL_SECONDS_DIGITS];
static BitmapLayer *seconds_digits_layers[TOTAL_SECONDS_DIGITS];

const int TINYSEC_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_TINYSEC_0,
  RESOURCE_ID_IMAGE_TINYSEC_1,
  RESOURCE_ID_IMAGE_TINYSEC_2,
  RESOURCE_ID_IMAGE_TINYSEC_3,
  RESOURCE_ID_IMAGE_TINYSEC_4,
  RESOURCE_ID_IMAGE_TINYSEC_5,
  RESOURCE_ID_IMAGE_TINYSEC_6,
  RESOURCE_ID_IMAGE_TINYSEC_7,
  RESOURCE_ID_IMAGE_TINYSEC_8,
  RESOURCE_ID_IMAGE_TINYSEC_9
};

static GFont *custom_font;

//InverterLayer *inverter_layer = NULL;


void set_invert_color(bool invert) {
  if (invert && effect_layer == NULL) {
    // Add inverter layer
    Layer *window_layer = window_get_root_layer(window);

    effect_layer = effect_layer_create(GRect(53, 2, 65, 166));
    layer_add_child(window_layer, effect_layer_get_layer(effect_layer));
    effect_layer_add_effect(effect_layer, effect_invert, NULL);
	
  } else if (!invert && effect_layer != NULL) {
    // Remove Inverter layer
   layer_remove_from_parent(effect_layer_get_layer(effect_layer));
    effect_layer_destroy(effect_layer);
    effect_layer = NULL;
  }
  // No action required
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

void hide_date_now(bool hide_date) {
	
	if (hide_date) {
		layer_set_hidden(bitmap_layer_get_layer(day_name_layer), true);
		layer_set_hidden(bitmap_layer_get_layer(month_layer), true);
		
		for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[i]), true);
			}
	} else {
		layer_set_hidden(bitmap_layer_get_layer(day_name_layer), false);
		layer_set_hidden(bitmap_layer_get_layer(month_layer), false);
		
		for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        layer_set_hidden( bitmap_layer_get_layer(date_digits_layers[i]), false);
			}
	}
}

void hide_weather_now(bool hide_weather) {
	
	if (hide_weather) {
		layer_set_hidden(bitmap_layer_get_layer(icon_layer), true);
		layer_set_hidden(text_layer_get_layer(temp_layer), true);
		
	} else {
		layer_set_hidden(bitmap_layer_get_layer(icon_layer), false);
		layer_set_hidden(text_layer_get_layer(temp_layer), false);
	}
}

void hide_batt_now(bool hide_batt) {
	
	if (hide_batt) {
		
		layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), true);
		//layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), true);
		   tick_timer_service_unsubscribe();

			for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
        layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[i]), false);
			}
		        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

	} else {
		
		layer_set_hidden(bitmap_layer_get_layer(layer_batt_img), false);
		//layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), false);
		    tick_timer_service_unsubscribe();

			for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
        layer_set_hidden(bitmap_layer_get_layer(seconds_digits_layers[i]), true);
			} 
		        tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);

	}
}

static void sync_tuple_changed_callback(const uint32_t key,
                                        const Tuple* new_tuple,
                                        const Tuple* old_tuple,
                                        void* context) {

  // App Sync keeps new_tuple in sync_buffer, so we may use it directly
  switch (key) {
    case WEATHER_ICON_KEY:
    if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }

      icon_bitmap = gbitmap_create_with_resource(
          WEATHER_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;
	  
	 case WEATHER_TEMPERATURE_KEY:
      text_layer_set_text(temp_layer, new_tuple->value->cstring);
      break;
	  
	  
    case INVERT_COLOR_KEY:
      invert = new_tuple->value->uint8 != 0;
	  persist_write_bool(INVERT_COLOR_KEY, invert);
      set_invert_color(invert);
      break;
	  
    case BLUETOOTHVIBE_KEY:
      bluetoothvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(BLUETOOTHVIBE_KEY, bluetoothvibe);
      break;      
	  
    case HOURLYVIBE_KEY:
      hourlyvibe = new_tuple->value->uint8 != 0;
	  persist_write_bool(HOURLYVIBE_KEY, hourlyvibe);	  
      break;	  
	
	case STYLE_KEY:
      hidesec = new_tuple->value->uint8 != 0;
	  persist_write_bool(STYLE_KEY, hidesec);	  
	 //hide_batt_now(hidesec);
      break; 
	  
	case HIDE_BATT_KEY:
	  hide_batt = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_BATT_KEY, hide_batt);
	  hide_batt_now(hide_batt);
	  break;
	  
	case HIDE_DATE_KEY:
      hide_date = new_tuple->value->uint8 != 0;
	  persist_write_bool(HIDE_DATE_KEY, hide_date);	  
	  hide_date_now(hide_date);
	  break;
	  
    case HIDE_WEATHER_KEY:
      hide_weather = new_tuple->value->uint8 !=0;
	  persist_write_bool(HIDE_WEATHER_KEY, hide_weather);	  
	  hide_weather_now(hide_weather);
	  break;
  }
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;

  *bmp_image = gbitmap_create_with_resource(resource_id);
#ifdef PBL_PLATFORM_BASALT
  GRect bitmap_bounds = gbitmap_get_bounds((*bmp_image));
#else
  GRect bitmap_bounds = (*bmp_image)->bounds;
#endif
  GRect frame = GRect(origin.x, origin.y, bitmap_bounds.size.w, bitmap_bounds.size.h);
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);

  if (old_image != NULL) {
  	gbitmap_destroy(old_image);
  }
}

void handle_battery(BatteryChargeState charge_state) {

    if (charge_state.is_charging) {
        bitmap_layer_set_bitmap(layer_batt_img, img_battery_charge);

    } else {
		if (charge_state.charge_percent <= 10) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_10);
        } else if (charge_state.charge_percent <= 20) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_20);
        } else if (charge_state.charge_percent <= 30) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_30);
		} else if (charge_state.charge_percent <= 40) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_40);
		} else if (charge_state.charge_percent <= 50) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_50);
    	} else if (charge_state.charge_percent <= 60) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_60);	
        } else if (charge_state.charge_percent <= 70) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_70);
		} else if (charge_state.charge_percent <= 80) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_80);
		} else if (charge_state.charge_percent <= 90) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_90);
		} else if (charge_state.charge_percent <= 100) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);			
            
        if (charge_state.charge_percent < charge_percent) {
            if (charge_state.charge_percent==20){
                vibes_double_pulse();
            } else if(charge_state.charge_percent==10){
                vibes_long_pulse();
            }
        }
    }
    charge_percent = charge_state.charge_percent;
  }
}

void handle_bluetooth(bool connected) {
 if(appStarted && !connected && bluetoothvibe) {
    //vibe!
    vibes_long_pulse();
  }
   layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), !connected);	
}

void force_update(void) {
    handle_battery(battery_state_service_peek());
    handle_bluetooth(bluetooth_connection_service_peek());
}

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_days(struct tm *tick_time) {
  set_container_image(&month_image, month_layer, MONTH_IMAGE_RESOURCE_IDS[tick_time->tm_mon], GPoint(34, 60));

  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint( 7, 60));
  set_container_image(&date_digits_images[0], date_digits_layers[0], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(7, 30));
  set_container_image(&date_digits_images[1], date_digits_layers[1], TINY_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(7, 5));
}

static void update_hours(struct tm *tick_time) {
	
   if (appStarted && hourlyvibe) {
    //vibe!
    vibes_short_pulse();
  } 
   unsigned short display_hour = get_display_hour(tick_time->tm_hour);

  set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(56, 130));
  set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(56, 93));
	
  if (!clock_is_24h_style() ) { 
   if (display_hour/10 == 0) {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
    }
    else {
      layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
    }
  }
}

static void update_minutes(struct tm *tick_time) {
  set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(56, 42));
  set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(56, 5));
}


static void update_seconds(struct tm *tick_time) {

  set_container_image(&seconds_digits_images[0], seconds_digits_layers[0], TINYSEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec/10], GPoint(125, 15));
  set_container_image(&seconds_digits_images[1], seconds_digits_layers[1], TINYSEC_IMAGE_RESOURCE_IDS[tick_time->tm_sec%10], GPoint(125, 5));
	
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {

  if (units_changed & DAY_UNIT) {
    update_days(tick_time);
  }
  if (units_changed & HOUR_UNIT) {
    update_hours(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
    update_minutes(tick_time);
  }	
  if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }		  
}

static void init(void) {
  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));

  memset(&date_digits_layers, 0, sizeof(date_digits_layers));
  memset(&date_digits_images, 0, sizeof(date_digits_images));

 // Setup messaging
  const int inbound_size = 128;
  const int outbound_size = 128;
  app_message_open(inbound_size, outbound_size);	
	
  window = window_create();
  if (window == NULL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }
	
  window_set_background_color(window, GColorBlack);

	
  window_stack_push(window, true /* Animated */);
  window_layer = window_get_root_layer(window);
	
	// resources
  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARE_26));

    img_battery_100   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_090_100);
    img_battery_90   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_080_090);
    img_battery_80   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_070_080);
    img_battery_70   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_060_070);
    img_battery_60   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_050_060);
    img_battery_50   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_040_050);
    img_battery_40   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_030_040);
    img_battery_30    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_020_030);
    img_battery_20    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_010_020);
    img_battery_10    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_000_010);
    img_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_CHARGING);

    // layers
    layer_batt_img  = bitmap_layer_create(GRect(132, 5, 9, 98));
    bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
	layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));
	
  separator_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SEPARATOR);
#ifdef PBL_PLATFORM_BASALT
  GRect bitmap_bounds = gbitmap_get_bounds(separator_image);
#else
  GRect bitmap_bounds = separator_image->bounds;
#endif	
  GRect frame = GRect(68, 81, bitmap_bounds.size.w, bitmap_bounds.size.h);
  separator_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(separator_layer, separator_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(separator_layer));   

  bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
#ifdef PBL_PLATFORM_BASALT
  GRect bitmap_bounds2 = gbitmap_get_bounds(bluetooth_image);
#else
  GRect bitmap_bounds2 = bluetooth_image->bounds;
#endif	
  GRect frame2 = GRect(128, 112, bitmap_bounds2.size.w, bitmap_bounds2.size.h);
  bluetooth_layer = bitmap_layer_create(frame2);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));

  Layer *weather_holder = layer_create(GRect(0, 0, 144, 168 ));
  layer_add_child(window_layer, weather_holder);

  icon_layer = bitmap_layer_create(GRect(-3, 100, 60, 75));
  layer_add_child(weather_holder, bitmap_layer_get_layer(icon_layer));
 	
  // Create time and date layers
  GRect dummy_frame = { {0, 0}, {0, 0} };	
   day_name_layer = bitmap_layer_create(dummy_frame);
   layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));	
		
   month_layer = bitmap_layer_create(dummy_frame);
   layer_add_child(window_layer, bitmap_layer_get_layer(month_layer));	
	
    for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }
	
    for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
  }
	
  for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    seconds_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(seconds_digits_layers[i]));
  }

  temp_layer = text_layer_create(GRect(120, 135, 40, 40));
  text_layer_set_text_color(temp_layer, GColorWhite);
#ifdef PBL_COLOR	
		text_layer_set_text_color(temp_layer, GColorVividCerulean     );			
#endif
  text_layer_set_background_color(temp_layer, GColorClear);
 // text_layer_set_font(temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_font(temp_layer, custom_font);
  text_layer_set_text_alignment(temp_layer, GTextAlignmentLeft);
  layer_add_child(weather_holder, text_layer_get_layer(temp_layer));
	
 // creating effect layer to rotate temp
  effect_layer_2 = effect_layer_create(GRect(115,135,33,33));
  effect_layer_add_effect(effect_layer_2, effect_rotate_90_degrees, (void *)false);
  layer_add_child(window_get_root_layer(window), effect_layer_get_layer(effect_layer_2));
	
Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 14),
    TupletInteger(INVERT_COLOR_KEY, persist_read_bool(INVERT_COLOR_KEY)),
    TupletInteger(BLUETOOTHVIBE_KEY, persist_read_bool(BLUETOOTHVIBE_KEY)),
    TupletInteger(STYLE_KEY, persist_read_bool(STYLE_KEY)),
    TupletInteger(HOURLYVIBE_KEY, persist_read_bool(HOURLYVIBE_KEY)),
    TupletInteger(HIDE_BATT_KEY, persist_read_bool(HIDE_BATT_KEY)),
	TupletInteger(HIDE_DATE_KEY, persist_read_bool(HIDE_DATE_KEY)),
	TupletInteger(HIDE_WEATHER_KEY, persist_read_bool(HIDE_WEATHER_KEY)),
	TupletCString(WEATHER_TEMPERATURE_KEY, ""),

  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                ARRAY_LENGTH(initial_values), sync_tuple_changed_callback,
                NULL, NULL);

  appStarted = true;

  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  
  handle_tick(tick_time, MONTH_UNIT + DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);

  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);	

	 // handlers
    battery_state_service_subscribe(&handle_battery);
    bluetooth_connection_service_subscribe(&handle_bluetooth);
	
	 // draw first frame
    force_update();

}

static void deinit(void) {
  app_sync_deinit(&sync);
  
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();

  layer_remove_from_parent(bitmap_layer_get_layer(separator_layer));
  bitmap_layer_destroy(separator_layer);
  gbitmap_destroy(separator_image);
	
  layer_remove_from_parent(bitmap_layer_get_layer(bluetooth_layer));
  bitmap_layer_destroy(bluetooth_layer);
  gbitmap_destroy(bluetooth_image);
  
  bitmap_layer_destroy(layer_batt_img);
  gbitmap_destroy(img_battery_100);
  gbitmap_destroy(img_battery_90);
  gbitmap_destroy(img_battery_80);
  gbitmap_destroy(img_battery_70);
  gbitmap_destroy(img_battery_60);
  gbitmap_destroy(img_battery_50);
  gbitmap_destroy(img_battery_40);
  gbitmap_destroy(img_battery_30);
  gbitmap_destroy(img_battery_20);
  gbitmap_destroy(img_battery_10);
  gbitmap_destroy(img_battery_charge);	  
	
  fonts_unload_custom_font(custom_font);

  layer_remove_from_parent(bitmap_layer_get_layer(month_layer));
  bitmap_layer_destroy(month_layer);
  gbitmap_destroy(month_image);
		
  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
	
  layer_remove_from_parent(bitmap_layer_get_layer(icon_layer));
  bitmap_layer_destroy(icon_layer);
  gbitmap_destroy(icon_bitmap);
	
  effect_layer_destroy(effect_layer_2);	

  if (effect_layer != NULL) {
	  effect_layer_destroy(effect_layer);
  }

  text_layer_destroy( temp_layer );

  layer_destroy(weather_holder);
	
	for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    gbitmap_destroy(date_digits_images[i]);
    bitmap_layer_destroy(date_digits_layers[i]);
  }

	for (int i = 0; i < TOTAL_SECONDS_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(seconds_digits_layers[i]));
    gbitmap_destroy(seconds_digits_images[i]);
    bitmap_layer_destroy(seconds_digits_layers[i]);
  }
	
   for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    bitmap_layer_destroy(time_digits_layers[i]);
  } 

	  window_destroy(window);

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}