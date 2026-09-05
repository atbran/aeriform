#include "ModMatrix.h"

// ModMatrix is header-only; this translation unit exists so the module has a
// compiled home for future non-inline helpers.
namespace aeriform::dsp
{
static_assert ((int) ModSource::Count == 15, "Mod source list changed: update choices::modSources()");
static_assert ((int) ModDest::Count == 24, "Mod destination list changed: update choices::modDests()");
}
