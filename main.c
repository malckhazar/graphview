#include <gtk/gtk.h>
#include <stdio.h>

static GtkWidget* image_view = NULL;
static GtkWidget* text_view = NULL;

static void view_button_toggle(GtkCheckButton* button, gpointer user_data)
{
	gtk_widget_set_visible(GTK_WIDGET(user_data), gtk_check_button_get_active(button));
}

static GtkWidget* create_view_controls()
{
	GtkWidget* top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

	GtkWidget* button_new = gtk_button_new_with_label("New");
	// signal
	GtkWidget* button_open = gtk_button_new_with_label("Open...");
	// signal
	
	GtkWidget* frame = gtk_frame_new("View type");
	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_widget_set_hexpand(frame, FALSE);
	gtk_frame_set_child(GTK_FRAME(frame), box);

	// image button
	GtkWidget* image_button = gtk_check_button_new_with_label("Image");
	g_signal_connect(image_button, "toggled", G_CALLBACK(view_button_toggle), image_view);

	// text button
	GtkWidget* text_button = gtk_check_button_new_with_label("Text");
	gtk_check_button_set_group(GTK_CHECK_BUTTON(text_button), GTK_CHECK_BUTTON(image_button));
	g_signal_connect(text_button, "toggled", G_CALLBACK(view_button_toggle), text_view);

	gtk_box_append(GTK_BOX(box), image_button);
	gtk_box_append(GTK_BOX(box), text_button);

	gtk_box_append(GTK_BOX(top), button_new);
	gtk_box_append(GTK_BOX(top), button_open);
	gtk_box_append(GTK_BOX(top), frame);

	gtk_check_button_set_active(GTK_CHECK_BUTTON(text_button), TRUE);
	gtk_widget_set_visible(image_view, FALSE);

	return GTK_WIDGET(top);
}

static void app_activate(GtkApplication* app, gpointer user_data)
{
	GtkWidget* wnd = gtk_application_window_new(app);
	GtkWidget *cont, *w;

	gtk_window_set_title(GTK_WINDOW(wnd), "graphtool");
	gtk_window_set_default_size(GTK_WINDOW(wnd), 800, 600);

	cont = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_window_set_child(GTK_WINDOW(wnd), cont);
	//gtk_box_set_homogeneous(GTK_BOX(cont), TRUE);

	// view part
	image_view = gtk_image_new();
	text_view = gtk_text_view_new();

	// control part
	gtk_box_append(GTK_BOX(cont), create_view_controls());
	gtk_box_append(GTK_BOX(cont), image_view);
	gtk_box_append(GTK_BOX(cont), text_view);

	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(buffer, "Test text", -1);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);

	gtk_window_maximize(GTK_WINDOW(wnd));
	gtk_window_present(GTK_WINDOW(wnd));
}

int main (int argc, char** argv)
{
	GtkApplication* app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	int status;

	g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}

