
typedef struct rip_opts_t {
  int read_sectors;
  int bitrate;
  int min_bitrate;
  int max_bitrate;
  int managed;
  double quality;
  int quality_set;
  char *device;
} rip_opts_s;

typedef struct stats_t {
  int minutes, seconds;
  int tracknum, tracktot;
  int done;
  char **filenames_c;
} stats_s;

int rip (cdrom_drive *drive, text_tag_s **text_tags, char **filenames);
