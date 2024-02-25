#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include "file_ops.h"
#include "text.h"

static gchar new_dot_text[] = "digraph G {\n"
                      "    a -> b;\n"
                      "}";

static void text_buffer_changed(GtkTextBuffer* buffer, gpointer user_data)
{
    GtkTextMark* mark = gtk_text_buffer_get_insert(buffer);

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

    struct tag_search_range range;
    range.buffer = buffer;
    range.start = iter;
    range.end = iter;

    if (!gtk_text_iter_starts_line(&iter)) {
        if (gtk_text_iter_inside_word(&range.start) || gtk_text_iter_ends_word(&range.start))
            gtk_text_iter_backward_word_start(&range.start);

        if (gtk_text_iter_inside_word(&range.end) || gtk_text_iter_starts_word(&range.end))
            if (!gtk_text_iter_ends_line(&range.end))
                gtk_text_iter_forward_word_end(&range.end);
    } else {
        gtk_text_iter_backward_line(&range.start);
        gtk_text_iter_forward_line(&range.end);
    }

    GtkTextTagTable* tag_table = gtk_text_buffer_get_tag_table(buffer);
    gtk_text_tag_table_foreach(tag_table, check_apply_tag, &range);
}

static void new_dot(GtkButton* button, gpointer user_data)
{
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(user_data);
    gtk_text_buffer_set_text(buffer, new_dot_text, strlen(new_dot_text));

    struct tag_search_range range;
    range.buffer = buffer;
    gtk_text_buffer_get_start_iter(buffer, &range.start);
    gtk_text_buffer_get_end_iter(buffer, &range.end);
    GtkTextTagTable* tag_table = gtk_text_buffer_get_tag_table(buffer);
    gtk_text_tag_table_foreach(tag_table, check_apply_tag, &range);

    compile_dot(buffer);
}

static void compile_dot_from_editor(GtkButton* button, gpointer user_data)
{
    GtkTextBuffer* buffer = GTK_TEXT_BUFFER(user_data);
    compile_dot(buffer);
}

static GtkWidget* create_view_controls(GtkWidget* text_view)
{
	GtkWidget* top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

	GtkWidget* button_new = gtk_button_new_with_label("New dot");
    g_signal_connect(button_new, "clicked", G_CALLBACK(new_dot), buffer);

    GtkWidget* button_open = gtk_button_new_with_label("Open dot...");    
    GtkWidget* button_save = gtk_button_new_with_label("Save dot...");

    bind_file_actions_to_buttons(button_open, button_save, buffer);

    GtkWidget* button_compile = gtk_button_new_with_label("Compile dot");
    g_signal_connect(button_compile, "clicked", G_CALLBACK(compile_dot_from_editor), buffer);

    gtk_box_append(GTK_BOX(top), button_new);
	gtk_box_append(GTK_BOX(top), button_open);
	gtk_box_append(GTK_BOX(top), button_save);
    gtk_box_append(GTK_BOX(top), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    gtk_box_append(GTK_BOX(top), button_compile);

	return top;
}

static void app_activate(GtkApplication* app, gpointer user_data)
{
	GtkWidget *cont;
    GtkWindow *window = NULL;
	window = GTK_WINDOW(gtk_application_window_new(app));

	gtk_window_set_title(GTK_WINDOW(window), "graphtool");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

	cont = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_window_set_child(GTK_WINDOW(window), cont);

	// view part
	GtkWidget* image_view = gtk_picture_new();
	GtkWidget* text_view = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_view), TRUE);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    g_object_set_data (G_OBJECT(buffer), "image-view", image_view);
    g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(text_buffer_changed), NULL);
    
    fill_dictionary(buffer);
    file_ops_init(window, buffer);

    GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

	// control part
	gtk_box_append(GTK_BOX(cont), create_view_controls(text_view));
    gtk_box_append(GTK_BOX(cont), paned);

    GtkWidget *text_view_frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(text_view_frame), text_view);

	gtk_paned_set_start_child(GTK_PANED(paned), text_view_frame);
    gtk_paned_set_resize_start_child(GTK_PANED(paned), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
    gtk_widget_set_size_request(text_view, 100, -1);

    GtkWidget *image_view_frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(image_view_frame), image_view);

    gtk_paned_set_end_child(GTK_PANED(paned), image_view_frame);
    gtk_paned_set_resize_end_child(GTK_PANED(paned), TRUE);
    gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);
    gtk_widget_set_size_request(image_view, 100, -1);

    gtk_widget_set_hexpand(image_view, TRUE);
    gtk_widget_set_hexpand_set(image_view, TRUE);
    gtk_widget_set_vexpand(text_view, TRUE);
    gtk_widget_set_vexpand_set(text_view, TRUE);

    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);

	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);

    new_dot(NULL, buffer);

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

    file_ops_cleanup();
    g_object_unref(app);

	return status;
}

