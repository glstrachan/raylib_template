#ifdef _WIN32
#include <Windows.h>

extern int entry_main(void);
int WinMain(
             HINSTANCE hInstance,
   HINSTANCE hPrevInstance,
             LPSTR     lpCmdLine,
             int       nShowCmd
) {
    return entry_main();
}
#endif
