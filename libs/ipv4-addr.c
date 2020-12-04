#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include "../plugin_api.h"
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define PAGE 4096

int plugin_get_info(struct plugin_info *ppi){
    ppi->plugin_name = "ipv4-addr";
    ppi->sup_opts_len = 1;
    ppi->sup_opts->opt.name = "ipv4-addr";
    ppi->sup_opts->opt.has_arg = required_argument;
    ppi->sup_opts->opt_descr = "This option helps us call our plugin and let it do it's dirty job (which is finding ipv4 address in a file)";
    return 0;
}

int plugin_process_file(const char *fname,
                        struct option *in_opts[],
                        size_t in_opts_len,
                        char *out_buff,
                        size_t out_buff_len){
    char *ip_string = (char*)in_opts[0]->flag;

    int scan_res = 1, errno = 0;
    unsigned int a[4];
    off_t file_offset = 0, fsize;
    struct stat st;
    stat(fname, &st);
    fsize = st.st_size;
    scan_res = sscanf(ip_string, "%u.%u.%u.%u", &a[0], &a[1], &a[2], &a[3]);

    if ((scan_res != 4) || (a[0] > 255)|| (a[1] > 255)|| (a[2] > 255)|| (a[3] > 255)){
        strcpy(out_buff, "That is not an IP-address");
        return -1;
    }
    void *start = malloc(PAGE);

    int fd = open(fname, 0);
    unsigned int ip_le = (a[0] << 24)+(a[1] << 16)+(a[2] << 8)+(a[3]),
                 ip_be = (a[3] << 24)+(a[2] << 16)+(a[1] << 8)+(a[0]);
    unsigned int *bytes;
    char buffer[16];
    do{

        void *file_page = mmap(start, PAGE, PROT_READ, MAP_FILE | MAP_SHARED, fd, file_offset);
        if (file_page == MAP_FAILED){
            snprintf(out_buff, out_buff_len, "Map failed \n");
            return -1;
        }
        if (file_offset){
            char big_buffer[32];

            strncpy(big_buffer, buffer, 16);
            bytes = (unsigned int *)((char *)file_page);
            strncpy(big_buffer+16, (char *) bytes, 16);
            for(int i = 0; i < 16; i++){
                if (*bytes == ip_le)
                    return 1;

                if (*bytes == ip_be)
                    return 1;

                if (strncmp(big_buffer, ip_string, strlen(ip_string)) == 0) {
                    return 1;
                }
            }
        }

        for (int i = 0; i < PAGE-16; i++) {

            bytes = (unsigned int *)((char *)file_page + i);

            strncpy(buffer, (char *) bytes, 16);

            if (*bytes == ip_le)
                return 1;

            if (*bytes == ip_be)
                return 1;

            if (strncmp(buffer, ip_string, strlen(ip_string)) == 0) {
                return 1;
            }
        }
        munmap(file_page, PAGE);
        file_offset += PAGE;
    }while(file_offset < fsize);

    return 0;
}