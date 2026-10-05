#ifdef _MSC_VER
#pragma warning(push, 0)
// Upstream uses portable C functions rather than Microsoft-specific secure CRT APIs.
#pragma warning(disable : 4996)
#endif
#define CGLTF_IMPLEMENTATION
#include "ThirdParty/cgltf/cgltf.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
