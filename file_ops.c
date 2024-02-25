#include "file_ops.h"
#include "text.h"

enum {
    file_action_open,
    file_action_save
};

static GtkFileDialog *file_dialog = NULL;

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

    struct tag_search_range range;
    range.buffer = buffer;
    gtk_text_buffer_get_start_iter(buffer, &range.start);
    gtk_text_buffer_get_end_iter(buffer, &range.end);
    GtkTextTagTable* tag_table = gtk_text_buffer_get_tag_table(buffer);
    gtk_text_tag_table_foreach(tag_table, check_apply_tag, &range);

    GtkWidget *image_view = g_object_get_data(G_OBJECT(buffer), "image-view");
    compile_dot(buffer);
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
                compile_dot(buffer);
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

    GtkWindow *window = GTK_WINDOW(g_object_get_data(G_OBJECT(file_dialog), "parent-window"));
    start_action(file_dialog, window, NULL, perform_file_action, (gpointer)finish_action);
}

void bind_file_actions_to_buttons(GtkWidget* button_open, GtkWidget* button_save, gpointer buffer)
{
    g_object_set_data(G_OBJECT(button_open), "action-type", (gpointer)file_action_open);
    g_signal_connect(button_open, "clicked",
            G_CALLBACK(file_action_button_clicked), buffer);
    g_object_set_data(G_OBJECT(button_save), "action-type", (gpointer)file_action_save);
    g_signal_connect(button_save, "clicked",
            G_CALLBACK(file_action_button_clicked), buffer);
}

void file_ops_init(GtkWindow* parent, gpointer buffer)
{
    file_dialog = gtk_file_dialog_new();
    g_object_set_data(G_OBJECT(file_dialog), "parent-window", parent);
    g_object_set_data(G_OBJECT(file_dialog), "text-buffer", buffer);
    gtk_file_dialog_set_modal(file_dialog, TRUE);
}

void file_ops_cleanup()
{
    g_object_unref(file_dialog);
}
