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
#include <stdbool.h>



/* #define RESET   "\033[0m"
#define GREEN   "\033[32m"   
#define YELLOW  "\033[33m"     
#define BOLDGREEN   "\033[1m\033[32m"     
#define BOLDYELLOW  "\033[1m\033[33m" */
bool have_used_color;



struct color_code
{
        int cc_length;
        char* color_code_string;
};

static struct color_code color_code_placeholder[] =
        {
                {sizeof("\033[") - 1, "\033["},   // left
                {sizeof("m") - 1, "m"},           // right
                {0, (char*)0},                    // End color
                {sizeof("0") - 1, "0"},           // color reset
                {0, (char*)0},                    // normal color
                {0, (char*)0},                    // File default
                {sizeof("32") - 1, "32"},         // green
                {sizeof("33") - 1, "33"},         // yellow
                {sizeof("01;32") - 1, "01;32"},   // bold green
                {sizeof("01;33") - 1, "01;33"},   // bold yellow
                {0, (char*)0},                    // Missing file
                {0, (char*)0},                    // orphaned symlink
                {0, (char*)0},                    // disabled by default
                {0, (char*)0},                    // disabled by default
                {sizeof("\033[K") - 1, "\033[K"}, // clear to end of line

        };

enum color_picker
        {
                LEFT,
                RIGHT,
                END_COLOR,
                COLOR_RESET,
                COLOR_NORMAL,
                FILE_DEFAULT,
                GREEN,
                YELLOW,
                BOLD_GREEN,
                BOLD_YELLOW,
                FILE_MISSING,
                ORPHANED_SYMLINK,
                DISABLEDBD,
                DISABLED_BY_DEFAULT,
                CLTEOL
        };

void init_have_used_color();
static bool has_color(enum color_picker indicator);
void set_color_default();
static bool
write_color (const struct color_code *clrcode);
void prepare_color_output();
void write_color_indicator(const struct color_code* ind);

void init_have_used_color()
{
        have_used_color = false;
}

static bool has_color(enum color_picker indicator)
{
        int len = color_code_placeholder[indicator].cc_length;
        char* str = color_code_placeholder[indicator].color_code_string;
        if(len == 0) {
                return false;
        }
        else if(strcmp("0", str) == 0) {
                return false;
        }
        else if(strcmp("00", str) == 0) {
                return false;
        }
        else {
                return true;
        }
}

void set_color_default()
{
        write_color_indicator (&color_code_placeholder[LEFT]);
        write_color_indicator (&color_code_placeholder[RIGHT]);
}

static bool
write_color (const struct color_code* clrcode)
{
        if (clrcode) {
                /* Avoid attribute combinations */
                if (has_color (COLOR_NORMAL))
                        set_color_default();

                write_color_indicator (&color_code_placeholder[LEFT]);
                write_color_indicator (clrcode);
                write_color_indicator (&color_code_placeholder[RIGHT]);
        }

        return clrcode != (struct color_code*)0;
}

void prepare_color_output()
{
        if(&color_code_placeholder[END_COLOR] != (struct color_code*)0) {
                write_color_indicator(&color_code_placeholder[END_COLOR]);
        }
        else {
                write_color_indicator(&color_code_placeholder[LEFT]);
                write_color_indicator(&color_code_placeholder[COLOR_RESET]);
                write_color_indicator(&color_code_placeholder[RIGHT]);
        }
}

void write_color_indicator(const struct color_code* ind)
{
        if(have_used_color == false) {
                have_used_color = true;
                prepare_color_output();
        }
        fwrite(ind->color_code_string, ind->cc_length, 1, stdout);
}




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

int main(int args, char** argv)
{
        DIR         *d;
        int         b = 0;
        int         i = 0;
        int         l = 0;
        int         j = 0;
        int         v = 0;
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


        if(args > 2) {
                printf("\nUsage: dl, dl --help, dl -h or dl -v.\n");
                exit(1);
        }
        if (args == 2) {
                if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
                        printf("\nType dl -v for full file names and full directory names.\nOtherwise it will show a maximum of twenty letters from\na file name and from a directory name.\n");
                        exit(2);
                }
                else if ((strcmp(argv[1], "-v") == 0)) {
                        v = 1;
                }
                else {
                        printf("\nUsage: dl, dl -h, dl --help or dl -v\n");
                        exit(3);
                }
        }

        init_have_used_color();

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
                                }

                                        j++;
                                        d_count++;
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
        char name[48];
        for(int a = 0; a<48; a++)
                name[a] = '\0';
        strcpy(name, "Directories");        
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
        char temp[48];
        for(int a = 0; a<48; a++)
                temp[a] = '\0';
        bool test = write_color(&color_code_placeholder[BOLD_GREEN]);
        if(!test) {
                exit(5);
        }
        else {
                color_code_placeholder[COLOR_NORMAL].cc_length = sizeof("01;33") - 1;
                color_code_placeholder[COLOR_NORMAL].color_code_string = "01;33";
        } 

        printf("\n%-21s%-20s%-20s%s\n", name, permissions, usergroupid, unamegname);
        //set_color_default();
        file_information = (struct stat*)calloc(count, sizeof(struct stat));
        // allocate some members in the struct stat 
        for(int a = 0; a<1024; a++)
                temp_path[a] = '\0';
        for(int a = 0; a<128; a++)
                temp_path2[a] = '\0';
        unsigned char* statptr = (unsigned char*)file_information;
        for(int a = 0; a < (count*(sizeof(struct stat))); a++)
                *(statptr + a) = 0;

        test = write_color(&color_code_placeholder[GREEN]);
        if(!test) {
                exit(6);
        }
        else {
                color_code_placeholder[COLOR_NORMAL].cc_length = sizeof("32") - 1;
                color_code_placeholder[COLOR_NORMAL].color_code_string = "32";
        }
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
                        b = strlen(d_dirs + l);

                        if ((b > 20) && (v == 0)) {
                                d_dirs[l + 20] = '\0';
                        }
 
                        strcpy(name, (d_dirs + l));
                        snprintf(usergroupid, sizeof(usergroupid), "%d", (file_information + i)->st_uid);
                        strcat(usergroupid, " ");
                        snprintf(temp, sizeof(temp), "%d", (file_information + i)->st_gid);
                        strcat(usergroupid, temp);

                        for(int a = 0; a<48; a++)
                                unamegname[a] = '\0';

                        uname = get_uname((file_information + i)->st_uid);
                        gname = get_gname((file_information + i)->st_gid);
                        strcpy(unamegname, uname);
                        strcat(unamegname, "/");
                        strcat(unamegname, gname);

                        printf("%-21s%-20s%-20s%s\n", name, parse_file_permissions((file_information + i)->st_mode), usergroupid, unamegname);
                        l += b;
                        l++;
                        i++;
                }

                for(int a = 0; a<1024; a++)
                        temp_path[a] = '\0';

        }

        j = i;
        i = 0;
        l = 0;


        for(int a = 0; a<48; a++)
                name[a] = '\0';
        strcpy(name, "Files");        
        for(int a = 0; a<48; a++)
                permissions[a] = '\0';
        strcpy(permissions, "Permissions");
        for(int a = 0; a<48; a++)
                usergroupid[a] = '\0';
        strcpy(usergroupid, "User/Group id");
         for(int a = 0; a<48; a++)
                unamegname[a] = '\0';
        strcpy(unamegname, "Username/Groupname");
        test = write_color(&color_code_placeholder[BOLD_YELLOW]);
        if(!test) {
                exit(7);
        }
        else {
                color_code_placeholder[COLOR_NORMAL].cc_length = sizeof("01;33") - 1;
                color_code_placeholder[COLOR_NORMAL].color_code_string = "01;33";
        }
        printf("\n%-21s%-20s%-20s%s\n", name, permissions, usergroupid, unamegname);
        set_color_default();
        /*test = write_color(&color_code_placeholder[COLOR_RESET]);
        if(!test) {
                exit(8);
        }
        else {
                color_code_placeholder[COLOR_NORMAL].cc_length = sizeof("0") - 1;
                strcpy(color_code_placeholder[COLOR_NORMAL].color_code_string, "0";
        }*/
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
                        b = strlen(d_files + l);

                        if ((b > 20) && (v == 0)) {
                                d_files[l + 20] = '\0';
                        }

                        strcpy(name, (d_files + l));
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
                        printf("%-21s%-20s%-20s%s\n", name, parse_file_permissions((file_information + i)->st_mode), usergroupid, unamegname);
                        l+= b;
                        l++;
                        i++;
                }
                for(int a = 0; a<1024; a++)
                        temp_path[a] = '\0';
        }
        
        free(file_information);
        return 0;
        
}



