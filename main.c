#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <json-c/json.h>
#include <gtk-2.0/gtk/gtk.h>

#define FONT_SIZE_CLOCK             90
#define FONT_SIZE_UPTIME            13
#define FONT_SIZE_TEMP_CURRENT      80
#define FONT_SIZE_TEMP_APPARENT     13
#define FONT_SIZE_CONDITION_SUMMARY 30
#define FONT_SIZE_HUMIDITY          13
#define FONT_SIZE_WIND_SPEED        13

#define FONT_COLOR_CLOCK             "af2020"
#define FONT_COLOR_UPTIME            "333333"
#define FONT_COLOR_TEMP_CURRENT      "aaaaaa"
#define FONT_COLOR_TEMP_APPARENT     "6a7a80"
#define FONT_COLOR_CONDITION_SUMMARY "6a7a80"
#define FONT_COLOR_HUMIDITY          "6a7a80"
#define FONT_COLOR_WIND_SPEED        "6a7a80"

static unsigned int g_start_time;

typedef struct Weather
{
	GtkWidget* temp_current;
	GtkWidget* temp_apparent;
	GtkWidget* condition_summary;
	GtkWidget* humidity_current;
	GtkWidget* wind_speed;
}
Weather;

void init_start_time()
{
	time_t current_time;
	struct tm* local_time;

	current_time = time(NULL);

	g_start_time = (unsigned int)current_time;
}

void get_clock_string(char* buffer)
{
	time_t current_time;
	struct tm* local_time;

	current_time = time(NULL);
	local_time = localtime(&current_time);

	strftime(buffer, 32, "%r", local_time);
}

gboolean update_clock(gpointer data)
{
	char clock_string[32];
	get_clock_string(clock_string);

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "<span font_desc=\"%i.0\" color=\"#%s\">%s</span>", FONT_SIZE_CLOCK, FONT_COLOR_CLOCK, clock_string);

	GtkWidget* clock_label = (GtkWidget*)data;
	gtk_label_set_markup(GTK_LABEL(clock_label), buffer);
}

void get_uptime_string(char* buffer)
{
	// Get the current uptime in seconds.
	time_t current_time = time(NULL);
	time_t uptime = current_time - g_start_time;

	int hours = uptime / 3600;
	int minutes = (uptime % 3600) / 60;
	int seconds = (uptime % 3600) % 60;

	snprintf(buffer, 32, "%02i:%02i:%02i", hours, minutes, seconds);
}

gboolean update_uptime(gpointer data)
{
	char uptime_string[32];
	get_uptime_string(uptime_string);

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "<span font_desc=\"%i.0\" color=\"#%s\">%s</span>", FONT_SIZE_UPTIME, FONT_COLOR_UPTIME, uptime_string);

	GtkWidget* uptime_label = (GtkWidget*)data;
	gtk_label_set_markup(GTK_LABEL(uptime_label), buffer);
}

int parse_int(const char* string)
{
	return (int)strtol(string, (char**)NULL, 10);
}

gboolean update_weather(gpointer data)
{
	// Fetch weather data and sleep to allow the request to finish.
	system("wget -q -O /tmp/weather-tmp https://api.forecast.io/forecast/0d4d76c285b5f60423c6e6e59cabe0a7/30.616,-96.341");
	usleep(500 * 1000);

	FILE* file;
	file = fopen("/tmp/weather-tmp", "r");
	if (file == NULL)
		printf(stderr, "Failed to open weather file.\n");

	char source[1000000];
	size_t len = fread(source, sizeof(char), 1000000, file);
	if (len == 0)
		printf("Failed to read weather file.\n");

	fclose(file);

	system("rm /tmp/weather-tmp");

	// Read JSON data into strings.
	char temp_current[32];
	char temp_apparent[32];
	char condition_summary[32];
	char humidity[32];
	char wind_speed[32];

	json_object* root_object;
	root_object = json_tokener_parse(source);

	json_object* currently_object;
	json_object_object_get_ex(root_object, "currently", &currently_object);

	json_object* child_object;
	json_object_object_get_ex(currently_object, "temperature", &child_object);
	strcpy(temp_current, json_object_to_json_string(child_object));

	json_object_object_get_ex(currently_object, "apparentTemperature", &child_object);
	strcpy(temp_apparent, json_object_to_json_string(child_object));

	json_object_object_get_ex(currently_object, "summary", &child_object);
	strcpy(condition_summary, json_object_to_json_string(child_object));

	// Remove the opening and closing quotation marks.
	char* p = condition_summary;
	p++;
	p[strlen(p) - 1] = 0;
	strcpy(condition_summary, p);

	json_object_object_get_ex(currently_object, "humidity", &child_object);
	strcpy(humidity, json_object_to_json_string(child_object));

	// Remove the opening '0.'.
	p = humidity;
	p += 2;
	strcpy(humidity, p);

	json_object_object_get_ex(currently_object, "windSpeed", &child_object);
	strcpy(wind_speed, json_object_to_json_string(child_object));

	json_object_put(child_object);
	json_object_put(currently_object);
	json_object_put(root_object);

	// Update weather label widgets.
	Weather* weather = (Weather*)data;

	char buffer[128];

	snprintf(buffer, sizeof(buffer), "<span font_desc=\"%i.0\" color=\"#%s\">%i°</span>", FONT_SIZE_TEMP_CURRENT, FONT_COLOR_TEMP_CURRENT, parse_int(temp_current));
	gtk_label_set_markup(GTK_LABEL(weather->temp_current), buffer);

	snprintf(buffer, sizeof(buffer), "<span font_desc=\"%i.0\" color=\"#%s\">Feels like %i°</span>", FONT_SIZE_TEMP_APPARENT, FONT_COLOR_TEMP_APPARENT, parse_int(temp_apparent));
	gtk_label_set_markup(GTK_LABEL(weather->temp_apparent), buffer);

	snprintf(buffer, sizeof(buffer), "<span font_desc=\"%i.0\" color=\"#%s\">%s</span>", FONT_SIZE_CONDITION_SUMMARY, FONT_COLOR_CONDITION_SUMMARY, condition_summary);
	gtk_label_set_markup(GTK_LABEL(weather->condition_summary), buffer);

	snprintf(buffer, sizeof(buffer), "<span font_desc=\"%i.0\" color=\"#%s\">Humidity: %i\%</span>", FONT_SIZE_HUMIDITY, FONT_COLOR_HUMIDITY, parse_int(humidity));
	gtk_label_set_markup(GTK_LABEL(weather->humidity_current), buffer);

	snprintf(buffer, sizeof(buffer), "<span font_desc=\"%i.0\" color=\"#%s\">Wind: %i mph</span>", FONT_SIZE_WIND_SPEED, FONT_COLOR_WIND_SPEED, parse_int(wind_speed));
	gtk_label_set_markup(GTK_LABEL(weather->wind_speed), buffer);
}

int main(int argc, char** argv)
{
	init_start_time();

	gtk_init(&argc, &argv);

	// Create the main GTK window.
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Screensaver");
	gtk_window_set_default_size(GTK_WINDOW(window), 1280, 720);

	//gtk_window_fullscreen(window);

	GdkColor bg_color;
	bg_color.red   = 0x0000;
	bg_color.green = 0x0000;
	bg_color.blue  = 0x0000;
	gtk_widget_modify_bg(GTK_WIDGET(window), GTK_STATE_NORMAL, &bg_color);

	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	// Create widgets.
	GtkWidget* clock_label = gtk_label_new(NULL);
	GtkWidget* uptime_label = gtk_label_new(NULL);

	Weather weather;
	weather.temp_current = gtk_label_new(NULL);
	weather.temp_apparent = gtk_label_new(NULL);
	weather.condition_summary = gtk_label_new(NULL);
	weather.humidity_current = gtk_label_new(NULL);
	weather.wind_speed = gtk_label_new(NULL);

	// Set each widget's fixed position.
	GtkWidget* fixed = gtk_fixed_new();
	gtk_fixed_put(GTK_FIXED(fixed), clock_label, 500, 270);
	gtk_fixed_put(GTK_FIXED(fixed), uptime_label, 820, 410);
	gtk_fixed_put(GTK_FIXED(fixed), weather.temp_current, 150, 250);
	gtk_fixed_put(GTK_FIXED(fixed), weather.temp_apparent, 150, 410);
	gtk_fixed_put(GTK_FIXED(fixed), weather.condition_summary, 150, 365);
	gtk_fixed_put(GTK_FIXED(fixed), weather.humidity_current, 150, 430);
	gtk_fixed_put(GTK_FIXED(fixed), weather.wind_speed, 150, 450);
	gtk_container_add(GTK_CONTAINER(window), fixed);

	gtk_widget_show_all(window);

	// Update the initial label values.
	update_clock((gpointer)clock_label);
	update_weather((gpointer)&weather);
	update_uptime((gpointer)uptime_label);

	// Set up main loop function calls.
	g_timeout_add_seconds(1, update_clock, (gpointer)clock_label);
	g_timeout_add_seconds(60, update_weather, (gpointer)&weather);
	g_timeout_add_seconds(1, update_uptime, (gpointer)uptime_label);

	gtk_main();

	return 0;
}
