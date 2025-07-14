#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#undef min
#undef max

extern int main(int argc, char** argv);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    MessageBoxA(NULL, "WinMain() called", "Dynamix", MB_OK);
    
    MessageBoxA(NULL, "About to call main()", "Dynamix", MB_OK);
    
    int result = main(__argc, __argv);
    
    char msg[256];
    sprintf(msg, "main() returned: %d", result);
    MessageBoxA(NULL, msg, "Dynamix", MB_OK);
    return result;
}
#endif 