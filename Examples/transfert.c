#include <gtk/gtk.h>
#include <string.h>
#include "../conduct.h"
#define BUFFER_SIZE 100

struct data {
    struct conduct * conduit;
};

GtkWidget *window;
GtkWidget *grid;
GtkWidget *Lecture, *Ecriture, *Transfert;

int capacite = 600;
int atomicite = 6;
int echange = 100;

int work_done = 0;
int work_start = 0;

char * source;
char * destination;

static void openfc(GtkButton *button, gpointer user_data){

    gint res;
    GtkWidget * dialog;

    if( button == (GtkButton *) Lecture ){
        dialog =  gtk_file_chooser_dialog_new("Open file", (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_OPEN,
                                              "Cancel", GTK_RESPONSE_CANCEL,
                                              "Open", GTK_RESPONSE_ACCEPT,
                                              NULL);

        res = gtk_dialog_run (GTK_DIALOG (dialog));
        if (res == GTK_RESPONSE_ACCEPT){
            GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
            source = gtk_file_chooser_get_filename (chooser);
            gtk_button_set_label ((GtkButton *)Lecture, source);
            gtk_widget_set_sensitive (Ecriture, TRUE);
        }
    } else {
        GtkFileChooser *chooser;
        dialog =  gtk_file_chooser_dialog_new("Save file", (GtkWindow *) window, GTK_FILE_CHOOSER_ACTION_SAVE,
                                              "Cancel", GTK_RESPONSE_CANCEL,
                                              "Save", GTK_RESPONSE_ACCEPT,
                                              NULL);

        chooser = GTK_FILE_CHOOSER (dialog);
        gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

        res = gtk_dialog_run (GTK_DIALOG (dialog));
        if (res == GTK_RESPONSE_ACCEPT){
            destination = gtk_file_chooser_get_filename (chooser);
            gtk_button_set_label ((GtkButton *)Ecriture, destination);
            gtk_widget_set_sensitive (Transfert, TRUE);
        }
    }
    gtk_widget_destroy (dialog);
}


void * read_source(void *arg){

    work_start = 1;
    struct data flux = *(struct data *) arg;
    int f_source;
    struct stat st;
    if((f_source = open(source, O_RDONLY, 0666)) == - 1){
        printf("%s\n", strerror(errno));
    }

    fstat(f_source, &st);

    printf("Taille du fichier à transférer : %ld octects\n", st.st_size);

    char * size_file = malloc(10);
    sprintf(size_file, "%ld", st.st_size);
    conduct_write(flux.conduit, size_file, 10);

    int nread = 0;
    void * buffer = malloc(echange);

    while ((nread = read(f_source, buffer, echange)) > 0) {
        conduct_write(flux.conduit, buffer, nread);
        buffer = malloc(echange);
    }
    conduct_write_eof(flux.conduit);

    close(f_source);
    return NULL;
}


void * write_destination(void *arg){

    struct data flux = *(struct data *) arg;
    int f_destination;
    if((f_destination = open(destination, O_CREAT | O_RDWR, 0666)) == - 1){
        printf("%s\n", strerror(errno));
    }

    char * size_file = malloc(10);
    conduct_read(flux.conduit, size_file, 10);

    int size = atoi(size_file);

    if (ftruncate(f_destination, size) == -1){
        return NULL;
    }

    int nread = 0;
    void * buffer;

    do {
        buffer = malloc(echange);
        nread = conduct_read(flux.conduit, buffer, echange);
        if(nread > 0) {
            write(f_destination, buffer, nread);
        }
    }while(nread > 0);

    close(f_destination);
    return NULL;
}


void * init_timer(void *arg){

    struct timespec start, stop;

    while(work_start == 0){}

    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
        perror("clock");
        exit(1);
    }

    work_start = 0;

    while (work_done == 0) {}

    if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
        perror("clock");
        exit(1);
    }

    work_done = 0;

    printf("Transfert fini en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

    return NULL;
}


static void worker(GtkButton *button, gpointer user_data) {

    struct data flux;
    flux.conduit = conduct_create(NULL, atomicite, capacite);

    if(flux.conduit == NULL){
        printf("%s\n", strerror(errno));
        exit(1);
    }

    printf("Conduit construit !\n");
    printf("Capacité : %ld\n", flux.conduit->capacity);
    printf("Atomicité : %ld\n", flux.conduit->atomic);

    pthread_t reader, writer, time_seeker;

    if(pthread_create(&time_seeker, NULL, init_timer, NULL)){
        fprintf(stderr,"Erreur");
        exit(1);
    }

    if(pthread_create(&reader, NULL, read_source, &flux)){
        fprintf(stderr,"Erreur");
        exit(1);
    }

    if(pthread_create(&writer, NULL, write_destination, &flux)){
        fprintf(stderr,"Erreur");
        exit(1);
    }

    pthread_join(reader, NULL);
    pthread_join(writer, NULL);
    work_done = 1;
    pthread_join(time_seeker, NULL);
    gtk_widget_set_sensitive (Ecriture, FALSE);
    gtk_widget_set_sensitive (Transfert, FALSE);
    gtk_button_set_label ((GtkButton *)Lecture,"Depuis...");
    gtk_button_set_label ((GtkButton *)Ecriture,"Vers...");

}

int main(int argc, char **argv){

    gtk_init(&argc, &argv);

    if(argc == 2 && (capacite = atoi(argv[1])) < 0){
        return 1;
    }

    if(argc == 3 && (capacite = atoi(argv[1])) < 0 && (atomicite = atoi(argv[2])) < 0){
        return 1;
    }

    if(argc == 4 && (capacite = atoi(argv[1])) < 0 && (atomicite = atoi(argv[2])) < 0 && (echange = atoi(argv[3])) < 0){
        return 1;
    }

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 100);
    gtk_window_set_title (GTK_WINDOW (window), "Transfert");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    grid = gtk_grid_new ();
    gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (grid));

    Lecture = gtk_button_new_with_label("Depuis...");
    Ecriture = gtk_button_new_with_label("Vers...");
    Transfert = gtk_button_new_with_label("Transférer !");

    gtk_grid_attach(GTK_GRID(grid),Lecture,1,0,1,1);
    gtk_grid_attach(GTK_GRID(grid),Ecriture,1,1,1,1);
    gtk_grid_attach(GTK_GRID(grid),Transfert,1,2,1,1);

    gtk_grid_set_column_homogeneous(GTK_GRID(grid),TRUE);

    g_signal_connect (Lecture, "clicked", G_CALLBACK (openfc), NULL);
    g_signal_connect (Ecriture, "clicked", G_CALLBACK (openfc), NULL);
    g_signal_connect (Transfert, "clicked", G_CALLBACK (worker), NULL);

    gtk_widget_set_sensitive (Ecriture, FALSE);
    gtk_widget_set_sensitive (Transfert, FALSE);

    gtk_widget_show_all (window);

    gtk_main();

    return 0;
}
