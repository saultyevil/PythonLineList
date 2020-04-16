/* bound.c */
void bound_bound_main_menu(void);
void get_bound_bound_lines(void);
/* buffer.c */
void clean_up_display_buffer(void);
void display_text_buffer(WINDOW *win, int start_y, int start_x);
void add_to_display_buffer(char *fmt, ...);
void add_separator_to_buffer (const int len);
/* main.c */
int main_menu(int current_index);
void process_main_menu_choices(int choice);
int main(int argc, char *argv[]);
/* menu.c */
void clean_up_menu(MENU *menu, ITEM **items, int nitems);
int control_menu(MENU *menu, int c);
int update_menu_window(char *menu_message, char **menu_items, int nitems, int current_index, int user_control_menu);
/* tools.c */
void get_element_name(int z, char *element);
int check_command_line(int argc, char **argv);
/* ui.c */
void initialise_ncurses_stdscr(void);
void cleanup_ncurses_stdscr(void);
void initialise_main_windows(void);
void draw_window_boundaries(void);
void bold_message(WINDOW *win, int y, int x, char *msg, ...);
/* xsections.c */
void photoionization_home_menu(void);
void get_photoionization_cross_sections(double wmin, double wmax);
/* read_atomic_data.c */
int fraction(double value, double array[], int npts, int *ival, double *f, int mode);
int linterp(double x, double xarray[], double yarray[], int xdim, double *y, int mode);
int index_phot_top(void);
int index_inner_cross(void);
void indexx(int n, float arrin[], int indx[]);
int limit_lines(double freqmin, double freqmax);
int check_xsections(void);
double a21(struct lines *line_ptr);
double upsilon(int n_coll, double u0);
int index_lines(void);
int get_atomic_data(char *masterfile);
/* query.c */
int query_atomic_data(void);
double get_wavelength(WINDOW *win, char *msg, int y, int x, int len);
void query_wavelength_range(double *wmin, double *wmax);
