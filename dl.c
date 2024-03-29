#include <stdio.h>
#include <sysexits.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>



#define RESET   "\033[0m"
#define GREEN   "\033[32m"   
#define YELLOW  "\033[33m"     
#define BOLDGREEN   "\033[1m\033[32m"     
#define BOLDYELLOW  "\033[1m\033[33m" 



char table[9][4] = {{'_','_','_', '\0'}, {'_','_','x','\0'},{'_','w','_','\0'},{'_','w','x','\0'},{'r','_','_','\0'},{'r','_','x','\0'},{'r','w','_','\0'},{'r','w','x','\0'},{'\0','\0','\0','\0'}};
char file_permissions[32];

char* get_grame(int gid);
char* get_uname(int uid);
char* parse_file_permissions(int stmode);
char* parse_file_permissions(int stmode)
{

        for(int a = 0; a<32; a++)
                file_permissions[a] = '\0';
        char* f_permissions;
        int test;
        // First the program will test for owner permissions
        test = ((stmode >> 6) & 7);
        f_permissions = strcat(file_permissions, *(table + test));
        f_permissions = strcat(file_permissions, "|");
 
        // Second the program will test for group permissions
        test = ((stmode >> 3) & 7);
        f_permissions = strcat(file_permissions, *(table + test));
        f_permissions = strcat(file_permissions, "|");
        // Third the program will test for permissions for others
        test = (stmode & 7);
        f_permissions = strcat(file_permissions, *(table + test));
        // check for suid, sgid and the sticky bit
        test = (((stmode >> 12) & 63));
        // A regular file
        if(test == 8) {
                // test for suid
                if(((stmode >> 9) & 7) == 4) {
                        //check if the file has execute permissions
                        if(strcmp((file_permissions + 2), "x") == 0) {
                                f_permissions[2] = 's';

                        }
                        else {
                                f_permissions[2] = 'S';

                        }


                }
                // test for sgid
                if(((stmode >> 9) & 7) == 2) {
                        //check if the file has execute permissions
                        if(strcmp((f_permissions + 6), "x") == 0) {
                                f_permissions[6] = 's';

                        }
                        else {
                                f_permissions[6] = 'S';

                        }


                }


        }
        // directory
        else if (test == 4) {
                //check for a sticky bit
                if(((stmode >> 9) & 7) == 1) {
                        //check if the file has execute permissions
                        if(strcmp((f_permissions + 10), "x") == 0) {
                                f_permissions[10] = 't';

                        }
                        else {
                                f_permissions[10] = 'T';

                        }


                }        



        }

        return f_permissions;
}



char* get_uname(int uid) {

        struct passwd *pws;
        pws = getpwuid(uid);
        return pws->pw_name;
        
}

char* get_gname(int gid) {

        struct group *grp;
        grp = getgrgid(gid);
        return grp->gr_name;
        
}

int main()
{


        DIR         *d;
        int         i = 0;
        int         l = 0;
        int         j = 0;
        int         f_count = 0;
        int         d_count = 0;
        int         count = 0;
        struct dirent* dir;
        char        dir_path[1024] = ".";
        char        temp_path[1024];
        char        d_files[4096];
        char        d_dirs[2048];
        char        temp_path2[128];
        struct stat *file_information;
        int         saved_errno;
        int         f_stat;
        
        char        *ptr = temp_path2;
        char        *uname;
        char        *gname;


        d_files[0] = '\0';
        d_files[4095] = '\0';
        d_dirs[0] = '\0';
        d_dirs[2047] = '\0';
        for(int a = 39; a<1024; a++)
                dir_path[a] = '\0';
        temp_path[0] = '\0';
        temp_path[1023] = '\0';
        errno = 0;
        d = opendir(dir_path);
        saved_errno = errno;


        if (d) {

                while (((dir = readdir(d)) != NULL)) {
                        l = strlen(dir->d_name);

                        if((dir->d_type==DT_REG)){

                                strcpy((d_files + i), dir->d_name);
                                i += l;
                                d_files[i] = '\0';
                                i++;
                                f_count++;
                        
                        }
                        else if ((dir->d_type==DT_DIR)) {
                                if(strcmp((dir->d_name), ".") != 0) {
                                        strcpy((d_dirs + j), dir->d_name);
                                        j += l;
                                        d_dirs[j] = '\0';
                                        j++;
                                        d_count++;


                                }


                        }

                }
                i = 0;
                j = 0;
                l = 0;
                count = d_count + f_count;


                closedir(d);
        }





        else {
                printf("Could not open the directory\n");
                closedir(d);
                return saved_errno;

        }
        char directory_name[48];
        for(int a = 0; a<48; a++)
                directory_name[a] = '\0';
        strcpy(directory_name, "Directories");        
        char d_permissions[48];
        for(int a = 0; a<48; a++)
                d_permissions[a] = '\0';
        strcpy(d_permissions, "Permissions");
        char d_usergroupid[48];
        for(int a = 0; a<48; a++)
                d_usergroupid[a] = '\0';
        strcpy(d_usergroupid, "User/Group id");
        char d_unamegname[48];
        for(int a = 0; a<48; a++)
                d_unamegname[a] = '\0';
        strcpy(d_unamegname, "Username/Groupname");
        char temp[48];
        for(int a = 0; a<48; a++)
                temp[a] = '\0';
    




        printf(BOLDGREEN "\n%-20s%-20s%-20s%s\n", directory_name, d_permissions, d_usergroupid, d_unamegname);
        printf(RESET);
        file_information = (struct stat*)calloc(count, sizeof(struct stat));
        // allocate some members in the struct stat 
        for(int a = 0; a<1024; a++)
                temp_path[a] = '\0';
        for(int a = 0; a<128; a++)
                temp_path2[a] = '\0';
        unsigned char* statptr = (unsigned char*)file_information;
        for(int a = 0; a < (count*(sizeof(struct stat))); a++)
                *(statptr + a) = 0;
        while(i < d_count) {
                strcpy(temp_path, "./");
                ptr = strcat(temp_path, (d_dirs + l));
                strcpy(temp_path, ptr);
                if((f_stat = stat(temp_path, (file_information + i))) == -1 ) {
                        free(file_information);
                        perror("stat");
                        exit(EXIT_FAILURE);
                }

                else if ((strcmp((d_dirs + l),"..") == 0)) {
                        l+= strlen(d_dirs + l);
                        l++;
                        i++;

                }
                else {
 
                        strcpy(directory_name, (d_dirs + l));
                        snprintf(d_usergroupid, sizeof(d_usergroupid), "%d", (file_information + i)->st_uid);
                        strcat(d_usergroupid, " ");
                        snprintf(temp, sizeof(temp), "%d", (file_information + i)->st_gid);
                        strcat(d_usergroupid, temp);
                        for(int a = 0; a<48; a++)
                                d_unamegname[a] = '\0';
                        uname = get_uname((file_information + i)->st_uid);
                        gname = get_gname((file_information + i)->st_gid);
                        strcpy(d_unamegname, uname);
                        strcat(d_unamegname, "/");
                        strcat(d_unamegname, gname);
                        printf(GREEN "%-20s%-20s%-20s%s\n", directory_name, parse_file_permissions((file_information + i)->st_mode), d_usergroupid, d_unamegname);
                        l+= strlen(d_dirs + l);
                        l++;
                        i++;
                }
                for(int a = 0; a<1024; a++)
                        temp_path[a] = '\0';



        }
        j = i;

        i = 0;
        l = 0;

        char file_name[48];
        for(int a = 0; a<48; a++)
                file_name[a] = '\0';
        strcpy(file_name, "Files");        
        char permissions[48];
        for(int a = 0; a<48; a++)
                permissions[a] = '\0';
        strcpy(permissions, "Permissions");
        char usergroupid[48];
        for(int a = 0; a<48; a++)
                usergroupid[a] = '\0';
        strcpy(usergroupid, "User/Group id");
        char unamegname[48];
        for(int a = 0; a<48; a++)
                unamegname[a] = '\0';
        strcpy(unamegname, "Username/Groupname");


        printf(BOLDYELLOW "\n%-20s%-20s%-20s%s\n", file_name, permissions, usergroupid, unamegname);

        while(i < f_count) {
                strcpy(temp_path, "./");
                ptr = strcat(temp_path, d_files + l);
                strcpy(temp_path, ptr);
                if((f_stat = stat(temp_path, (struct stat*)(file_information + i))) == -1) {
                        free(file_information);
                        perror("stat");
                        exit(EXIT_FAILURE);
                }

                else {
                        strcpy(file_name, (d_files + l));
                        snprintf(usergroupid, sizeof(usergroupid), "%d", (file_information + i)->st_uid);
                        strcat(usergroupid, " ");
                        for(int a = 0; a<48; a++)
                                temp[a] = '\0';
                        snprintf(temp, sizeof(temp), "%d", (file_information + i)->st_gid);
                        strcat(usergroupid, temp);
                        for(int a = 0; a<48; a++)
                                unamegname[a] = '\0';
                        uname = get_uname((file_information + i)->st_uid);
                        gname = get_gname((file_information + i)->st_gid);
                        strcpy(unamegname, uname);
                        strcat(unamegname, "/");
                        strcat(unamegname, gname);
                        printf(RESET "%-20s%-20s%-20s%s\n", file_name, parse_file_permissions((file_information + i)->st_mode), usergroupid, unamegname);
                        l+= strlen(d_files + l);
                        l++;
                        i++;
                }
                for(int a = 0; a<1024; a++)
                        temp_path[a] = '\0';

        }

        



        free(file_information);
        return 0;
        
}



