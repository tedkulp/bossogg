
#ifdef __cplusplus
extern "C" {
#endif
  
  /* the single wrapper function we need to export to C */
  int id3lib_wrapper (char *filename, text_tag_s **text_tag);
  
#ifdef __cplusplus
}
#endif
