#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include "streams.h"

/* stream objects */
stream_t suc1;
stream_t cons1;
stream_t cons2;
stream_t cons3;
stream_t cons4;

/* text box widgets for threads to write into */
GtkWidget *output_a;
GtkWidget *output_b;
GtkWidget *output_c;
GtkWidget *output_d;
GtkWidget *output_suc;

void connect_a(GtkWidget *widget, gpointer data) {}
void connect_b(GtkWidget *widget, gpointer data) {}
void connect_c(GtkWidget *widget, gpointer data) {}
void connect_d(GtkWidget *widget, gpointer data) {}

void consume_a(GtkWidget *widget, gpointer data) {
    static char text[100];
    sprintf(text, "%s %d", text, *(int*)consume_single(&cons1));
    gtk_entry_set_text(GTK_ENTRY(output_a), text);
}

void consume_b(GtkWidget *widget, gpointer data) {
    static char text[100];
    sprintf(text, "%s %d", text, *(int*)consume_single(&cons2));
    gtk_entry_set_text(GTK_ENTRY(output_b), text);
}

void consume_c(GtkWidget *widget, gpointer data) {}
void consume_d(GtkWidget *widget, gpointer data) {}

void *successor_thread (void *stream) {
    stream_t *self = (stream_t*)stream;
    int i = 0;
    int count = 0;
    int *value;
    char text[100];

    while (1){
        value = (int*)malloc(sizeof(int));
        *value = count;
        printf("Successor put %d into index %d\n", count, self->put_idx);
        put(self, (void*)value);
        count++;

        /* only try to loop through and draw the buffer when it's filled
         * with valid pointers
         */
        if (count > 4) {
            memset(text, 0, 100);
            for (i=0; i<BUFFER_SIZE; i++) {
                sprintf(text, "%s %d ", text, *(int*)self->buffer[i]);
            }
            gtk_entry_set_text(GTK_ENTRY(output_suc), text);
        }
    }

    pthread_exit(NULL);
}

void init_window(void) {

    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *button_table;

    GtkWidget *button_a_connect;
    GtkWidget *button_b_connect;
    GtkWidget *button_c_connect;
    GtkWidget *button_d_connect;

    GtkWidget *button_a_consume;
    GtkWidget *button_b_consume;
    GtkWidget *button_c_consume;
    GtkWidget *button_d_consume;

    GtkWidget *output_a_box;
    GtkWidget *output_b_box;
    GtkWidget *output_c_box;
    GtkWidget *output_d_box;
    GtkWidget *output_suc_box;

    GtkWidget *label_a;
    GtkWidget *label_b;
    GtkWidget *label_c;
    GtkWidget *label_d;
    GtkWidget *label_suc;

    GtkWidget *seperator;

    gtk_init(NULL, NULL);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
    gtk_window_set_title(GTK_WINDOW(window), "Producer Consumer");

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    button_table = gtk_table_new(4, 2, FALSE);

    button_a_connect = gtk_button_new_with_label("Connect A");
    button_b_connect = gtk_button_new_with_label("Connect B");
    button_c_connect = gtk_button_new_with_label("Connect C");
    button_d_connect = gtk_button_new_with_label("Connect D");

    button_a_consume = gtk_button_new_with_label("Consume A");
    button_b_consume = gtk_button_new_with_label("Consume B");
    button_c_consume = gtk_button_new_with_label("Consume C");
    button_d_consume = gtk_button_new_with_label("Consume D");

    gtk_table_attach_defaults(GTK_TABLE(button_table), button_a_connect, 0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(button_table), button_b_connect, 1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(button_table), button_c_connect, 2, 3, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(button_table), button_d_connect, 3, 4, 0, 1);

    gtk_table_attach_defaults(GTK_TABLE(button_table), button_a_consume, 0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(button_table), button_b_consume, 1, 2, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(button_table), button_c_consume, 2, 3, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(button_table), button_d_consume, 3, 4, 1, 2);

    output_a_box = gtk_hbox_new(FALSE, 1);
    output_b_box = gtk_hbox_new(FALSE, 1);
    output_c_box = gtk_hbox_new(FALSE, 1);
    output_d_box = gtk_hbox_new(FALSE, 1);
    output_suc_box = gtk_hbox_new(FALSE, 1);

    label_a = gtk_label_new("A: ");
    label_b = gtk_label_new("B: ");
    label_c = gtk_label_new("C: ");
    label_d = gtk_label_new("D: ");
    label_suc = gtk_label_new("Successor buffer: ");

    output_a = gtk_entry_new();
    output_b = gtk_entry_new();
    output_c = gtk_entry_new();
    output_d = gtk_entry_new();
    output_suc = gtk_entry_new();

    gtk_entry_set_editable(GTK_ENTRY(output_a), FALSE);
    gtk_entry_set_editable(GTK_ENTRY(output_b), FALSE);
    gtk_entry_set_editable(GTK_ENTRY(output_c), FALSE);
    gtk_entry_set_editable(GTK_ENTRY(output_d), FALSE);
    gtk_entry_set_editable(GTK_ENTRY(output_suc), FALSE);

    gtk_box_pack_start(GTK_BOX(output_a_box), label_a, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_b_box), label_b, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_c_box), label_c, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_d_box), label_d, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(output_suc_box), label_suc, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(output_a_box), output_a, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(output_b_box), output_b, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(output_c_box), output_c, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(output_d_box), output_d, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(output_suc_box), output_suc, TRUE, TRUE, 0);

    seperator = gtk_hseparator_new();

    gtk_box_pack_start(GTK_BOX(vbox), button_table, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), output_a_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), output_b_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), output_c_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), output_d_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), seperator, FALSE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), output_suc_box, TRUE, TRUE, 5);

    g_signal_connect_swapped(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    g_signal_connect(button_a_connect, "clicked", G_CALLBACK(connect_a), NULL);
    g_signal_connect(button_b_connect, "clicked", G_CALLBACK(connect_b), NULL);
    g_signal_connect(button_c_connect, "clicked", G_CALLBACK(connect_c), NULL);
    g_signal_connect(button_d_connect, "clicked", G_CALLBACK(connect_d), NULL);

    g_signal_connect(button_a_consume, "clicked", G_CALLBACK(consume_a), NULL);
    g_signal_connect(button_b_consume, "clicked", G_CALLBACK(consume_b), NULL);
    g_signal_connect(button_c_consume, "clicked", G_CALLBACK(consume_c), NULL);
    g_signal_connect(button_d_consume, "clicked", G_CALLBACK(consume_d), NULL);

    gtk_widget_show_all(window);
}

int main(int argc, char** argv) {

    init_window();

    pthread_t s1;

    init_stream(&suc1, NULL);
    init_stream(&cons1, NULL);
    init_stream(&cons2, NULL);
    init_stream(&cons3, NULL);
    init_stream(&cons4, NULL);

    stream_connect(&cons1, &suc1);
    stream_connect(&cons2, &suc1);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&s1, &attr, successor_thread, (void*)&suc1);

    /*
    pthread_create(&c1, &attr, consumer, (void*)&cons1);
    pthread_create(&c2, &attr, consumer, (void*)&cons2);
    pthread_create(&c3, &attr, consumer, (void*)&cons3);
    pthread_create(&c4, &attr, consumer, (void*)&cons4);
    */


    gtk_main();

    return 0;
}

