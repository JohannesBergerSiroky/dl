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
#include <signal.h>


bool have_used_color;
sigset_t signals_received;
static sig_atomic_t volatile interrupt_sig;
static sig_atomic_t volatile stop_sgnl_count;

#ifndef SA_NOCLDSTP
#define SA_NOCLDSTP 0
#endif
#define ARRAY_CARDINALITY(Array) (sizeof (Array) / sizeof *(Array)) 

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

char table[9][4] = {{'_','_','_', '\0'}, {'_','_','x','\0'},{'_','w','_','\0'},{'_','w','x','\0'},{'r','_','_','\0'},{'r','_','x','\0'},{'r','w','_','\0'},{'r','w','x','\0'},{'\0','\0','\0','\0'}};
char file_permissions[32];

void init_some_variables();
static bool has_color(enum color_picker indicator);
void set_color_default();
static bool
write_color (const struct color_code *clrcode);
void prepare_color_output();
void write_color_indicator(const struct color_code* ind);
char* get_grame(int gid);
char* get_uname(int uid);
char* parse_file_permissions(int stmode);
void init_signal_handling(bool active);
static void s_normalhandler(int sgnl);
static void s_stophandler (int sgnl);
static void dl_handle_signals(void);
void restore_signal(void);
void init_signal(void);

void restore_signal(void)
{
        init_signal_handling(false);
}
void init_signal(void)
{
        init_signal_handling(true);
}

static void dl_handle_signals(void)
{
        while (interrupt_sig || stop_sgnl_count) {
                int sgnl;
                int s_stop;
                sigset_t oldset;

                if(have_used_color) {
                        set_color_default();
                        fflush(stdout);
                }

                sigprocmask(SIG_BLOCK, &signals_received, &oldset);
                sgnl = interrupt_sig;
                s_stop = stop_sgnl_count;

                if(s_stop) {
                        stop_sgnl_count = s_stop - 1;
                        sgnl = SIGSTOP;
                }
                else {
                        signal(sgnl, SIG_DFL);
                }
                /*Stop the program */
                raise(sgnl);
                sigprocmask(SIG_SETMASK, &oldset, NULL);        
        }
}

static void s_normalhandler(int sgnl)
{
        if(!SA_NOCLDSTP) {
                signal(sgnl, SIG_IGN);
        }
        if(! interrupt_sig) {
                interrupt_sig = sgnl;
        }

}

static void s_stophandler(int sgnl)
{
        if(!SA_NOCLDSTP) {
                signal (sgnl, s_stophandler);
        }
        if(interrupt_sig) {
                interrupt_sig = sgnl;
        }
}

/* This function is largley taken from parts of the code of ls.c in coreutils.*/
void init_signal_handling(bool active)
{

        static int possible_signals [] = {
        SIGTERM, SIGINT, 
        SIGHUP,  SIGQUIT,
        SIGPIPE, SIGALRM,
        SIGTSTP,

        #ifdef SIGPOLL
                SIGPOLL,
        #endif

        #ifdef SIGPROF
                SIGPROF,
        #endif

        #ifdef SIGVTALRM
                SIGVTALRM,
        #endif

        #ifdef SIGXCPU
                SIGXCPU,
        #endif

        #ifdef SIGCFSZ
                SIGXFSZ,
        #endif
        };

        //size_t size_possible_signals = sizeof(possible_signals)/sizeof(possible_signals[0]);
        enum {possible_signals_size = ARRAY_CARDINALITY(possible_signals)};
        #if ! SA_NOCLDSTP
        static bool signals_recvd[possible_signals_size];
        #endif
        if(active) {
                for(int i = 0; i < possible_signals_size;i++) {
                        signals_recvd[i] = 0;
                }
                #if SA_NOCLDSTOP
                        struct sigaction s_act;
                        sigemptyset(&signals_received);
                        
                        for (int i = 0; i < possible_signals_size;i++) {
                                sigaction(possible_signals[i], NULL, &s_act);
                                if (s_act.sa_handler != SIG_IGN) {
                                        sigaddset(&signals_received, possible_signals[i]);
                                }
                        }

                        s_act.sa_mask = signals_received;
                        s_act.sa_flags = SA_RESTART;

                        for(int i = 0; i < possible_signals_size; i++) {
                                if(sigismember(&signals_received, possible_signals[i])) {
                                        if(possible_signals[i] == SIGTSTP) {
                                                s_act.sa_handler = s_stophandler;
                                        }
                                        else {
                                                s_act.sa_handler = s_normalhandler;
                                        }
                                        sigaction(possible_signals[i], &s_act, NULL);
                                }
                        }
                #else

                        for (int i = 0; i < possible_signals_size;i++) {
                                signals_recvd[i] = signal(possible_signals[i], SIG_IGN) != SIG_IGN;
                                /* If it's not SIG_IGN */
                                if(signals_received[i]) {
                                        signal(possible_signals[i], possible_signals[i] == SIGTSTP ? s_stophandler : s_normalhandler);
                                        siginterrupt(possible_signals[i], 0);
                                }
                        }
                #endif
        }

        else {
               #if SA_NOCLDTSTP
                       for (int i = 0; i < possible_signals_size;i++) {
                               if(sigismember(&signals_received, possible_signals[i])) {
                                        signal(possible_signals[i], SIG_DFL);
                                }
                       }
                #else
                        for(int i = 0; i < possible_signals_size; i++) {
                                if(signals_recvd[i]) {
                                        signal(possible_signals[i], SIG_DFL);
                                }
                        }
                #endif
        }
}
 
void init_some_variables()
{
        have_used_color = false;
        interrupt_sig = 0;
        stop_sgnl_count = 0;
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
                if(0 <= STDOUT_FILENO)
                        init_signal();
                prepare_color_output();
        }
        fwrite(ind->color_code_string, ind->cc_length, 1, stdout);
}

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

        dl_handle_signals();
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
        bool        test = false;
        int         b = 0;
        int         i = 0;
        int         l = 0;
        int         j = 0;
        int         v = 0;
        int         f_count = 0;
        int         d_count = 0;
        int         count = 0;
        int         saved_errno;
        int         f_stat;

        char        dir_path[1024] = ".";
        char        temp_path[1024];
        char        d_files[4096];
        char        d_dirs[2048];
        char        temp_path2[128];        
        char        *ptr = temp_path2;
        char        *uname;
        char        *gname;

        struct dirent* dir;
        struct stat *file_information;

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

        init_some_variables();

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
                        dl_handle_signals();
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

        test = write_color(&color_code_placeholder[BOLD_GREEN]);
        if(!test) {
                exit(5);
        }
        else {
                color_code_placeholder[COLOR_NORMAL].cc_length = sizeof("01;33") - 1;
                color_code_placeholder[COLOR_NORMAL].color_code_string = "01;33";
        } 

        printf("\n%-21s%-20s%-20s%s\n", name, permissions, usergroupid, unamegname);
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
                        dl_handle_signals();
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
                        dl_handle_signals();
                }
                for(int a = 0; a<1024; a++)
                        temp_path[a] = '\0';
        }

        if(have_used_color && test) {
                fflush(stdout);
                restore_signal();

                for(int i = stop_sgnl_count; i; i--) {
                        raise(SIGSTOP);
                }

                i = interrupt_sig;
                if(i)
                        raise(i);
        }
        
        free(file_information);
        return 0;       
}
