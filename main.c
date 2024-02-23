#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>

#define ASSERT(b) if (!(b)) g_error("%s: failed (%s)!\n", __func__, #b);

enum {
    file_action_open,
    file_action_save
};

GtkWindow *window = NULL;
GtkFileDialog *file_dialog = NULL;

static gchar new_dot_text[] = "digraph G {\n"
                      "    a -> b;\n"
                      "}";

static void read_file(GFile* file, GtkTextBuffer* buffer);
static void write_file(GFile* file, GtkTextBuffer* buffer);

static void report_file_error(const char* func, const GError *error)
{
    g_error ("%s: file IO error: %s (%d)", func, error->message, error->code);
}

static void compile_dot(GtkTextBuffer* buffer, GtkWidget *image_view)
{
    static gchar tmp_text_file_template[] = "graphview-XXXXXX.dot";
    static gchar tmp_image_file_template[] = "graphview-XXXXXX.png";

    //write text to tmp
    GFileIOStream *dot_iostream = NULL;
    GFileIOStream *svg_iostream = NULL;
    GError *error = NULL;
    GFile *tmp_dot = NULL;
    GFile *tmp_svg = NULL;
    char* text = NULL;

    tmp_dot = g_file_new_tmp(tmp_text_file_template, &dot_iostream, &error);
    if (error) goto compile_dot_error;

    tmp_svg = g_file_new_tmp(tmp_image_file_template, &svg_iostream, &error);
    if (error) goto compile_dot_error;

    g_io_stream_close(G_IO_STREAM(svg_iostream), NULL, &error);
    g_object_unref(svg_iostream);
    svg_iostream = NULL;
    if (error) goto compile_dot_error;

    GOutputStream *ostream = g_io_stream_get_output_stream(G_IO_STREAM(dot_iostream));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    if (!text) {
        g_error("Unable to get text from text buffer!");
        goto compile_dot_error;
    }
    gtk_text_buffer_set_modified (buffer, FALSE);

    gsize written = 0;
    if (!g_output_stream_write_all(ostream, text, strlen(text), &written, NULL, &error))
        goto compile_dot_error;

    if (!g_io_stream_close(G_IO_STREAM(dot_iostream), NULL, &error))
        goto compile_dot_error;

    // call dot 
    const char* dot_file_path = g_file_get_path(tmp_dot);
    const char* svg_file_path = g_file_get_path(tmp_svg);

    static const gchar call_template[] = "/usr/bin/dot -Tpng -o%s %s";
    size_t cmd_len = strlen(call_template) + 
        strlen(dot_file_path) - 2 +
        strlen(svg_file_path) - 2 + 1;
    gchar *call_dot_cmd = malloc(cmd_len);
    memset(call_dot_cmd, 0, cmd_len);
    snprintf(call_dot_cmd, cmd_len, call_template, svg_file_path, dot_file_path);
    
    int res = system(call_dot_cmd);
    if (res < 0) {
        g_error("system(%s) call failed: %s", call_dot_cmd, strerror(errno));
        free(call_dot_cmd);
        return;
    }
    g_info("system(%s) call returned: %d", call_dot_cmd, res);
    free(call_dot_cmd);

    // show image to image_view
    gtk_picture_set_file(GTK_PICTURE(image_view), tmp_svg);

compile_dot_error:
    if (error) report_file_error(__func__, error);

compile_dot_free:
    if (text) g_free(text);

    if (dot_iostream) {
        g_io_stream_close(G_IO_STREAM(dot_iostream), NULL, NULL);
        g_object_unref(dot_iostream);
    }
    if (tmp_dot) {
        g_file_delete(tmp_dot, NULL, NULL);
        g_object_unref(tmp_dot);
    }
    
    if (svg_iostream) {
        g_io_stream_close(G_IO_STREAM(svg_iostream), NULL, NULL);
        g_object_unref(svg_iostream);
    }
    if (tmp_svg) {
        g_file_delete(tmp_svg, NULL, NULL);
        g_object_unref(tmp_svg);
    }
}

static void new_dot(GtkButton* button, gpointer user_data)
{
    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(user_data);
    gtk_text_buffer_set_text(buffer, new_dot_text, strlen(new_dot_text));

    GtkWidget* image_view = g_object_get_data(G_OBJECT(buffer), "image-view");
    compile_dot(buffer, image_view);
}

static void read_file(GFile* file, GtkTextBuffer* buffer)
{
    GError* error = NULL;

    char* content = NULL;
    gsize size = 0;
    g_file_load_contents(file, NULL, &content, &size, NULL, &error);
    if (error) {
        g_free(content);
        return;
    }

    gtk_text_buffer_set_text(buffer, content, size);
    g_free(content);

    GtkWidget *image_view = g_object_get_data(G_OBJECT(buffer), "image-view");
    compile_dot(buffer, image_view);
}

static void write_file(GFile* file, GtkTextBuffer* buffer)
{
    GError* error = NULL;
    GFileIOStream* iostream = g_file_open_readwrite(file, NULL, &error);
    if (error) {
        if (error->code == G_IO_ERROR_IS_DIRECTORY)
            return;
        if (error->code == G_IO_ERROR_NOT_FOUND) {
            error = NULL;
            iostream = g_file_create_readwrite(file, G_FILE_CREATE_NONE, NULL, &error);
        }

        if (error) {
            g_error("file operation failed: %s", error->message);
            return;
        }
    }

    GOutputStream *ostream = g_io_stream_get_output_stream(G_IO_STREAM(iostream));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    if (!text) {
        g_error("Unable to get text from text buffer!");
        g_object_unref(iostream);
        return;
    }
    gtk_text_buffer_set_modified (buffer, FALSE);

    gsize written = 0;
    g_output_stream_write_all(ostream, text, strlen(text), &written, NULL, &error);
    g_free(text);
    g_io_stream_close(G_IO_STREAM(iostream), NULL, NULL);
    g_object_unref(iostream);
}

static void compile_dot_from_editor(GtkButton* button, gpointer user_data)
{
    GtkTextBuffer* buffer = GTK_TEXT_BUFFER(user_data);
    GtkWidget *image_view = g_object_get_data(G_OBJECT(buffer), "image-view");
    compile_dot(buffer, image_view);
}

static void perform_file_action(GObject* object, GAsyncResult* res, gpointer data)
{
    typedef GFile* (*finish_action_fn)(GtkFileDialog*, GAsyncResult*, GError**);
    finish_action_fn finish_action = (finish_action_fn)data;

    GtkTextBuffer *buffer = GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(file_dialog), "text-buffer"));
    GtkWidget *image_view = g_object_get_data(G_OBJECT(buffer), "image-view");

    GFile *file = finish_action(file_dialog, res, NULL);
    if (file) {
        long action_type = (long)g_object_get_data(G_OBJECT(file_dialog), "action-type");
        switch(action_type)
        {
            case file_action_open:
                read_file(file, buffer);
                compile_dot(buffer, image_view);
                break;

            case file_action_save:
                write_file(file, buffer);
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

static GtkWidget* create_view_controls(GtkWidget* text_view)
{
	GtkWidget* top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

	GtkWidget* button_new = gtk_button_new_with_label("New dot");
    g_signal_connect(button_new, "clicked", G_CALLBACK(new_dot), buffer);

    GtkWidget* button_open = gtk_button_new_with_label("Open dot...");
    g_object_set_data(G_OBJECT(button_open), "action-type", (gpointer)file_action_open);
    g_signal_connect(button_open, "clicked", G_CALLBACK(file_action_button_clicked), buffer);
    
    GtkWidget* button_save = gtk_button_new_with_label("Save dot...");
    g_object_set_data(G_OBJECT(button_save), "action-type", (gpointer)file_action_save);
    g_signal_connect(button_save, "clicked", G_CALLBACK(file_action_button_clicked), buffer);

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
	GtkWidget *cont, *w;
	window = GTK_WINDOW(gtk_application_window_new(app));

	gtk_window_set_title(GTK_WINDOW(window), "graphtool");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

	cont = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
	gtk_window_set_child(GTK_WINDOW(window), cont);

	// view part
	GtkWidget* image_view = gtk_picture_new();
	GtkWidget* text_view = gtk_text_view_new();
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    g_object_set_data (G_OBJECT(buffer), "image-view", image_view);

    file_dialog = gtk_file_dialog_new();
    g_object_set_data(G_OBJECT(file_dialog), "text-buffer", buffer);
    gtk_file_dialog_set_modal(file_dialog, TRUE);

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

	gtk_text_buffer_set_text(buffer, new_dot_text, -1);
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

