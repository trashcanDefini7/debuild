#define DEBUILD_IMPLEMENTATION
#include "debuild.h"

int main()
{
    deb_init();

    Deb_Cmd cmd = {0};
    
    deb_cmd_append(&cmd, "cc", "-w", "../../src/debuild.c", "-o", "../target/debuild.new");

    if (deb_cmd_execute(&cmd))
        deb_log_info("Build success");
    else
        deb_log_error("Build failed");

    return 0;
}
