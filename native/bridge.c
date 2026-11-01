#define _XOPEN_SOURCE 700
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#define API __declspec(dllexport)
#else
#define API __attribute__((visibility("default")))
#endif
#ifdef __APPLE__
#include <CoreText/CoreText.h>
#elif !defined(_WIN32)
#include <fontconfig/fontconfig.h>
#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>
#endif
/* All calls must be serialized on one application thread. No global error buffer. */
typedef struct { char *family, *face, *postscript; } Face;
typedef struct { Face *faces; size_t count; char error[256]; } Snapshot;
typedef struct Registration { char *path; unsigned refs; struct Registration *next; } Registration;
typedef struct { Registration *entry; char error[256]; } Lease;
static Registration *registrations;
static char *copy(const char *s) { size_t n = strlen(s)+1; char *p = malloc(n); if(p) memcpy(p,s,n); return p; }
static int append(Snapshot *s, const char *family, const char *face, const char *ps) {
    Face f = {copy(family), copy(face), copy(ps)};
    if(!f.family || !f.face || !f.postscript) { free(f.family); free(f.face); free(f.postscript); strcpy(s->error,"Out of memory"); return 0; }
    Face *p = realloc(s->faces, (s->count+1)*sizeof(Face));
    if(!p) { free(f.family); free(f.face); free(f.postscript); strcpy(s->error,"Out of memory"); return 0; }
    s->faces=p; s->faces[s->count++]=f; return 1;
}
#ifdef __APPLE__
static char *cftext(CFStringRef s) {
    if(!s) return copy("");
    CFIndex n=CFStringGetMaximumSizeForEncoding(CFStringGetLength(s),kCFStringEncodingUTF8)+1;
    char *p=malloc((size_t)n); if(p && !CFStringGetCString(s,p,n,kCFStringEncodingUTF8)) { free(p); return NULL; } return p;
}
static CFURLRef fonturl(const char *p) { return CFURLCreateFromFileSystemRepresentation(NULL,(const UInt8 *)p,(CFIndex)strlen(p),false); }
#elif defined(_WIN32)
static wchar_t *wide(const char *s) {
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,NULL,0); if(!n) return NULL;
    wchar_t *p=malloc((size_t)n*sizeof(wchar_t)); if(p) MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,p,n); return p;
}
static char *utf8(const wchar_t *s) {
    int n=WideCharToMultiByte(CP_UTF8,0,s,-1,NULL,0,NULL,NULL); if(!n) return NULL;
    char *p=malloc((size_t)n); if(p) WideCharToMultiByte(CP_UTF8,0,s,-1,p,n,NULL,NULL); return p;
}
static int CALLBACK enumerate(const LOGFONTW *lf,const TEXTMETRICW *tm,DWORD type,LPARAM data) {
    (void)tm; (void)type; const ENUMLOGFONTEXW *e=(const ENUMLOGFONTEXW *)lf;
    char *family=utf8(e->elfLogFont.lfFaceName), *face=utf8(e->elfFullName);
    Snapshot *s=(Snapshot *)data;
    if(!family || !face) { strcpy(s->error,"UTF conversion/allocation failed"); free(family); free(face); return 0; }
    int ok=append(s,family,face,""); free(family); free(face); return ok;
}
#else
static const char *fcstr(FcPattern *p,const char *key) { FcChar8 *s=NULL; return FcPatternGetString(p,key,0,&s)==FcResultMatch ? (const char *)s : ""; }
static void invalidate(void) {
    PangoFontMap *map=pango_cairo_font_map_get_default();
    if(PANGO_IS_FC_FONT_MAP(map)) pango_fc_font_map_config_changed(PANGO_FC_FONT_MAP(map));
}
#endif
API void *bfk_snapshot(void) {
    Snapshot *s=calloc(1,sizeof(*s)); if(!s) return NULL;
#ifdef __APPLE__
    CFArrayRef names=CTFontManagerCopyAvailablePostScriptNames();
    if(!names) { strcpy(s->error,"Core Text enumeration failed"); return s; }
    for(CFIndex i=0;i<CFArrayGetCount(names);i++) {
        CTFontRef font=CTFontCreateWithName(CFArrayGetValueAtIndex(names,i),12,NULL);
        if(!font) continue;
        CFStringRef fa=CTFontCopyFamilyName(font), fn=CTFontCopyFullName(font), ps=CTFontCopyPostScriptName(font);
        char *a=cftext(fa),*b=cftext(fn),*c=cftext(ps);
        if(a && b && c) append(s,a,b,c); else strcpy(s->error,"UTF conversion/allocation failed");
        free(a); free(b); free(c); if(fa) CFRelease(fa); if(fn) CFRelease(fn); if(ps) CFRelease(ps); CFRelease(font);
        if(s->error[0]) break;
    }
    CFRelease(names);
#elif defined(_WIN32)
    HDC dc=CreateCompatibleDC(NULL); if(!dc) { strcpy(s->error,"Cannot create GDI DC"); return s; }
    LOGFONTW lf={0}; lf.lfCharSet=DEFAULT_CHARSET;
    EnumFontFamiliesExW(dc,&lf,enumerate,(LPARAM)s,0); DeleteDC(dc);
#else
    if(!FcInit()) { strcpy(s->error,"Fontconfig initialization failed"); return s; }
    FcPattern *p=FcPatternCreate(); FcObjectSet *os=FcObjectSetBuild(FC_FAMILY,FC_FULLNAME,FC_POSTSCRIPT_NAME,NULL);
    FcFontSet *fonts=p && os ? FcFontList(NULL,p,os) : NULL;
    if(!fonts) strcpy(s->error,"Fontconfig enumeration failed");
    else { for(int i=0;i<fonts->nfont;i++) if(!append(s,fcstr(fonts->fonts[i],FC_FAMILY),fcstr(fonts->fonts[i],FC_FULLNAME),fcstr(fonts->fonts[i],FC_POSTSCRIPT_NAME))) break; FcFontSetDestroy(fonts); }
    if(os) FcObjectSetDestroy(os);
    if(p) FcPatternDestroy(p);
#endif
    return s;
}
API const char *bfk_snapshot_error(Snapshot *s) { return s ? s->error : "Out of memory"; }
API int64_t bfk_count(Snapshot *s) { return s ? (int64_t)s->count : 0; }
API const char *bfk_field(Snapshot *s,int64_t i,int64_t field) {
    if(!s || i<0 || (uint64_t)i>=s->count) return "";
    return field==0 ? s->faces[i].family : field==1 ? s->faces[i].face : s->faces[i].postscript;
}
API void bfk_snapshot_free(Snapshot *s) {
    if(!s) return;
    for(size_t i=0;i<s->count;i++) { free(s->faces[i].family); free(s->faces[i].face); free(s->faces[i].postscript); } free(s->faces); free(s);
}
API void *bfk_register(const char *path) {
    Lease *l=calloc(1,sizeof(*l)); if(!l) return NULL;
    char *canonical=NULL;
#ifdef _WIN32
    wchar_t *w=wide(path); wchar_t full[32768];
    DWORD n=w ? GetFullPathNameW(w,32768,full,NULL) : 0; free(w);
    if(n && n<32768 && GetFileAttributesW(full)!=INVALID_FILE_ATTRIBUTES) canonical=utf8(full);
#else
    canonical=realpath(path,NULL);
#endif
    if(!canonical) { strcpy(l->error,"Font path does not exist or cannot be resolved"); return l; }
    for(Registration *r=registrations;r;r=r->next) if(strcmp(canonical,r->path)==0) { r->refs++; l->entry=r; free(canonical); return l; }
    Registration *r=calloc(1,sizeof(*r));
    if(!r) { free(canonical); strcpy(l->error,"Out of memory"); return l; }
    int ok=0;
#ifdef __APPLE__
    CFURLRef url=fonturl(canonical); CFErrorRef error=NULL;
    if(url) { ok=CTFontManagerRegisterFontsForURL(url,kCTFontManagerScopeProcess,&error); CFRelease(url); }
    if(error) CFRelease(error);
#elif defined(_WIN32)
    w=wide(canonical); if(w) { ok=AddFontResourceExW(w,FR_PRIVATE,NULL)>0; free(w); }
#else
    if(FcInit()) { ok=FcConfigAppFontAddFile(NULL,(const FcChar8 *)canonical); if(ok) invalidate(); }
#endif
    if(!ok) { strcpy(l->error,"Native font registration failed (invalid, unsupported, or already registered externally)"); free(canonical); free(r); return l; }
    r->path=canonical; r->refs=1; r->next=registrations; registrations=r; l->entry=r; return l;
}
API const char *bfk_registration_error(Lease *l) { return l ? l->error : "Out of memory"; }
/* Close is retryable on native failure. Linux retains the registry entry until exit. */
API int64_t bfk_close(Lease *l) {
    if(!l) return 1;
    Registration *r=l->entry;
    if(r) {
#if defined(__APPLE__) || defined(_WIN32)
        if(r->refs==1) {
            int ok=0;
#ifdef __APPLE__
            CFURLRef url=fonturl(r->path); CFErrorRef error=NULL;
            if(url) { ok=CTFontManagerUnregisterFontsForURL(url,kCTFontManagerScopeProcess,&error); CFRelease(url); }
            if(error) CFRelease(error);
#else
            wchar_t *w=wide(r->path); if(w) { ok=RemoveFontResourceExW(w,FR_PRIVATE,NULL)!=0; free(w); }
#endif
            if(!ok) { strcpy(l->error,"Native font unregistration failed; retain handle and retry"); return 0; }
            Registration **p=&registrations; while(*p!=r) p=&(*p)->next; *p=r->next; free(r->path); free(r);
        } else r->refs--;
#else
        r->refs--;
#endif
    }
    free(l); return 1;
}
API int64_t bfk_can_unload(void) {
#if defined(__APPLE__) || defined(_WIN32)
    return 1;
#else
    return 0;
#endif
}
API const char *bfk_backend(void) {
#ifdef __APPLE__
    return "CoreText-process";
#elif defined(_WIN32)
    return "GDI-private (not GDI+ private collections)";
#else
    return "Fontconfig-current + default PangoCairo Fc map";
#endif
}
