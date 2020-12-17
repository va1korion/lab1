// compile with:
// gcc -fPIC -shared testplugin.c -o testplugin.so

#include <stdlib.h>
#include "plugin_api.h"


int plugin_get_info(struct plugin_info* ppi) {
    if (ppi == NULL) {
        return -1;
    }
    
    ppi->plugin_name = "testplugin";
    ppi->sup_opts_len = 2;

    struct plugin_option *sup_opts = calloc(2, sizeof(struct plugin_option));
    
    sup_opts[0].opt.name = "first-option";
    sup_opts[0].opt.has_arg = no_argument;
    sup_opts[0].opt_descr = "First test option";
    
    sup_opts[1].opt.name = "second-option";
    sup_opts[1].opt.has_arg = required_argument;
    sup_opts[1].opt_descr = "Second test option";
    
    ppi->sup_opts = sup_opts;
    
    return 0;
}

int plugin_process_file(const char *fname,
        struct option *in_opts[],
        size_t in_opts_len,
        char *out_buff,
        size_t out_buff_len) {
        
    return -1;
}
