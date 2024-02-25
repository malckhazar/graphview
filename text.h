#include <gtk/gtk.h>

struct tag_search_range {
    GtkTextBuffer *buffer;
    GtkTextIter start;
    GtkTextIter end;
};

void check_apply_tag(GtkTextTag* tag, gpointer data);
void compile_dot(GtkTextBuffer* buffer);
void fill_dictionary(GtkTextBuffer* buffer);

