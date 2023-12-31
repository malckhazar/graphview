#include <gtk/gtk.h>
#include <stdio.h>

enum {
    file_action_open,
    file_action_save
};

GtkWindow *window = NULL;
GtkFileDialog *file_dialog = NULL;

static void compile_dot_from_editor(GtkButton* button, gpointer user_data)
{
    GtkTextView* text_view = user_data;

    //TODO: make compilation from dot to svg + render it in image
}

static void view_button_toggle(GtkCheckButton* button, gpointer user_data)
{
	gtk_widget_set_visible(GTK_WIDGET(user_data), gtk_check_button_get_active(button));
}

static void read_file(GFile* file, GtkTextView* text_view)
{
    //TODO
}

static void write_file(GFile* file, GtkTextView* text_view)
{
    //TODO
}

static void perform_file_action(GObject* object, GAsyncResult* res, gpointer data)
{
    typedef GFile* (*finish_action_fn)(GtkFileDialog*, GAsyncResult*, GError**);
    finish_action_fn finish_action = (finish_action_fn)data;

    GtkWidget* text_view = GTK_WIDGET(g_object_get_data(G_OBJECT(file_dialog), "text-view"));
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    GFile *file = finish_action(file_dialog, res, NULL);
    if (file) {
        long action_type = (long)g_object_get_data(G_OBJECT(file_dialog), "action-type");
        switch(action_type)
        {
            case file_action_open:
                read_file(file, GTK_TEXT_VIEW(text_view));
                break;

            case file_action_save:
                write_file(file, GTK_TEXT_VIEW(text_view));
                break;

            default:
                g_error("Undefined file action type!");
        }

        g_object_unref(file);
    }
}

static void file_action_button_clicked(GtkButton* button, gpointer user_data)
{
    GtkWidget* text_view = user_data;

    void (*start_action)(GtkFileDialog*, GtkWindow*, GCancellable*, GAsyncReadyCallback, gpointer) = NULL;
    GFile* (*finish_action)(GtkFileDialog*, GAsyncResult*, GError**) = NULL;
    long action_type = (long)g_object_get_data(G_OBJECT(button), "action-type");

    switch (action_type) {
        case file_action_open:
            start_action = gtk_file_dialog_open;
            finish_action = gtk_file_dialog_open_finish;
            break;
        case file_action_save:
            start_action = gtk_file_dialog_save;
            finish_action = gtk_file_dialog_save_finish;
            break;
    }

    if (!start_action || !finish_action)
        return;

    g_object_set_data(G_OBJECT(file_dialog), "action-type", (gpointer)action_type);

    start_action(file_dialog, window, NULL, perform_file_action, (gpointer)finish_action);
}

static GtkWidget* create_view_controls(GtkWidget* image_view, GtkWidget* text_view)
{
	GtkWidget* top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    GtkWidget* debug_label = gtk_label_new(NULL);
    gtk_box_append(GTK_BOX(top), debug_label);

	GtkWidget* button_new = gtk_button_new_with_label("New dot");
	// TODO: add signal

    GtkWidget* button_open = gtk_button_new_with_label("Open dot...");
    g_object_set_data(G_OBJECT(button_open), "action-type", (gpointer)file_action_open);
    g_signal_connect(button_open, "clicked", G_CALLBACK(file_action_button_clicked), text_view);
    
    GtkWidget* button_save = gtk_button_new_with_label("Save dot...");
    g_object_set_data(G_OBJECT(button_save), "action-type", (gpointer)file_action_save);
    g_signal_connect(button_save, "clicked", G_CALLBACK(file_action_button_clicked), text_view);

	GtkWidget* frame = gtk_frame_new("View type");
	GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gtk_widget_set_hexpand(frame, FALSE);

	// image button
	GtkWidget* image_button = gtk_check_button_new_with_label("Image");
	g_signal_connect(image_button, "toggled", G_CALLBACK(view_button_toggle), image_view);

	// text button
	GtkWidget* text_button = gtk_check_button_new_with_label("Editor");
	gtk_check_button_set_group(GTK_CHECK_BUTTON(text_button), GTK_CHECK_BUTTON(image_button));
	g_signal_connect(text_button, "toggled", G_CALLBACK(view_button_toggle), text_view);

	gtk_box_append(GTK_BOX(box), image_button);
	gtk_box_append(GTK_BOX(box), text_button);
	gtk_frame_set_child(GTK_FRAME(frame), box);

    gtk_box_append(GTK_BOX(top), button_new);
	gtk_box_append(GTK_BOX(top), button_open);
	gtk_box_append(GTK_BOX(top), button_save);
	gtk_box_append(GTK_BOX(top), frame);

	gtk_check_button_set_active(GTK_CHECK_BUTTON(text_button), TRUE);

	return top;
}

static GtkWidget* create_editor(GtkWidget* text_view)
{
    GtkWidget* top = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

    GtkWidget* compile_button = gtk_button_new_with_label("Compile");
    g_signal_connect(compile_button, "activate", G_CALLBACK(compile_dot_from_editor), text_view);

    gtk_box_append(GTK_BOX(toolbar), compile_button);
    gtk_box_append(GTK_BOX(top), toolbar);
    gtk_box_append(GTK_BOX(top), text_view);

    return top;
}

static void app_activate(GtkApplication* app, gpointer user_data)
{
	GtkWidget *cont, *w;
	window = GTK_WINDOW(gtk_application_window_new(app));

	gtk_window_set_title(GTK_WINDOW(window), "graphtool");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

	cont = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
	gtk_window_set_child(GTK_WINDOW(window), cont);

	// view part
	GtkWidget* image_view = gtk_image_new();
	GtkWidget* text_view = gtk_text_view_new();

    file_dialog = gtk_file_dialog_new();
    g_object_set_data(G_OBJECT(file_dialog), "text-view", text_view);
    gtk_file_dialog_set_modal(file_dialog, TRUE);

	// control part
    GtkWidget* editor = create_editor(text_view);
	gtk_box_append(GTK_BOX(cont), create_view_controls(image_view, editor));
    gtk_box_append(GTK_BOX(cont), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
	gtk_box_append(GTK_BOX(cont), image_view);
    gtk_box_append(GTK_BOX(cont), editor);

    gtk_widget_set_hexpand(image_view, TRUE);
    gtk_widget_set_hexpand_set(image_view, TRUE);
    gtk_widget_set_hexpand(text_view, TRUE);
    gtk_widget_set_vexpand(text_view, TRUE);
    gtk_widget_set_hexpand_set(text_view, TRUE);
    gtk_widget_set_vexpand_set(text_view, TRUE);

    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);

	gtk_widget_set_visible(image_view, FALSE);

	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(buffer, "Test text", -1);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);

	gtk_window_maximize(GTK_WINDOW(window));
	gtk_window_present(GTK_WINDOW(window));
}

int main (int argc, char** argv)
{
    gtk_init();

	GtkApplication* app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	int status;

	g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);
    g_object_unref(file_dialog);

	return status;
}

