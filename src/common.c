#include "dcds/common.h"

const char *dcds_status_str(dcds_status s)
{
    switch (s) {
    case DCDS_OK:
        return "ok";
    case DCDS_ENOMEM:
        return "out of memory";
    case DCDS_ENOTFOUND:
        return "not found";
    case DCDS_ERANGE:
        return "out of range or empty";
    }
    return "unknown status";
}
