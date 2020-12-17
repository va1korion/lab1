#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include "plugin_api.h"
#include <limits.h>

#define BUFFSIZE 255
#define MAX_PLUGINS 8
#define SHORT_OPTS 6
#define MAX_FILES 100
#define COND_AND 1
#define COND_OR 0

static const char *version = "0.1";
static int out_buff_len = BUFFSIZE, ans_len = 0, negative = 0, condition = COND_AND;
char **fnames_ans;

//function that adds name of the plugin, so we can find it in getopt
struct option* add_plugin_option(struct option *longoptions, int *len, struct plugin_info *pluginInfo){
    longoptions[*len].name = pluginInfo->sup_opts->opt.name;       //shitty code, yet should work just fine
    longoptions[*len].has_arg = required_argument;
    longoptions[*len].flag = malloc(BUFFSIZE);
    *longoptions[*len].flag = 0;
    longoptions[*len].val = 2;
    *len += 1;
    longoptions = realloc(longoptions, *len*sizeof(struct option));
    longoptions[*len].name = 0;
    longoptions[*len].has_arg = 0;
    longoptions[*len].flag = 0;
    longoptions[*len].val = 0;
    return longoptions;
}


int process_dir(char* dir, int (**pp_file)(const char *,
                                  struct option *[],
                                  size_t,
                                  char *,
                                  size_t), struct option *in_opts[], FILE* log, int handle_len){
    struct dirent *local_dirent;
    char *err_buff = malloc(out_buff_len);
    DIR* current_dir = opendir(dir);
    do{
        local_dirent = readdir(current_dir);
        if(local_dirent == NULL){
            return 0;
        }

        char *fullname = malloc(PATH_MAX);
        strcpy(fullname, "");
        strcat(fullname, realpath(dir, NULL));
        strcat(fullname, "/");
        strcat(fullname, local_dirent->d_name);

        if (local_dirent->d_type == DT_REG) {
            int found = 0;
            //magic happens here

            for (int i = 0; i < handle_len; i++) {

                if ((pp_file[i] == NULL) || (in_opts[i] == NULL)) {
                    continue;
                }
                fprintf(log, "Processing file: %s (run %s) \n", fullname, in_opts[0][i].name);
                fflush(log);


                // I am deeply ashamed of theese
                void *poi = &in_opts[0][i];
                int x = (*pp_file[i])(fullname, (struct option **)&poi, 1, err_buff, out_buff_len);


                if (x <= -1){
                    fprintf(log, "Plugin has returned: %s \n", err_buff);
                    fflush(log);
                    continue;
                }
                x = !x;

                if (x ^ negative){
                    //((x && !negative) || (!x && negative)){
                    fprintf(log, "Criteria %s found in %s \n", in_opts[0][i].name, fullname);
                    fflush(log);
                    if (!found) {            //if it's the first plugin running for that file
                        ans_len += 1;
                        found += 1;
                        fnames_ans[ans_len - 1] = malloc(strlen(fullname));
                        strcpy(fnames_ans[ans_len - 1], fullname);
                    }

                }else{
                    if (found && (condition == COND_AND)) {
                        free(fnames_ans[ans_len - 1]);
                        ans_len -= 1;
                        break;
                    }
                    if (condition == COND_AND){
                        fprintf(log, "Criteria %s not found in %s \n", in_opts[0][i].name, fullname);
                        break;
                    }
                }


            }
            free(fullname);

        }
        if (local_dirent->d_type == DT_DIR) {
            if ((strcmp(local_dirent->d_name, ".") == 0) || (strcmp(local_dirent->d_name, "..") == 0)){
                continue;
            }
            process_dir(fullname, pp_file, in_opts, log, handle_len);
            free(fullname);
        }


    }while (local_dirent != NULL);
    return 0;
}



int main(int argc, char** argv) {
    /* basic intialization
     * should be in init, but whatever*/
    char* log_file_name = "lab1log.txt";
    void* handle[MAX_PLUGINS];
    struct dirent *local_dirent;
    struct option in_opts[MAX_PLUGINS];
    struct plugin_info **ppi = malloc(sizeof(struct plugin_info*) * MAX_PLUGINS);
    for (int i = 0; i < MAX_PLUGINS; i++){
        ppi[i] = malloc(sizeof(struct plugin_info));
    }
    int c = 0, longoption_len = SHORT_OPTS, error = 0, handle_len = 0;
    char *search_dir_name, *plugin_dir_name;
    int option_index = 0;
    int (*get_info)(struct plugin_info*);
    condition = COND_AND;
    plugin_dir_name = ".";

    struct option *longoptions = malloc(sizeof(struct option) * (SHORT_OPTS));


    longoptions[0].name = "C";
    longoptions[0].has_arg = required_argument;
    longoptions[0].flag = NULL;

    longoptions[1].name = "N";
    longoptions[1].has_arg = no_argument;

    longoptions[2].name = "v";
    longoptions[2].has_arg = no_argument;

    longoptions[3].name = "h";
    longoptions[3].has_arg = no_argument;

    longoptions[4].name = "P";
    longoptions[4].has_arg = required_argument;

    longoptions[5].name = "l";
    longoptions[5].has_arg = required_argument;

    longoptions[longoption_len].name = 0;
    longoptions[longoption_len].has_arg = 0;

    FILE* log = stderr;

    opterr = 0;
    DIR* plugin_dir = opendir(plugin_dir_name);
    do{
        local_dirent = readdir(plugin_dir);
        if (local_dirent == NULL){
            break;
        }

        if (local_dirent->d_type == DT_REG){
            if (strstr(local_dirent->d_name, ".so") != NULL){
                char *fullname = malloc(PATH_MAX);
                strcpy(fullname, "");
                strcat(fullname, plugin_dir_name);
                strcat(fullname, "/");
                strcat(fullname, local_dirent->d_name);
                //fullname = realpath(fullname, NULL);
                fprintf(log, "processing .so: %s \n", fullname);
                fflush(log);

                handle[handle_len] = dlopen(fullname, RTLD_LAZY);

                if(handle[handle_len] == NULL){

                    free(fullname);
                    continue;
                }

                get_info = (int (*)(struct plugin_info *)) dlsym(handle[handle_len], "plugin_get_info");

                if (!get_info){
                    free(fullname);
                    continue;
                }
                handle_len++;

                error = get_info(ppi[handle_len-1]);

                for(int i; i < ppi[handle_len-1]->sup_opts_len; i++){
                    fprintf(log, "Adding a search option %s ... Return code: %i \n", ppi[handle_len-1]->sup_opts->opt.name, error);
                    fflush(log);
                    longoptions = add_plugin_option(longoptions, &longoption_len, ppi[handle_len-1]);
                    fprintf(log, "Added a search option %s \n", longoptions[longoption_len-1].name);
                    fflush(log);
                }
                free(fullname);
            }
        }
    }
    while (local_dirent != NULL);


    search_dir_name = argv[argc-1];
    DIR* search_dir = opendir(search_dir_name);


    if(search_dir == NULL){
        fprintf(log, "Version: %s  What help do you hope to get? \n"
               "This program looks for plugins in cwd (also in directory specified in -P argument) \n"
               "Only single-option plugins are currently supported \n"
               "Then it recursively processes all the files in a directory specified in a last argument \n"
               "*This directory MUST be specified as the LAST argument\n", version);
        fprintf(log, "Could not open directory %s \n", search_dir_name);
        fflush(log);
        exit(0);
    }

    //looking for plugins
    while (c != -1) {
        c = getopt_long_only(argc, argv, "-P:C:l:Nvh",
                             longoptions, &option_index);

        switch (c) {
            case 'v':
                printf("Version: %s \n", version);
                exit(0);
            case 'P':
                plugin_dir_name = optarg;
                argv[optind-1] = "0";
                break;
            case 'h':
                printf("Version: %s What help do you hope to get? \n"
                   "This program looks for plugins in cwd (also in directory specified in -P argument) \n"
                   "Only single-option plugins are currently supported \n"
                   "Then it recursively processes all the files in a directory specified in a last argument \n"
                   "*This directory must be specified as the LAST argument", version);
                exit(0);

            case 'l':
                log_file_name = optarg;
                argv[optind-1] = "0";
                break;
            default:
                continue;
        }
    }

    log = fopen(log_file_name, "w");
    plugin_dir = opendir(plugin_dir_name);

    if(plugin_dir) {
        fprintf(log, "opened directory %s \n", plugin_dir_name);
        fflush(log);
    }
    fprintf(log, "Search dir: %s \n", search_dir_name);
    fflush(log);
    do{
        local_dirent = readdir(plugin_dir);
        if (local_dirent == NULL){
            break;
        }

        if (local_dirent->d_type == DT_REG){
            if (strstr(local_dirent->d_name, ".so") != NULL){
                char *fullname = malloc(PATH_MAX);
                strcpy(fullname, "");
                strcat(fullname, plugin_dir_name);
                strcat(fullname, "/");
                strcat(fullname, local_dirent->d_name);
                //fullname = realpath(fullname, NULL);
                fprintf(log, "processing .so: %s \n", fullname);
                fflush(log);

                handle[handle_len] = dlopen(fullname, RTLD_LAZY);

                if(handle[handle_len] == NULL){
                    fprintf(log, "but it's NULL \n");
                    fflush(log);
                    free(fullname);
                    continue;
                }

                get_info = (int (*)(struct plugin_info *)) dlsym(handle[handle_len], "plugin_get_info");

                if (!get_info){
                    fprintf(log, "dlsym has returned an error for %s \n", dlerror());
                    dlclose(handle[handle_len]);
                    free(fullname);
                    continue;
                }
                handle_len++;
                fprintf(log, "searching for get_info in .so: %s \n", fullname);
                fflush(log);
                error = get_info(ppi[handle_len-1]);


                for(int i; i < ppi[handle_len-1]->sup_opts_len; i++){
                    fprintf(log, "Adding a search option %s ... Return code: %i \n", ppi[handle_len-1]->sup_opts->opt.name, error);
                    fflush(log);
                    longoptions = add_plugin_option(longoptions, &longoption_len, ppi[handle_len-1]);
                    fprintf(log, "Added a search option %s \n", longoptions[longoption_len-1].name);
                    fflush(log);
                }
                free(fullname);
            }
        }
    }
    while (local_dirent != NULL);


    fprintf(log, "Plugins processed, parsing getopt \n");
    fflush(log);
    optind = 0;


    while (1) {
        option_index = 0;

        c = getopt_long_only(argc, argv, "-P:C:l:Nvh",
                        longoptions, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case 'C':
                fprintf(log, "option C with value '%s'\n", optarg);
                if (strcmp(optarg, "AND") == 0)
                    continue;
                else if (strcmp(optarg, "OR") == 0)
                        condition = COND_OR;
                else fprintf(stderr, "Option -C invoked with wrong arguments. Only \"AND\" and \"OR\" are supported\n");
                continue;
            case 'N':
                fprintf(log, "Negative option detected \n");
                negative = 1;
                continue;
            case 0:
                for(int j = SHORT_OPTS; j < longoption_len; j++)
                {
                    if (option_index == j)
                    {
                        longoptions[option_index].flag = (int*) optarg;
                        fprintf(log, "File criteria added %s %s \n",longoptions[option_index].name, optarg);
                        fflush(log);
                        continue;
                    }
                }

            // could rewrite for tidier code (proper flags and vals in options)

            default:
                continue;


        }
    }



    int x = 0;
    int (**process_file)() = malloc(MAX_PLUGINS* sizeof(void*));
    for(int i = 0; i < MAX_PLUGINS; i++){
        process_file[i] = NULL;
    }

    //dlsyming pp_file from plugins
    for(int i = 0; i < handle_len; i++){   //if we have anything to search for we are looking for something to look with
        if(*longoptions[SHORT_OPTS + i].flag != 0)
        {
            fprintf(log, "Hello there %s \n", longoptions[SHORT_OPTS + i].name);
            fflush(log);
            process_file[i] = (int (*)()) dlsym(handle[i], "plugin_process_file");

        }
    }

    ans_len = 0;
    fnames_ans = malloc(MAX_FILES * sizeof(char*));

    longoptions = longoptions+SHORT_OPTS;
    error = process_dir(search_dir_name, process_file, &longoptions, log, handle_len);




    fflush(log);
    fprintf(log, "Files found: \n");
    fprintf(stdout, "Files found: \n");
    for(int j = 0; j < ans_len; j++){
        fprintf(log, "%s  \n", fnames_ans[j]);
        fprintf(stdout, "%s  \n", fnames_ans[j]);
    }




    //doing some cleanup
    for(int i = 0; i < handle_len; i++){
        dlclose(handle[i]);
    }
    closedir(plugin_dir);
    closedir(search_dir);
    for (int i = 0; i < MAX_PLUGINS; i++){
        free(ppi[i]->sup_opts);
        free(ppi[i]);
    }
    free(ppi);
   // free(longoptions);
    return 0;
}