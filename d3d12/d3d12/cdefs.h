#ifndef CDEFS_H
#define CDEFS_H
#ifdef __cplusplus
#define BEGIN_CDECL extrern "C" {
#define END_CDECL }
#define BEGIN_UNNAMED namespace {
#define END_UNNAMED }
#else
#define BEGIN_CDECL
#define END_CDECL
#define BEGIN_UNNAMED
#define END_UNNAMED
#endif

#define KiB(n) ((n) * 1024)
#define MiB(n) KiB((n) * 1024)
#define GiB(n) MiB((n) * 1024)
#define TiB(n) GiB((n) * 1024)

#define COPY_ON_RENDER_CORE
#define WITH_VIRTUAL_ALLOC
#define WITHOUT_SWAP_CHAIN

#endif	/* !CDEFS_H */
