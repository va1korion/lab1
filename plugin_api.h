#ifndef _PLUGIN_API_H
#define _PLUGIN_API_H

#include <getopt.h>

/*
    Структура, описывающая опцию, поддерживаемую плагином.
*/
struct plugin_option {
    /* Опция в формате, поддерживаемом getopt_long (man 3 getopt_long). */
    struct option opt;
    /* Описание опции, которое предоставляет плагин. */
    const char *opt_descr;
};

/*
    Структура, содержащая информацию о плагине.
*/
struct plugin_info {
    /* Название плагина */
    const char *plugin_name;

    /* Длина списка опций */
    size_t sup_opts_len;

    /* Список опций, поддерживаемых плагином */
    struct plugin_option *sup_opts;
};

/*
    Функция, позволяющая получить информацию о плагине.
    
    Аргументы:
        ppi - адрес структуры, которую заполняет информацией плагин.
        
    Возвращаемое значение:
        0 - в случае успеха,
        1 - в случае неудачи (в этом случае продолжать работу с этим плагином нельзя).
*/
int plugin_get_info(struct plugin_info* ppi);


/*
    Фунция, позволяющая выяснить, отвечает ли файл заданным критериям.
    
    Аргументы:
        in_opts - список опций (критериев поиска), которые передаются плагину.
           struct option {
               const char *name;
               int         has_arg;
               int        *flag;
               int         val;
           };
           Поле name используется для передачи имени опции, поле flag - для передачи
           значения опции (в виде строки). Если у опции есть аргумент, поле has_arg
           устанавливается в ненулевое значение.
           
        in_opts_len - длина списка опций.
        
        out_buff - буфер, предназначенный для возврата данных от плагина. В случае ошибки
            в этот буфер, если в данном параметре передано ненулевое значение, 
            копируется сообщение об ошибке.
            
        out_buff_len - размер буфера. Если размера буфера оказалось недостаточно, функция
            завершается с ошибкой.
                    
    Возвращаемое значение:
          0 - файл отвечает заданным критериям,
        > 0 - файл НЕ отвечает заданным критериям,
        < 0 - в процессе работы возникла ошибка (код ошибки).
*/

int plugin_process_file(const char *fname,
        struct option *in_opts[],
        size_t in_opts_len,
        char *out_buff,
        size_t out_buff_len);
        
#endif