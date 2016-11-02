#include "catalog.h"
#include "clean_buff.h"

/* Globals */
static size_t fd_cat;

/*Function to create catalog file.
Input:void
Output:int*/
int
init_catalog_store(char *path)
{

        int ret         =       -1;
        DIR *dp = NULL;
        char filename[1024],cat_path[1024];

        strcpy(cat_path,path);
        sprintf(cat_path, "%s/catalogs", cat_path);
        dp = opendir(cat_path);
        if (NULL == dp) {
                ret = mkdir(cat_path, 0777);
                if (ret < 0) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
        }
        sprintf (filename,"%s/filecatalog.txt",cat_path);
        fd_cat = open(filename, O_APPEND|O_CREAT|O_RDWR,
                S_IRUSR|S_IWUSR);
        if (fd_cat < 1) {
                fprintf(stderr, "%s\n", strerror(errno));
                goto out;
        }
        ret = 0;
out:
        if (dp != NULL)
                closedir(dp);
        return ret;

}

/*Function to write contents to a catalog file.
Input:char* filename
Output:int
*/
int
writecatalog(char *filename)
{

        int ret                 =       -1;
        char actualpath[PATH_MAX+1];
        char *real_path         =       NULL;
        int size_of_real_path   =       0;

        printf("[chikdol] file_name: %s\n", filename);

        if (filename == '\0' || filename == NULL) {
                goto out;
        }

        real_path = realpath(filename, actualpath);
        printf("[chikdol] Real_Path: %s\n", real_path);

        if (real_path == NULL) {
                fprintf(stderr, "%s\n", strerror(errno));
                printf("[chikdol] write catalog failed: %d, %s",__LINE__, __FILE__);
                goto out;
        }

        size_of_real_path = strlen(real_path);
        printf("[chikdol] size_of_real_path: %d\n", size_of_real_path);

        if (-1 == write(fd_cat, &size_of_real_path, int_size)) {
                fprintf(stderr, "%s\n", strerror(errno));
                printf("[chikdol] write catalog failed: %d, %s",__LINE__, __FILE__);
                goto out;
        }
        if (-1 == write(fd_cat, real_path, size_of_real_path)) {
                fprintf(stderr, "%s\n", strerror(errno));
                printf("[chikdol] write catalog failed: %d, %s",__LINE__, __FILE__);
                goto out;
        }
        ret = 0;
out:

        return ret;

}

/*Function to read all deduped files from a catalog file.
Input:void
Output:int
*/
int
readfilecatalog()
{

        struct stat              st;
        int ret          =       -1;
        char *buffer     =      NULL;
        int size         =       0;
        int length       =       0;

        fstat(fd_cat, &st);
        size = st.st_size;
        if (size > 0) {
                if (-1 == lseek(fd_cat, 0, SEEK_SET)) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
        } else {
                printf("This namespace dose not continue any files\n");
                goto out;
        }
        printf("\nAbsolute path of deduped files are:\n");
        while (size > 0) {
                ret = read(fd_cat, &length, int_size);
                if (ret == -1) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
                buffer = (char *)calloc(1, length + 1);
                ret = read(fd_cat, buffer, length);
                if (ret == -1) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
                buffer[length] = '\0';
                printf("\n%s\n", buffer);
                size -= (length + int_size);
                memset(buffer, 0, length+1);
                clean_buff(&buffer);
                ret = 1;
        }
        printf("\n");
out:
        return ret;

}

/*Function to reset all deduped files from a catalog file.
Input:void
Output:int
*/
int
reset_catalog(char *file_path, char *path)
{

        struct stat             st;
        int     size                    =          0;
        int     length                  =          0;
        int     ret                     =         -1;
        char    *buffer                 =       NULL;
        DIR     *dp                     =       NULL;
        int     fd                      =          0;
        char filename[1024], cat_path[1024], temp_filename[1024];

        strcpy(cat_path, path);
        sprintf(cat_path, "%s/store_block/catalogs", cat_path);

        dp = opendir(cat_path);
        if (NULL == dp) {
                fprintf(stderr, "%s\n", strerror(errno));
                goto out;
        }
        sprintf (filename,"%s/filecatalog.txt",cat_path);
        sprintf (temp_filename,"%s/temp_filecatalog.txt",cat_path);
        fd = open(temp_filename, O_RDONLY | O_WRONLY | O_TRUNC | O_CREAT,
                S_IRUSR|S_IWUSR);
        if (fd < 1) {
                fprintf(stderr, "%s\n", strerror(errno));
                goto out;
        }

        fstat(fd_cat, &st);
        size = st.st_size;
        /*rewind the stream pointer to the start of catalog file*/
        if (size > 0) {
                if (-1 == lseek(fd_cat, 0, SEEK_SET)) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
        }
        if (size == 0) {
                ret = 1;
                goto out;
        }
        while (size > 0) {
                ret = read(fd_cat, &length, int_size);
                if (ret == -1) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
                buffer = (char *)calloc(1, length+1);
                ret = read(fd_cat, buffer, length);
                if (ret == -1) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
                buffer[length] = '\0';
                if (strcmp(file_path, buffer) != 0) {
                        if (-1 == write(fd, &length, int_size)) {
                                fprintf(stderr, "%s\n", strerror(errno));
                                goto out;
                        }
                        if (-1 == write(fd, buffer, length)) {
                                fprintf(stderr, "%s\n", strerror(errno));
                                goto out;
                        }
                }
                size -= (length + int_size);
                memset(buffer, 0, length+1);
                clean_buff(&buffer);
        }
        ret = remove(filename);
        if (ret < 0)
                goto out;
        ret = rename(temp_filename,filename);
        if (ret < 0)
                goto out;
        ret = 0;
out:
        if(fd != -1)
                close(fd);
        if (dp != NULL)
                closedir(dp);
        return ret;

}

/*Function to compare absolute path of file in file catalog.
Input:char out[],int fd_cat
Output:int
*/
int
comparepath(char out[])
{
	/*
		[chikdol] Input must be ABSOLUTE PATH!
	*/ 

        struct stat             st;
        int     size    =               0;
        size_t  length  =               0;
        int     ret     =               -1;
        char    *buffer  =               NULL;

        fstat(fd_cat, &st);
        
        size = st.st_size;
        printf("[chikdol] comparepath size: %d " , size);
        /*rewind the stream pointer to the start of catalog file*/
        if (size > 0) {
                if (-1 == lseek(fd_cat, 0, SEEK_SET)) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
        }
        if (size == 0) {
                ret = 1;
                goto out;
        }
        while (size > 0) {
                ret = read(fd_cat, &length, int_size);
                if (ret == -1) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
                buffer = (char *)calloc(1, length+1);
                ret = read(fd_cat, buffer, length);
                if (ret == -1) {
                        fprintf(stderr, "%s\n", strerror(errno));
                        goto out;
                }
                buffer[length] = '\0';
                printf("[chikdol] out - %s : buffer - %s", out, buffer);
                if (strcmp(out, buffer) == 0) {
                        ret = 0;
                        clean_buff(&buffer);
                        break;
                }
                size -= (length + int_size);
                memset(buffer, 0, length+1);
                clean_buff(&buffer);
                ret = 1;
        }
out:
        return ret;

}

/*Function to close catalog fd.
Input:void
Output:int*/
int
fini_catalog_store()
{

        int ret         =       -1;

        if (fd_cat != -1)
                ret = close(fd_cat);
        if (ret == -1) {
                fprintf(stderr, "%s\n", strerror(errno));
                goto out;
        }
        ret = 0;
out:
return ret;

}
