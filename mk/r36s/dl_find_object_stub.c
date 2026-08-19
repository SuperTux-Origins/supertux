/* Stub for glibc < 2.35 when linking static libgcc_eh from GCC 15+.
 * libgcc's exception unwinder calls _dl_find_object; older ArkOS glibc
 * does not export it. Returning -1 falls back to the slower path.
 */
struct dl_find_object; /* opaque */

__attribute__((weak))
int _dl_find_object(void *address, struct dl_find_object *result)
{
  (void)address;
  (void)result;
  return -1;
}
