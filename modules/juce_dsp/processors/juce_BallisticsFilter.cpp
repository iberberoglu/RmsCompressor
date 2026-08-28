/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce::dsp
{

//==============================================================================
template <typename SampleType>
BallisticsFilter<SampleType>::BallisticsFilter()
{
    setAttackTime (attackTime);
    setReleaseTime (releaseTime);
}

template <typename SampleType>
void BallisticsFilter<SampleType>::setAttackTime (SampleType attackTimeMs)
{
    attackTime = attackTimeMs;
    cteAT = calculateLimitedCte (static_cast<SampleType> (attackTime), expFactorAttack);
}

template <typename SampleType>
void BallisticsFilter<SampleType>::setReleaseTime (SampleType releaseTimeMs)
{
    releaseTime = releaseTimeMs;
    cteRL = calculateLimitedCte (static_cast<SampleType> (releaseTime), expFactorRelease);
}

template <typename SampleType>
void BallisticsFilter<SampleType>::setLevelCalculationType (LevelCalculationType newLevelType)
{
    levelType = newLevelType;
    reset();
}

template <typename SampleType>
void BallisticsFilter<SampleType>::prepare (const ProcessSpec& spec)
{
    jassert (spec.sampleRate > 0);
    jassert (spec.numChannels > 0);

    sampleRate = spec.sampleRate;
    expFactorAttack  = attackCoefficient * MathConstants<double>::pi * 1000.0 / sampleRate;
    expFactorRelease  = releaseCoefficient * MathConstants<double>::pi * 1000.0 / sampleRate;

    setAttackTime  (attackTime);
    setReleaseTime (releaseTime);

    yold.resize (spec.numChannels);

    reset();
}

template <typename SampleType>
void BallisticsFilter<SampleType>::reset()
{
    reset (0);
}

template <typename SampleType>
void BallisticsFilter<SampleType>::reset (SampleType initialValue)
{
    for (auto& old : yold)
        old = initialValue;
}

template <typename SampleType>
SampleType BallisticsFilter<SampleType>::processSample (int channel, SampleType inputValue, float rmsLevel, bool peak, bool rms, double attackCo, double releaseCo)
{
    jassert (isPositiveAndBelow (channel, yold.size()));
    
    expFactorAttack  = attackCo * MathConstants<double>::pi * 1000.0 / sampleRate;
    expFactorRelease  = releaseCo * MathConstants<double>::pi * 1000.0 / sampleRate;
    
    cteAT = calculateLimitedCte (static_cast<SampleType> (attackTime), expFactorAttack);
    cteRL = calculateLimitedCte (static_cast<SampleType> (releaseTime), expFactorRelease);

    // RMS modu: zarf, pencerelenmiş RMS'in karesini takip eder; sonuçta karekök
    // alınarak lineer genliğe dönülür.
    if (rms)
    {
        rmsLevel *= rmsLevel;

        const SampleType cte = (rmsLevel > yold[(size_t) channel] ? cteAT : cteRL);

        const SampleType result = rmsLevel + cte * (yold[(size_t) channel] - rmsLevel);
        yold[(size_t) channel] = result;

        return std::sqrt (result);
    }

    // Peak modu: zarf, örneğin mutlak değerini takip eder.
    if (peak)
    {
        inputValue = std::abs (inputValue);

        const SampleType cte = (inputValue > yold[(size_t) channel] ? cteAT : cteRL);

        const SampleType result = inputValue + cte * (yold[(size_t) channel] - inputValue);
        yold[(size_t) channel] = result;

        return result;
    }

    // Arayüzdeki radio grubu ikisinden birinin daima seçili olmasını sağlıyor;
    // buraya düşmek bir programlama hatası demek. Değer döndürmeden çıkmak
    // tanımsız davranış olurdu, o yüzden girişi olduğu gibi geri ver.
    jassertfalse;
    return inputValue;
}

template <typename SampleType>
void BallisticsFilter<SampleType>::snapToZero() noexcept
{
    for (auto& old : yold)
        util::snapToZero (old);
}

template <typename SampleType>
SampleType BallisticsFilter<SampleType>::calculateLimitedCte (SampleType timeMs, double expFactor) const noexcept
{
    return timeMs < static_cast<SampleType> (1.0e-3) ? 0
                                                     : static_cast<SampleType> (std::exp (expFactor / timeMs));
}

//==============================================================================
template class BallisticsFilter<float>;
template class BallisticsFilter<double>;

} // namespace juce::dsp
