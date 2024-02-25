#include <ctype.h>
#include "text.h"

static gchar new_dot_text[] = "digraph G {\n"
                      "    a -> b;\n"
                      "}";

static GtkTextTagTable* create_dictionary()
{
    static const char* keywords[] = {
        "graph",
        "digraph",
        "strict",
        "node",
        "edge",
        "subgraph",
        NULL
    };

    static const char* symbols[] = {
        "--",
        "->",
        NULL
    };

    GtkTextTagTable* table = gtk_text_tag_table_new();

    const char** keyword;
    GtkTextTag* tag;

    for (keyword = keywords; *keyword; keyword++) {
        tag = gtk_text_tag_new(*keyword);
        g_object_set(G_OBJECT(tag),
                "weight", PANGO_WEIGHT_BOLD,
                "foreground", "darkgreen",
                NULL);
        gtk_text_tag_table_add(table, tag);
        g_message("added tag '%s'", *keyword);
    }

    for (keyword = symbols; *keyword; keyword++) {
        tag = gtk_text_tag_new(*keyword);
        g_object_set(G_OBJECT(tag),
                "weight", PANGO_WEIGHT_BOLD,
                NULL);
        gtk_text_tag_table_add(table, tag);
        g_message("added tag '%s'", *keyword);
    }

    return table;
}

void check_apply_tag(GtkTextTag* tag, gpointer data)
{
    GtkTextIter iter;
    struct tag_search_range* range = (struct tag_search_range*) data;
    
    char* token = NULL;
    g_object_get(G_OBJECT(tag), "name", &token, NULL);

    gtk_text_buffer_remove_tag(range->buffer, tag, &range->start, &range->end);
    iter = range->start;
    GtkTextIter match_start, match_end;

    while (gtk_text_iter_forward_search(&iter, token, GTK_TEXT_SEARCH_VISIBLE_ONLY,
                    &match_start, &match_end, &range->end))
    {
        GtkTextIter ms = match_start;
        GtkTextIter me = match_end;
        
        gtk_text_iter_backward_char(&ms);
        gboolean is_symbol = isspace(gtk_text_iter_get_char(&ms)) &&
            (isspace(gtk_text_iter_get_char(&me)) || (gtk_text_iter_ends_line(&me)));

        if (gtk_text_iter_starts_word(&match_start) && gtk_text_iter_ends_word(&match_end)) {
            gtk_text_buffer_apply_tag(range->buffer, tag, &match_start, &match_end);
        } else if (is_symbol) {
            gtk_text_buffer_apply_tag(range->buffer, tag, &match_start, &match_end);
        }

        iter = match_end;
    }

    g_free(token);
}

void compile(GtkTextBuffer* buffer)
{
    static gchar tmp_text_file_template[] = "graphview-XXXXXX.dot";

    //write text to tmp
    GFileIOStream *iostream = NULL;
    GError *error = NULL;
    GFile *tmp_file = NULL;
    char* text = NULL;

    tmp_file = g_file_new_tmp(tmp_text_file_template, &iostream, &error);
    if (error) goto compile_dot_error;

    GOutputStream *ostream = g_io_stream_get_output_stream(G_IO_STREAM(iostream));
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

    if (written != strlen(text)) {
        g_warning("Incomplete data was written to tmp!");
        goto compile_dot_error;
    }

    if (!g_io_stream_close(G_IO_STREAM(iostream), NULL, &error))
        goto compile_dot_error;

    // call dot
    char* file_path = g_file_get_path(tmp_file);

    static const gchar call_template[] = "/usr/bin/dot -Tpng -O %s";
    size_t cmd_len = strlen(call_template) + strlen(file_path) - sizeof("%s")
                     + sizeof(".png") + 1;
    gchar *call_dot_cmd = malloc(cmd_len);
    snprintf(call_dot_cmd, cmd_len, call_template, file_path);
    
    int res = system(call_dot_cmd);
    free(call_dot_cmd);
    if (res < 0) {
        g_error("system(%s) call failed: %s", call_dot_cmd, strerror(errno));
        goto compile_dot_free;
    }

    g_file_delete(tmp_file, NULL, NULL);
    g_object_unref(tmp_file);

    file_path = strcat(file_path, ".png");
    tmp_file = g_file_new_for_path(file_path);
    g_free(file_path);
    file_path = NULL;

    // show image to image_view
    GtkWidget* image_view = g_object_get_data(G_OBJECT(buffer), "image-view");
    gtk_picture_set_file(GTK_PICTURE(image_view), tmp_file);

compile_dot_error:
    if (error) g_error ("%s: file IO error: %s (%d)", __func__,
            error->message, error->code);

compile_dot_free:
    if (text) g_free(text);

    if (file_path) g_free(file_path);

    if (iostream) {
        g_io_stream_close(G_IO_STREAM(iostream), NULL, NULL);
        g_object_unref(iostream);
    }
    if (tmp_file) {
        g_file_delete(tmp_file, NULL, NULL);
        g_object_unref(tmp_file);
    }
}

static void text_buffer_changed(GtkTextBuffer* buffer, gpointer user_data)
{
    GtkTextMark* mark = gtk_text_buffer_get_insert(buffer);

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

    struct tag_search_range range;
    range.buffer = buffer;
    range.start = iter;
    range.end = iter;

    // find processing range
    gtk_text_iter_backward_line(&range.start);
    gtk_text_iter_forward_line(&range.end);

    gtk_text_tag_table_foreach(gtk_text_buffer_get_tag_table(range.buffer),
            check_apply_tag, &range);
}

void set_default_text(GtkTextBuffer* model)
{
    gtk_text_buffer_set_text(model, new_dot_text, strlen(new_dot_text));

    struct tag_search_range range;
    range.buffer = model;
    gtk_text_buffer_get_start_iter(model, &range.start);
    gtk_text_buffer_get_end_iter(model, &range.end);

    gtk_text_tag_table_foreach(gtk_text_buffer_get_tag_table(range.buffer),
            check_apply_tag, &range);
}

GtkTextBuffer* create_text_model()
{
    GtkTextBuffer* model = gtk_text_buffer_new(create_dictionary());
    g_signal_connect(G_OBJECT(model), "changed", G_CALLBACK(text_buffer_changed), NULL);

    return model;
}
