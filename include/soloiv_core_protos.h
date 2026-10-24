/*
 * soloiv_core_protos.h -- prototypes for the soloiv_core library
 * (the perusal/ sources).
 *
 * Convention (same as s2_protos.h, which describes it in full):
 *
 *   - Every non-static function defined in perusal/ that is called
 *     from another file gets its prototype here, grouped by source
 *     file, and new functions must be added here.
 *   - No K&R declarations ("int foo();"), at file scope or inside
 *     functions.
 *   - This header must stay GTK-free: translate/ includes it, and
 *     translate/ is compiled without GTK types in scope.  Functions
 *     whose signatures need GTK types belong in a separate header
 *     that only GTK code includes.
 *
 * So far it holds only the functions that translate/ calls; it is to
 * be completed when perusal/ is converted to prototypes.
 */

#ifndef INCsoloiv_core_protosh
#define INCsoloiv_core_protosh

/* solo2.c */
void solo_message(char *message, ...);

#endif /* INCsoloiv_core_protosh */
