#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include <raudio.h>
#include <pthread.h>

/*
 * Global variables
 */

char *file, *fileBasename, infoText[320], volumeText[13];
GtkWidget *songTitle, *songVolume;
Music music;
pthread_t thread;
int playing, paused, stop, loop, dontPlay, threadRunning;
float volume;

static void menuHandler(GtkWidget* menu, gpointer data)
{
    if (!strcmp(gtk_menu_item_get_label(GTK_MENU_ITEM(menu)), "Open")) {
        GtkWidget *dialogBox;
        GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
        gint res;

        dialogBox = gtk_file_chooser_dialog_new("Open File",
                                            NULL,
                                            GTK_FILE_CHOOSER_ACTION_OPEN,
                                            "Cancel",
                                            GTK_RESPONSE_CANCEL,
                                            "Open",
                                            GTK_RESPONSE_ACCEPT,
                                            NULL);

        res = gtk_dialog_run(GTK_DIALOG (dialogBox));

        if (res == GTK_RESPONSE_ACCEPT) {

            GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialogBox);
            file = gtk_file_chooser_get_filename(chooser);
            fileBasename = g_path_get_basename(file);
        } else {
            snprintf(infoText, sizeof(infoText), "No song loaded");
            gtk_label_set_label(GTK_LABEL(songTitle), infoText);
            gtk_widget_destroy(dialogBox);
            return;
        }

        paused = playing = 0;
        stop = 1;

        snprintf(infoText, sizeof(infoText), "Loaded song: %s", fileBasename);

        gtk_widget_destroy(dialogBox);

        if (strstr(fileBasename, ".mp3") || strstr(fileBasename, ".wav") || strstr(fileBasename, ".ogg") || strstr(fileBasename, ".flac")) {
            gtk_label_set_label(GTK_LABEL(songTitle), infoText);
            dontPlay = 0;
            music = LoadMusicStream(file);
        } else {
            snprintf(infoText, sizeof(infoText), "File not supported: %s", fileBasename);
            gtk_label_set_label(GTK_LABEL(songTitle), infoText);
            dontPlay = 1;
        }
        stop = 0;
        if (threadRunning) {
            pthread_cancel(thread);
            threadRunning = 0;
        }
        
    }
}

void *update(void *vargp)
{
    threadRunning = 1;
    if (dontPlay) return NULL;
    int started = 0;
    while (loop || !started){
        started = 1;
        while (GetMusicTimePlayed(music) < (GetMusicTimeLength(music) - 0.007)) {
            if (!paused) UpdateMusicStream(music);
            if (stop) break;
            sleep(1);
        }
        SeekMusicStream(music, 0);
    }
    threadRunning = 0;
    paused = playing = stop = 0;
    snprintf(infoText, sizeof(infoText), "Stopped song: %s", fileBasename);
    gtk_label_set_label(GTK_LABEL(songTitle), infoText);
}

static void volumeManagement(GtkWidget *scale, gpointer data)
{
    volume = gtk_range_get_value(GTK_RANGE(scale));

    snprintf(volumeText, sizeof(volumeText), "Volume: %.f%%", volume * 100);
    gtk_label_set_label(GTK_LABEL(songVolume), volumeText);

    if (dontPlay) return;
    SetMusicVolume(music, volume);
}

static void stopAudio(GtkWidget* button, gpointer data)
{
    if (dontPlay) return;
    snprintf(infoText, sizeof(infoText), "Stopped song: %s", fileBasename);
    gtk_label_set_label(GTK_LABEL(songTitle), infoText);
    stop = 1;
}

static void loopAudio(GtkWidget* button, gpointer data)
{
    if (!loop) {
        gtk_button_set_label(GTK_BUTTON(button), "🔂");
        loop = 1;
    } else {
        gtk_button_set_label(GTK_BUTTON(button), "🔁");
        loop = 0;
    }
}

static void pauseAudio(GtkWidget* button, gpointer data)
{
    if (dontPlay) return;
    if (!paused) {
        PauseMusicStream(music);
        snprintf(infoText, sizeof(infoText), "Paused song: %s", fileBasename);
        gtk_label_set_label(GTK_LABEL(songTitle), infoText);
        paused = 1;
    } else {
        ResumeMusicStream(music);
        snprintf(infoText, sizeof(infoText), "Playing song: %s", fileBasename);
        gtk_label_set_label(GTK_LABEL(songTitle), infoText);
        paused = 0;
    }
}

static void playAudio(GtkWidget* button, gpointer data)
{
    if (dontPlay) return;
    PlayMusicStream(music);

    snprintf(infoText, sizeof(infoText), "Playing song: %s", fileBasename);
    gtk_label_set_label(GTK_LABEL(songTitle), infoText);

    if (!playing) {
        pthread_create(&thread, NULL, update, NULL);
        playing = 1;
    }
}

static void activate(GtkApplication* app, gpointer user_data)
{
    GtkWidget *window, *grid, *menuBar, *fileMenu, *fileSubmenu, *fileMenuItems[2], *borderLabels[4], *controlButtons[4], *volumeBar, *volumeBar2;

    window = gtk_application_window_new(app);
    grid = gtk_grid_new();

    volumeBar = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.001);

    gtk_range_set_value(GTK_RANGE(volumeBar), 1.0);

    gtk_scale_set_draw_value(GTK_SCALE(volumeBar), false);

    for (int i = 0; i < 4; i++){
        borderLabels[i] = gtk_label_new("");
    }

    controlButtons[0] = gtk_button_new_with_label("▶️");
    controlButtons[1] = gtk_button_new_with_label("⏸️");
    controlButtons[2] = gtk_button_new_with_label("⏹️");
    controlButtons[3] = gtk_button_new_with_label("🔁");

    songTitle = gtk_label_new("Welcome!");
    songVolume = gtk_label_new("Volume: 100%");

    menuBar = gtk_menu_bar_new();
    fileMenu = gtk_menu_item_new_with_label("File");
    fileSubmenu = gtk_menu_new();
    fileMenuItems[0] = gtk_menu_item_new_with_label("Open");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMenu), fileSubmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuBar), fileMenu);

    gtk_menu_shell_append(GTK_MENU_SHELL(fileSubmenu), fileMenuItems[0]);

    g_signal_connect(fileMenuItems[0], "activate", G_CALLBACK(menuHandler), NULL);
    g_signal_connect(controlButtons[0], "clicked", G_CALLBACK(playAudio), NULL);
    g_signal_connect(controlButtons[1], "clicked", G_CALLBACK(pauseAudio), NULL);
    g_signal_connect(controlButtons[2], "clicked", G_CALLBACK(stopAudio), NULL);
    g_signal_connect(controlButtons[3], "clicked", G_CALLBACK(loopAudio), NULL);
    g_signal_connect(volumeBar, "value-changed", G_CALLBACK(volumeManagement), NULL);

    gtk_grid_set_column_homogeneous(GTK_GRID (grid), TRUE);
    gtk_container_add(GTK_CONTAINER(window), grid);

    gtk_grid_attach(GTK_GRID(grid), menuBar, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), borderLabels[0], 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), borderLabels[1], 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), borderLabels[2], 7, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), borderLabels[3], 1, 3, 1, 1);

    gtk_grid_attach(GTK_GRID(grid), songTitle, 1, 2, 4, 1);
    gtk_grid_attach(GTK_GRID(grid), songVolume, 5, 2, 2, 1);

    for (int i = 0; i < 4; i++){
        gtk_grid_attach(GTK_GRID(grid), controlButtons[i], i+1, 4, 1, 1);
    }
    gtk_grid_attach(GTK_GRID(grid), volumeBar, 5, 4, 2, 1);

    gtk_window_set_title(GTK_WINDOW (window), "audio-player");

    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_widget_show_all(window);
}

int main (int argc, char **argv) 
{
    GtkApplication *app;
    int status;

    playing = paused = stop = loop = 0;

    dontPlay = 1;

    volume = 1.0;

    InitAudioDevice();

    app = gtk_application_new ("org.firethefox.audio-player", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    UnloadMusicStream(music);
    return status;
}
