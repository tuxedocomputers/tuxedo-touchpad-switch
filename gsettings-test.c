#include <gio/gio.h>
#include <stdio.h>

void send_events_handler(GSettings *settings, char* key, gpointer user_data) {
    const gchar *send_events_string = g_settings_get_string(settings, key);
    if (!send_events_string) {
        perror("g_settings_get_string");
    }
    printf("send_events_string: %s\n", send_events_string);
}

int main(int argc, char *argv[]) {
    GSettings *settings_touchpad = g_settings_new("org.gnome.desktop.peripherals.touchpad");
    if (!settings_touchpad) {
        perror("g_settings_new");
        return EXIT_FAILURE;
    }
    
    if (g_signal_connect(settings_touchpad, "changed::send-events", G_CALLBACK(send_events_handler), NULL) < 1) {
        perror("g_signal_connect");
        return EXIT_FAILURE;
    }
    
    GMainLoop *app = g_main_loop_new(NULL, TRUE);
    if (!app) {
        perror("g_main_loop_new");
        return EXIT_FAILURE;
    }
    
    g_main_loop_run(app);
    // g_main_loop_run should not return
    perror("g_main_loop_run");
    return EXIT_FAILURE;
}
