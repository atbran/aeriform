#include "ModMatrix.h"

// ModMatrix is header-only; this translation unit exists so the module has a
// compiled home for future non-inline helpers.
namespace aeriform::dsp
{
static_assert ((int) ModSource::Count == 30, "Mod source list changed: update choices::modSources()");
static_assert ((int) ModDest::Count == 54, "Mod destination list changed: update choices::modDests()");
}
