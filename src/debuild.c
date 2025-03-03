#define DEBUILD_IMPLEMENTATION
#include "debuild.h"

int main()
{
    deb_init();

    Deb_Cmd cmd = {0};
    deb_cmd_append(&cmd, "cc", "-w", "../../src/debuild.c", "-o", "../target/debuild_new.exe");

    //deb_file_copy("../../test/test1/foo.txt", "../../test/test2/bar.txt");
    //deb_file_copy("../target/debuild.exe", "../target/debuild_old.exe");
    //deb_file_copy("../target/debuild_new.exe", "../target/debuild.exe");

    char* buf;

    while (buf = deb_dir_list("../../"))
    {
        printf(deb_file_exists(buf) ? "file: " : "dir: ");
        printf("%s\n", buf);
    }

    if (deb_cmd_execute(&cmd))
        deb_log(Deb_LogType_Info, "Build success");
    else
        deb_log(Deb_LogType_Error, "Build failed");

    return 0;
}
