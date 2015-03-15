#include <pebble.h>
#include "pixelr.h"

// Defining Global Variables
Window *window;
TextLayer *hours_layer;
TextLayer *mins_layer;
TextLayer *secs_layer;
TextLayer *date_layer;
TextLayer *battery_layer;
static GFont *PixelFont48;
static GFont *PixelFont40;
static GFont *PixelFont32;
static GFont *PixelFont18;
static GFont *PixelFont10;
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
	PixelFont48 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIXEL_FONT_48));
	PixelFont40 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIXEL_FONT_40));
	PixelFont32 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIXEL_FONT_32));
	PixelFont18 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIXEL_FONT_18));
	PixelFont10= fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIXEL_FONT_10));

	// Hours layer
	hours_layer = text_layer_create(GRect(40, 0, 65 /* width */, 55 /* height */));
	text_layer_set_text_color(hours_layer, GColorWhite);
	text_layer_set_background_color(hours_layer, GColorBlack);
	text_layer_set_font(hours_layer, PixelFont48);
	text_layer_set_text_alignment(hours_layer, GTextAlignmentCenter);
	
	// Minutes layer
	mins_layer = text_layer_create(GRect(49, 47, 40 /* width */, 37 /* height */));
	text_layer_set_text_color(mins_layer, GColorWhite);
	text_layer_set_background_color(mins_layer, GColorBlack);
	text_layer_set_font(mins_layer, PixelFont40);
	text_layer_set_text_alignment(mins_layer, GTextAlignmentCenter);
	
	// Seconds layer
	secs_layer = text_layer_create(GRect(49, 87, 40 /* width */, 37 /* height */));
	text_layer_set_text_color(secs_layer, GColorWhite);
	text_layer_set_background_color(secs_layer, GColorBlack);
	text_layer_set_font(secs_layer, PixelFont32);
	text_layer_set_text_alignment(secs_layer, GTextAlignmentCenter);

	// Date layer
	date_layer = text_layer_create(GRect(20, 140, 105 /* width */, 25 /* height */));
	text_layer_set_text_color(date_layer, GColorWhite);
	text_layer_set_background_color(date_layer, GColorBlack);
	text_layer_set_font(date_layer, PixelFont18);
	text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
	
	// Battery status layer
	battery_layer = text_layer_create(GRect(110, 3, 35 /* width */, 20 /* height */));
	text_layer_set_text_color(battery_layer, GColorWhite);
	text_layer_set_background_color(battery_layer, GColorBlack);
	text_layer_set_font(battery_layer, PixelFont10);
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
	//layer_add_child(root_layer, text_layer_get_layer(mins_layer));
	//layer_add_child(root_layer, text_layer_get_layer(secs_layer));
	layer_add_child(root_layer, text_layer_get_layer(date_layer));
	layer_add_child(root_layer, bitmap_layer_get_layer(bluetooth_layer));
	layer_add_child(root_layer, text_layer_get_layer(battery_layer));
}

static void do_deinit(void)
{
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	fonts_unload_custom_font(PixelFont48);
	fonts_unload_custom_font(PixelFont40);
	fonts_unload_custom_font(PixelFont32);
	fonts_unload_custom_font(PixelFont18);
	fonts_unload_custom_font(PixelFont10);
	text_layer_destroy(hours_layer);
	text_layer_destroy(mins_layer);
	text_layer_destroy(secs_layer);
	text_layer_destroy(date_layer);
	bitmap_layer_destroy(bluetooth_layer);
	text_layer_destroy(battery_layer);
	window_destroy(window);
}

static void handle_battery(BatteryChargeState charge_state)
{
	static char battery_text[] = "100%";
	
	if (charge_state.is_charging)
	{
		if (charge_state.charge_percent >= 98)
		{
			snprintf(battery_text, sizeof(battery_text), "Full");
		}
		else
		{
			snprintf(battery_text, sizeof(battery_text), "...");
		}
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
	static char date[] = "00-00-00"; // Needs to be static because it's used by the system later.

	//strftime(hours, sizeof(hours), "%H", tick_time);
	snprintf(hours, sizeof(hours), "88");
	text_layer_set_text(hours_layer, hours);

	//strftime(mins, sizeof(mins), "%M", tick_time);
	snprintf(mins, sizeof(mins), "88");
	text_layer_set_text(mins_layer, mins);

	//strftime(secs, sizeof(secs), "%S", tick_time);
	snprintf(secs, sizeof(secs), "88");
	text_layer_set_text(secs_layer, secs);

	//strftime(date, sizeof(date), "%d-%m-%y", tick_time);
	snprintf(date, sizeof(date), "88-88-88");
	text_layer_set_text(date_layer, date);

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