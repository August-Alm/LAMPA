/* ***** ***** */

#include <stdio.h>
#include "src/lambda_parser.h"

/* ***** ***** */

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 1;
    } else {
        FILE *fp = fopen(argv[1], "r");
        if (fp) {
            // Works! 
            //struct names *xs = alloc_names(64);
            //struct terms1 *t1 = parse_terms1(fp);
            //struct terms2 *t2 = lam2db(t1, xs);
            //free_terms1(t1);
            //free_terms2(t2);
            //free_names(xs);
            
            // Works!
            //struct names *xs = alloc_names(16);

            //struct terms2 *t2 = parse_terms2(fp, xs);
            //fprintf_terms2(stdout, t2); printf("\n"); 
            //struct terms1 *t1 = db2lam(t2, xs);
            //fprintf_terms1(stdout, t1); printf("\n"); 
            //free_terms2(t2);
            //free_terms1(t1);

            //struct terms2 *e2 = parse_terms2(fp, xs);
            //fprintf_terms2(stdout, e2); printf("\n"); 
            //struct terms1 *e1 = db2lam(e2, xs);
            //fprintf_terms1(stdout, e1); printf("\n"); 
            //free_terms2(e2);
            //free_terms1(e1);

            //free_names(xs);
            
            // Works!
            //struct contexts1 *ctx = alloc_contexts1(16);
            //struct terms1 *t0 = parse_declterms1(fp, ctx);
            //fprintf_terms1(stdout, t0); printf("\n");
            //struct terms1 *t1 = parse_declterms1(fp, ctx);
            //fprintf_terms1(stdout, t1); printf("\n");
            //struct terms1 *t2 = parse_declterms1(fp, ctx);
            //fprintf_terms1(stdout, t2); printf("\n");
            //struct names *xs = alloc_names(16);
            //struct terms2 *t3 = lam2db(t2, xs);
            //fprintf_terms2(stdout, t3); printf("\n");
            //decref_terms2(t3);
            //free_names(xs);
            //free_contexts1(ctx);

            // Works!
            struct names *xs = alloc_names(16);
            struct contexts2 *ctx = alloc_contexts2(16);
            struct terms2 *t0 = parse_declterms2(fp, xs, ctx);
            fprintf_terms2(stdout, t0); printf("\n");
            struct terms2 *t1 = parse_declterms2(fp, xs, ctx);
            fprintf_terms2(stdout, t1); printf("\n");
            struct terms2 *t2 = parse_declterms2(fp, xs, ctx);
            fprintf_terms2(stdout, t2); printf("\n");
            free_contexts2(ctx);
            free_names(xs);

            fclose(fp);
        } else {
            fprintf(stderr, "Error reading file.\n");
            return 1;
        }
    }
    return 0;
}

