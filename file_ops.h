#include <gtk/gtk.h>

void bind_file_actions_to_buttons(GtkWidget* button_open, GtkWidget* button_save, gpointer buffer);
void file_ops_init(GtkWindow* parent, gpointer buffer);
void file_ops_cleanup();

