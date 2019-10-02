#if !defined(__INCLUDE_SYS_BOOTMM_H)
#define __INCLUDE_SYS_BOOTMM_H
void bootmm_init(void *vstart, void *vend);
void bootmm_free(char *v);
char *bootmm_alloc(void);
#endif // __INCLUDE_SYS_BOOTMM_H
