#ifdef WIN32
#define ITE6811_INLINE _inline
#else
#ifndef ITE6811_INLINE
#  if defined(__GNUC__)
#    define ITE6811_INLINE __inline__
#  else
#    define ITE6811_INLINE inline
#  endif 
#endif 
#endif 
