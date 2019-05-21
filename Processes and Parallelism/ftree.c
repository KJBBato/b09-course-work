#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include "ftree.h"
#include "hash.h"
#include <libgen.h>

int file_sync(const char *src, const char *dest, char *file_name, struct stat src_stat);
int dir_sync(const char *src, const char *dest);
int copy_file(const char *src, const char *dest, int src_permissionion);

int copy_ftree(const char *src, const char *dest){
    struct stat src_stat;
    int processes = 0;

    // get the file name
    char *file_name = malloc(strlen(src)*sizeof(char *));
    strcpy(file_name, src);
    file_name = basename(file_name);

    // get the status of the file, return -1 on fails
    if(lstat(src, &src_stat) < 0){
        perror("Error");
        return -1;
    }

    // if src is a file
    if(S_ISREG(src_stat.st_mode)){
        // sync the file to the destination directory
        processes = file_sync(src, dest, file_name, src_stat);
    }

    // if src is a directory
    else if(S_ISDIR(src_stat.st_mode)){

        // sync the directories
        char *dest_path = malloc(strlen(src)*sizeof(char *)+2);
        strcpy(dest_path, dest);
        strcat(dest_path, "/");
        strcat(dest_path, file_name);

        // create the directory with the same permission as the source
        int pass = mkdir(dest_path, (src_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)));

        // check failure state
        if(pass < 0){
            DIR *dest_dir;

            // if directory already exits, update permission
            if ((dest_dir = opendir(dest_path)) != NULL){
                chmod(dest_path, (src_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)));
                closedir(dest_dir);
            } else {
                perror("Error");
                return -1;
            }
        }
        processes += dir_sync(src, dest_path);
        free(dest_path);
    }
    return processes;
}

int dir_sync(const char *src, const char *dest){
    struct dirent *next_file;
    struct stat next_stat;

    // string representation of the path to src
    char *src_path = malloc(strlen(src)*sizeof(char *)+2);
    strcpy(src_path, src);
    strcat(src_path, "/");
    pid_t result;
    int processes = 1;
    DIR *src_dir = opendir(src);

    // terminate on failure
    if(src_dir == NULL){
        perror("Error");
        return -1;
    }

    // loop through all the content in directory.
    while((next_file = readdir(src_dir))){

        // and int to indicate if a directory is empty
        int empty = 0;

        // find the first non-parent or current file
        while(next_file->d_name[0] == '.'){
            next_file = readdir(src_dir);

            // if end of directory is reached, break the loop
            if(next_file == NULL){
                empty = 1;
                break;
            }
        }

        // exit the loop if the directory is empty
        if(empty == 1){
            break;  
        }

        // create a sting of the file path
        char *src_file_path = malloc((strlen(next_file->d_name)+strlen(src))*sizeof(char *)+1);
        strcpy(src_file_path, src_path);
        strcat(src_file_path, next_file->d_name);
        lstat(src_file_path, &next_stat);

        //check for content type to perform the correct method
        if(S_ISREG(next_stat.st_mode)){
            file_sync(src_file_path, dest, next_file->d_name, next_stat);
        }

        else if(S_ISDIR(next_stat.st_mode)){

            // create a new process to handle a directory synchronization
            result = fork();

            // terminate process on fork failure
            if(result < 0){
                return -1;
            }
            else if(result == 0){
                int exit_status = 1;
                int pass;
  
                // create a path to the destination
                char *dest_subdir_path = malloc((strlen(next_file->d_name)+strlen(src))*sizeof(char *)+1);
                strcpy(dest_subdir_path, dest);
                strcat(dest_subdir_path, "/");
                strcat(dest_subdir_path, next_file->d_name);

                char *src_dir_path = malloc(2*strlen(src)*sizeof(char *));
                strcpy(src_dir_path, src_path);
                strcat(src_dir_path, next_file->d_name);
                
                // create the directory
                mkdir(dest_subdir_path, (next_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)));

                // sync the directory
                pass = dir_sync(src_dir_path, dest_subdir_path);
 
                // terminate on failure
                if(pass < 0){
                    return -1;
                }
 
                free(dest_subdir_path);
                free(src_dir_path);
                exit(exit_status);
 
                break;

            } else {
                processes ++;
            }
        }
        free(src_file_path);
    }

    // wait for all processes to complete
    wait(NULL);
    closedir(src_dir);
    return processes;
}


int file_sync(const char *src, const char *dest, char * file_name, struct stat src_stat){
    struct stat dest_file_stat;
    char *target = malloc((strlen(src)+strlen(dest)+2) * sizeof(char *));

    // create the path 
    strcpy(target, dest);
    strcat(target, "/");
    strcat(target, file_name);
    int return_val = 0;
    
    // if the file does not exist in destiantion, copy it.
    if(lstat(target, &dest_file_stat) < 0){
        return_val = copy_file(src, target, src_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
    } else {

        // if the sizes are different, overwrite it.
        if(src_stat.st_size != dest_file_stat.st_size){
            return_val = copy_file(src, target, src_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
        } else {

            FILE * src_file = fopen(src, "r");
            
            // check for failure
            if(src_file == NULL){
                perror("Error");
                return -1;
            }

            FILE * dest_file = fopen(target, "r");
            if(dest_file == NULL){
                perror("Error");
                return -1;
            }

            // check for hash
            char * hash1 = hash(src_file);
            char * hash2 = hash(dest_file);
            fclose(src_file);
            fclose(dest_file);
            
            // copy if hash are different
            if(strcmp(hash1,hash2) != 0){
                return_val = copy_file(src, target, src_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
            }
            
            // update the permission
            else{
                chmod(target, src_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
                return_val ++;
            }
        }
    }
    free(target);
    return return_val;
}


int copy_file(const char *src, const char *dest, int src_permission){
    FILE * src_file = fopen(src, "r");

    // exit if src file cannot be opened
    if(src_file == NULL){
        perror("Error");
        return -1;
    }

    FILE * dest_file = fopen(dest, "w");
    // exit if dest file cannot be opened
    if(dest_file == NULL){
        perror("Error");
        return -1;

    }

    // copy the file from source to destination
    char buffer[1024*1024];
    size_t bytes;
    while (0 < (bytes = fread(buffer, 1, sizeof(buffer), src_file))){
        fwrite(buffer, 1, bytes, dest_file);
    }

    fclose(src_file);
    fclose(dest_file);
    
    // update the permission on the copied file
    chmod(dest, src_permission);
    return 1;
}