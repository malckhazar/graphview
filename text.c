#include "text.h"

#define ADD_TAG_KEYWORD(name) \
    gtk_text_buffer_create_tag(buffer, name, \
            "weight", PANGO_WEIGHT_BOLD, \
            "foreground", "darkgreen", \
            NULL);

void fill_dictionary(GtkTextBuffer *buffer)
{
    const char* keywords[] = {
        "graph",
        "digraph",
        "strict",
        "node",
        "edge",
        "subgraph",
        NULL
    };

    const char** keyword;
    for (keyword = keywords; *keyword; keyword++)
        ADD_TAG_KEYWORD(*keyword);
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
    while (gtk_text_iter_forward_search(&iter, token, GTK_TEXT_SEARCH_TEXT_ONLY,
                    &match_start, &match_end, &range->end))
    {
        if (gtk_text_iter_starts_word(&match_start) && gtk_text_iter_ends_word(&match_end))
            gtk_text_buffer_apply_tag(range->buffer, tag, &match_start, &match_end);
        iter = match_end;
    }
    g_free(token);
}

/*
static void report_file_error(const char* func, const GError *error)
{
    g_error ("%s: file IO error: %s (%d)", func, error->message, error->code);
}
*/
#define report_file_error(func, error) \
    g_error ("%s: file IO error: %s (%d)", func, error->message, error->code);

void compile_dot(GtkTextBuffer* buffer)
{
    static gchar tmp_text_file_template[] = "graphview-XXXXXX.dot";
    static gchar tmp_image_file_template[] = "graphview-XXXXXX.png";
    GtkWidget* image_view = g_object_get_data(G_OBJECT(buffer), "image-view");

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

