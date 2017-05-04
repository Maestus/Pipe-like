#include <gtk/gtk.h>
#include <string.h>
#include "../conduct.c"
#define BUFFER_SIZE 100

struct data {
  struct conduct * conduit;
};

GtkWidget *window;
GtkWidget *grid;
GtkWidget *Lecture, *Ecriture, *Transfert;

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

  struct data flux = *(struct data *) arg;
  int f_source;
  struct stat st;
  if((f_source = open(source, O_RDONLY, 0666)) == - 1){
    printf("%s\n", strerror(errno));
  }

  fstat(f_source, &st);

  char * size_file = malloc(10);
  sprintf(size_file, "%ld", st.st_size);
  conduct_write(flux.conduit, size_file, 10);

  int nread = 0;
  void * buffer = malloc(100);

  while ((nread = read(f_source, buffer, 100)) > 0) {
    conduct_write(flux.conduit, buffer, nread);
    buffer = malloc(100);
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
    buffer = malloc(100);
    nread = conduct_read(flux.conduit, buffer, 100);
    if(nread > 0) {
      write(f_destination, buffer, nread);
    }
  }while(nread > 0);

  close(f_destination);
  return NULL;
}


static void worker(GtkButton *button, gpointer user_data) {

  struct data flux;
  flux.conduit = conduct_create(NULL, 6, 600);
  printf("%ld\n", flux.conduit->capacity);
  pthread_t reader, writer;

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
}

int main(int argc, char **argv){

  gtk_init(&argc, &argv);

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
