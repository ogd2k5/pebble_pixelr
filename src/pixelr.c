#include <pebble.h>
#include "pixelr.h"

// Defining Global Variables
Window *window;
TextLayer *hours_layer;
TextLayer *mins_layer;
TextLayer *secs_layer;
TextLayer *day_layer;
TextLayer *month_layer;
TextLayer *year_layer;
TextLayer *battery_layer;
static GFont *PixelFont34;
static GFont *MiniPixelFont10;
static GBitmap *bluetooth_connected_image;
static GBitmap *bluetooth_disconnected_image;
static BitmapLayer *bluetooth_layer;

// Defining Functions
static void handle_second_tick(struct tm*, TimeUnits);
static void handle_battery(BatteryChargeState);
static void handle_bluetooth(bool);

static void do_init(void)
{
	window = window_create();
	window_stack_push(window, true);
	window_set_background_color(window, GColorBlack);
	Layer *root_layer = window_get_root_layer(window);
	PixelFont34 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIXEL_FONT_34));
	MiniPixelFont10 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MINI_PIXEL_10));

	// Hours layer
	hours_layer = text_layer_create(GRect(5, 40, 40 /* width */, 37 /* height */));
	text_layer_set_text_color(hours_layer, GColorWhite);
	text_layer_set_background_color(hours_layer, GColorClear);
	text_layer_set_font(hours_layer, PixelFont34);
	text_layer_set_text_alignment(hours_layer, GTextAlignmentCenter);

	// Minutes layer
	mins_layer = text_layer_create(GRect(5, 85, 40 /* width */, 37 /* height */));
	text_layer_set_text_color(mins_layer, GColorWhite);
	text_layer_set_background_color(mins_layer, GColorClear);
	text_layer_set_font(mins_layer, PixelFont34);
	text_layer_set_text_alignment(mins_layer, GTextAlignmentCenter);
	
	// Seconds layer
	secs_layer = text_layer_create(GRect(5, 130, 40 /* width */, 37 /* height */));
	text_layer_set_text_color(secs_layer, GColorWhite);
	text_layer_set_background_color(secs_layer, GColorClear);
	text_layer_set_font(secs_layer, PixelFont34);
	text_layer_set_text_alignment(secs_layer, GTextAlignmentCenter);

	// Day layer
	day_layer = text_layer_create(GRect(105, 40, 40 /* width */, 37 /* height */));
	text_layer_set_text_color(day_layer, GColorWhite);
	text_layer_set_background_color(day_layer, GColorClear);
	text_layer_set_font(day_layer, PixelFont34);
	text_layer_set_text_alignment(day_layer, GTextAlignmentCenter);

	// Month layer
	month_layer = text_layer_create(GRect(105, 85, 40 /* width */, 37 /* height */));
	text_layer_set_text_color(month_layer, GColorWhite);
	text_layer_set_background_color(month_layer, GColorClear);
	text_layer_set_font(month_layer, PixelFont34);
	text_layer_set_text_alignment(month_layer, GTextAlignmentCenter);

	// Year layer
	year_layer = text_layer_create(GRect(105, 130, 40 /* width */, 37 /* height */));
	text_layer_set_text_color(year_layer, GColorWhite);
	text_layer_set_background_color(year_layer, GColorClear);
	text_layer_set_font(year_layer, PixelFont34);
	text_layer_set_text_alignment(year_layer, GTextAlignmentCenter);
	
	// Battery status layer
	battery_layer = text_layer_create(GRect(110, 3, 35 /* width */, 20 /* height */));
	text_layer_set_text_color(battery_layer, GColorWhite);
	text_layer_set_background_color(battery_layer, GColorClear);
	text_layer_set_font(battery_layer, MiniPixelFont10);
	text_layer_set_text_alignment(battery_layer, GTextAlignmentCenter);
	handle_battery(battery_state_service_peek());
	//text_layer_set_text(battery_layer, "100%");

	// Bluetooth status layer
	bluetooth_layer = bitmap_layer_create(GRect(3, 3, 7 /* width */, 13 /* height */));
	bluetooth_connected_image = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_CONNECTED_IMAGE_WHITE);
	bluetooth_disconnected_image = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_DISCONNECTED_IMAGE_WHITE);
	handle_bluetooth(bluetooth_connection_service_peek());

	// Ensures time is displayed immediately (will break if NULL tick event accessed).
	// (This is why it's a good idea to have a separate routine to do the update itself.)
	time_t now = time(NULL);
	struct tm *current_time = localtime(&now);
	handle_second_tick(current_time, SECOND_UNIT);
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
	battery_state_service_subscribe(&handle_battery);
	bluetooth_connection_service_subscribe(&handle_bluetooth);

	layer_add_child(root_layer, text_layer_get_layer(hours_layer));
	layer_add_child(root_layer, text_layer_get_layer(mins_layer));
	layer_add_child(root_layer, text_layer_get_layer(secs_layer));
	layer_add_child(root_layer, text_layer_get_layer(day_layer));
	layer_add_child(root_layer, text_layer_get_layer(month_layer));
	layer_add_child(root_layer, text_layer_get_layer(year_layer));
	layer_add_child(root_layer, bitmap_layer_get_layer(bluetooth_layer));
	layer_add_child(root_layer, text_layer_get_layer(battery_layer));
}

static void do_deinit(void)
{
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	fonts_unload_custom_font(PixelFont34);
	text_layer_destroy(hours_layer);
	text_layer_destroy(mins_layer);
	text_layer_destroy(secs_layer);
	text_layer_destroy(day_layer);
	text_layer_destroy(month_layer);
	text_layer_destroy(year_layer);
	bitmap_layer_destroy(bluetooth_layer);
	text_layer_destroy(battery_layer);
	window_destroy(window);
}

static void handle_battery(BatteryChargeState charge_state)
{
	static char battery_text[] = "100%";
	
	if (charge_state.is_charging)
	{
		snprintf(battery_text, sizeof(battery_text), "ch");
	}
	else
	{
		snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
	}
	text_layer_set_text(battery_layer, battery_text);
}


static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed)
{
	// Called once per second
	int curr_mins;
	int curr_secs;
	static char hours[] = "00"; // Needs to be static because it's used by the system later.
	static char mins[] = "00"; // Needs to be static because it's used by the system later.
	static char secs[] = "00"; // Needs to be static because it's used by the system later.
	static char day[] = "00"; // Needs to be static because it's used by the system later.
	static char month[] = "00"; // Needs to be static because it's used by the system later.
	static char year[] = "00"; // Needs to be static because it's used by the system later.

	strftime(hours, sizeof(hours), "%H", tick_time);
	text_layer_set_text(hours_layer, hours);

	strftime(mins, sizeof(mins), "%M", tick_time);
	text_layer_set_text(mins_layer, mins);

	strftime(secs, sizeof(secs), "%S", tick_time);
	text_layer_set_text(secs_layer, secs);

	strftime(day, sizeof(day), "%d", tick_time);
	text_layer_set_text(day_layer, day);

	strftime(month, sizeof(month), "%m", tick_time);
	text_layer_set_text(month_layer, month);	

	strftime(year, sizeof(year), "%y", tick_time);
	text_layer_set_text(year_layer, year);

	// Samples the battery status every 50 minutes
	curr_mins = (*tick_time).tm_min;
	if (curr_mins % 50 == 0)
	{
		handle_battery(battery_state_service_peek());
	}

	// Samples the bluetooth status every 7 seconds
	curr_secs = (*tick_time).tm_sec;
	if (curr_secs % 7 == 0)
	{
		handle_bluetooth(bluetooth_connection_service_peek());
	}
}

static void handle_bluetooth(bool connected)
{
	if (connected)
	{
		bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_connected_image);
	}
	else
	{
		bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_disconnected_image);
	}
}


int main(void)
{
	do_init();
	app_event_loop();
	do_deinit();
}