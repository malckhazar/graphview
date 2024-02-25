#include <gtk/gtk.h>

struct tag_search_range {
    GtkTextBuffer *buffer;
    GtkTextIter start;
    GtkTextIter end;
};

void set_default_text(GtkTextBuffer* buffer);
void check_apply_tag(GtkTextTag* tag, gpointer data);
void compile(GtkTextBuffer* buffer);
void fill_dictionary(GtkTextBuffer* buffer);
GtkTextBuffer* create_text_model();

