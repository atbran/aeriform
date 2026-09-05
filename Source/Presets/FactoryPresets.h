#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <utility>

namespace aeriform
{
/** A factory preset is a name, a category and a sparse list of parameter values
    (DSP units, i.e. the same units the parameters display). Parameters that are
    not listed take their default value. */
struct FactoryPreset
{
    juce::String name;
    juce::String category;
    std::vector<std::pair<juce::String, float>> values;
};

const std::vector<FactoryPreset>& factoryPresets();
} // namespace aeriform
