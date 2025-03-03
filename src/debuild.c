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

    Deb_StringView buf;

    while (deb_dir_list("../../", &buf))
        printf("%s\n", buf.data);

    //printf("%d %d %d\n", deb_file_exists("build.bat"), deb_dir_exists("../target/debuild.exe"), deb_dir_exists("../../src"));

    if (deb_cmd_execute(&cmd))
        deb_log(Deb_LogType_Info, "Build success");
    else
        deb_log(Deb_LogType_Error, "Build failed");

    deb_cmd_free(&cmd);

    return 0;
}
