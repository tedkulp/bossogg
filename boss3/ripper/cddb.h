
typedef struct cddb_t {
  char *genre;
  int year;
  char *artist;
  char *title;
} cddb_s;

text_tag_s **query_cd (cdrom_drive *drive);
