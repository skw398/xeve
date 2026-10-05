#include <xeve.h>

int main(void)
{
    XEVE_PARAM param;
    return xeve_param_default(&param) == XEVE_OK ? 0 : 1;
}
