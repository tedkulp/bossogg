
#define MP3_FILE 0
#define OGG_FILE 1
#define FLAC_FILE 2
                                                                                
#define ARTIST 0
#define TITLE 1
#define ALBUM 2
#define GENRE 3
#define YEAR 4
#define TRACK 5

#define FN __FUNCTION__
#define LN __LINE__
#define FL __FILE__

typedef struct text_tag_t {
  char *artistname;
  char *songname;
  char *albumname;
  char *genre;
  int year;
  int track;
  int bitrate;
  int frequency;
  double songlength;
} text_tag_s;

#ifdef __cplusplus
extern "C" {
#endif
	
/* alloc / free a text_tag_s */
text_tag_s *new_text_tag (void);
void free_text_tag (text_tag_s *text_tag);
 
/* print a text_tag_s to stdout */
void print_text_tag (text_tag_s *text_tag);
                                                                                
/* fill in a text_tag_s from the given file */
text_tag_s *get_text_tag (char *filename);

/* log a message (for errors, etc) */
void log_msg (const char *msg, const char *file, const char *function, int line);

  /* some functions from python used in other modules */
#ifdef PYTHON
void getTag_helper_string (PyObject *d, char *key, char *str);
void getTag_helper_list (PyObject *d, char *key, PyObject *l);
PyObject *filenames_helper_array_string (int size, char **str);
void getTag_helper_int (PyObject *d, char *key, int num);
void getTag_helper_double (PyObject *d, char *key, double num);
void getTag_helper (PyObject *d, text_tag_s *text_tag);
text_tag_s *setTag (PyObject *tag);
PyObject *getTag (char *filename);
#endif
  
#ifdef __cplusplus
}
#endif
