/* 	$Id$	 */

#ifndef lint
static char vcid[] = "$Id$";
#endif /* lint */
# include <dorade_headers.h>
# include <s2_protos.h>

/* c------------------------------------------------------------------------ */

void by_products(struct dd_general_info *dgi, time_t ztime)
{
    dd_derived_fields(dgi);
    dd_pct_stats(dgi);		/* this routine is in the same file with
				 * dd_derived_fields
				 */
    produce_uf(dgi);
    dd_gecho(dgi);
    dd_product_x(dgi);
    produce_nssl_mrd(dgi, NO);
    produce_shanes_data(dgi);
    produce_ncdf( dgi, NO );

}
/* c------------------------------------------------------------------------ */
