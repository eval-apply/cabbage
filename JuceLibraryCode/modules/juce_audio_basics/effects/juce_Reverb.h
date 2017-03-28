/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2016 - ROLI Ltd.

   Permission is granted to use this software under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license/

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH REGARD
   TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
   OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
   USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
   OF THIS SOFTWARE.

   -----------------------------------------------------------------------------

   To release a closed-source product which uses other parts of JUCE not
   licensed under the ISC terms, commercial licenses are available: visit
   www.juce.com for more information.

  ==============================================================================
*/

#pragma once


//==============================================================================
/**
    Performs a simple reverb effect on a stream of audio data.

    This is a simple stereo reverb, based on the technique and tunings used in FreeVerb.
    Use setSampleRate() to prepare it, and then call processStereo() or processMono() to
    apply the reverb to your audio data.

    @see ReverbAudioSource
*/
class Reverb
{
public:
    //==============================================================================
    Reverb()
    {
        setParameters (Parameters());
        setSampleRate (44100.0);
    }

    //==============================================================================
    /** Holds the parameters being used by a Reverb object. */
    struct Parameters
    {
        Parameters() noexcept
          : roomSize   (0.5f),
            damping    (0.5f),
            wetLevel   (0.33f),
            dryLevel   (0.4f),
            width      (1.0f),
            freezeMode (0)
        {}

        float roomSize;     /**< Room size, 0 to 1.0, where 1.0 is big, 0 is small. */
        float damping;      /**< Damping, 0 to 1.0, where 0 is not damped, 1.0 is fully damped. */
        float wetLevel;     /**< Wet level, 0 to 1.0 */
        float dryLevel;     /**< Dry level, 0 to 1.0 */
        float width;        /**< Reverb width, 0 to 1.0, where 1.0 is very wide. */
        float freezeMode;   /**< Freeze mode - values < 0.5 are "normal" mode, values > 0.5
                                 put the reverb into a continuous feedback loop. */
    };

    //==============================================================================
    /** Returns the reverb's current parameters. */
    const Parameters& getParameters() const noexcept    { return parameters; }

    /** Applies a new set of parameters to the reverb.
        Note that this doesn't attempt to lock the reverb, so if you call this in parallel with
        the process method, you may get artifacts.
    */
    void setParameters (const Parameters& newParams)
    {
        const float wetScaleFactor = 3.0f;
        const float dryScaleFactor = 2.0f;

        const float wet = newParams.wetLevel * wetScaleFactor;
        dryGain.setValue (newParams.dryLevel * dryScaleFactor);
        wetGain1.setValue (0.5f * wet * (1.0f + newParams.width));
        wetGain2.setValue (0.5f * wet * (1.0f - newParams.width));

        gain = isFrozen (newParams.freezeMode) ? 0.0f : 0.015f;
        parameters = newParams;
        updateDamping();
    }

    //==============================================================================
    /** Sets the sample rate that will be used for the reverb.
        You must call this before the process methods, in order to tell it the correct sample rate.
    */
    void setSampleRate (const double sampleRate)
    {
        jassert (sampleRate > 0);

        static const short combTunings[] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 }; // (at 44100Hz)
        static const short allPassTunings[] = { 556, 441, 341, 225 };
        const int stereoSpread = 23;
        const int intSampleRate = (int) sampleRate;

        for (int i = 0; i < numCombs; ++i)
        {
            comb[0][i].setSize ((intSampleRate * combTunings[i]) / 44100);
            comb[1][i].setSize ((intSampleRate * (combTunings[i] + stereoSpread)) / 44100);
        }

        for (int i = 0; i < numAllPasses; ++i)
        {
            allPass[0][i].setSize ((intSampleRate * allPassTunings[i]) / 44100);
            allPass[1][i].setSize ((intSampleRate * (allPassTunings[i] + stereoSpread)) / 44100);
        }

        const double smoothTime = 0.01;
        damping .reset (sampleRate, smoothTime);
        feedback.reset (sampleRate, smoothTime);
        dryGain .reset (sampleRate, smoothTime);
        wetGain1.reset (sampleRate, smoothTime);
        wetGain2.reset (sampleRate, smoothTime);
    }

    /** Clears the reverb's buffers. */
    void reset()
    {
        for (int j = 0; j < numChannels; ++j)
        {
            for (int i = 0; i < numCombs; ++i)
                comb[j][i].clear();

            for (int i = 0; i < numAllPasses; ++i)
                allPass[j][i].clear();
        }
    }

    //==============================================================================
    /** Applies the reverb to two stereo channels of audio data. */
    void processStereo (float* const left, float* const right, const int numSamples) noexcept
    {
        jassert (left != nullptr && right != nullptr);

        for (int i = 0; i < numSamples; ++i)
        {
            const float input = (left[i] + right[i]) * gain;
            float outL = 0, outR = 0;

            const float damp    = damping.getNextValue();
            const float feedbck = feedback.getNextValue();

            for (int j = 0; j < numCombs; ++j)  // accumulate the comb filters in parallel
            {
                outL += comb[0][j].process (input, damp, feedbck);
                outR += comb[1][j].process (input, damp, feedbck);
            }

            for (int j = 0; j < numAllPasses; ++j)  // run the allpass filters in series
            {
                outL = allPass[0][j].process (outL);
                outR = allPass[1][j].process (outR);
            }

            const float dry  = dryGain.getNextValue();
            const float wet1 = wetGain1.getNextValue();
            const float wet2 = wetGain2.getNextValue();

            left[i]  = outL * wet1 + outR * wet2 + left[i]  * dry;
            right[i] = outR * wet1 + outL * wet2 + right[i] * dry;
        }
    }

    /** Applies the reverb to a single mono channel of audio data. */
    void processMono (float* const samples, const int numSamples) noexcept
    {
        jassert (samples != nullptr);

        for (int i = 0; i < numSamples; ++i)
        {
            const float input = samples[i] * gain;
            float output = 0;

            const float damp    = damping.getNextValue();
            const float feedbck = feedback.getNextValue();

            for (int j = 0; j < numCombs; ++j)  // accumulate the comb filters in parallel
                output += comb[0][j].process (input, damp, feedbck);

            for (int j = 0; j < numAllPasses; ++j)  // run the allpass filters in series
                output = allPass[0][j].process (output);

            const float dry  = dryGain.getNextValue();
            const float wet1 = wetGain1.getNextValue();

            samples[i] = output * wet1 + samples[i] * dry;
        }
    }

private:
    //==============================================================================
    static bool isFrozen (const float freezeMode) noexcept  { return freezeMode >= 0.5f; }

    void updateDamping() noexcept
    {
        const float roomScaleFactor = 0.28f;
        const float roomOffset = 0.7f;
        const float dampScaleFactor = 0.4f;

        if (isFrozen (parameters.freezeMode))
            setDamping (0.0f, 1.0f);
        else
            setDamping (parameters.damping * dampScaleFactor,
                        parameters.roomSize * roomScaleFactor + roomOffset);
    }

    void setDamping (const float dampingToUse, const float roomSizeToUse) noexcept
    {
        damping.setValue (dampingToUse);
        feedback.setValue (roomSizeToUse);
    }

    //==============================================================================
    class CombFilter
    {
    public:
        CombFilter() noexcept   : bufferSize (0), bufferIndex (0), last (0)  {}

        void setSize (const int size)
        {
            if (size != bufferSize)
            {
                bufferIndex = 0;
                buffer.malloc ((size_t) size);
                bufferSize = size;
            }

            clear();
        }

        void clear() noexcept
        {
            last = 0;
            buffer.clear ((size_t) bufferSize);
        }

        float process (const float input, const float damp, const float feedbackLevel) noexcept
        {
            const float output = buffer[bufferIndex];
            last = (output * (1.0f - damp)) + (last * damp);
            JUCE_UNDENORMALISE (last);

            float temp = input + (last * feedbackLevel);
            JUCE_UNDENORMALISE (temp);
            buffer[bufferIndex] = temp;
            bufferIndex = (bufferIndex + 1) % bufferSize;
            return output;
        }

    private:
        HeapBlock<float> buffer;
        int bufferSize, bufferIndex;
        float last;

        JUCE_DECLARE_NON_COPYABLE (CombFilter)
    };

    //==============================================================================
    class AllPassFilter
    {
    public:
        AllPassFilter() noexcept  : bufferSize (0), bufferIndex (0) {}

        void setSize (const int size)
        {
            if (size != bufferSize)
            {
                bufferIndex = 0;
                buffer.malloc ((size_t) size);
                bufferSize = size;
            }

            clear();
        }

        void clear() noexcept
        {
            buffer.clear ((size_t) bufferSize);
        }

        float process (const float input) noexcept
        {
            const float bufferedValue = buffer [bufferIndex];
            float temp = input + (bufferedValue * 0.5f);
            JUCE_UNDENORMALISE (temp);
            buffer [bufferIndex] = temp;
            bufferIndex = (bufferIndex + 1) % bufferSize;
            return bufferedValue - input;
        }

    private:
        HeapBlock<float> buffer;
        int bufferSize, bufferIndex;

        JUCE_DECLARE_NON_COPYABLE (AllPassFilter)
    };

    //==============================================================================
    enum { numCombs = 8, numAllPasses = 4, numChannels = 2 };

    Parameters parameters;
    float gain;

    CombFilter comb [numChannels][numCombs];
    AllPassFilter allPass [numChannels][numAllPasses];

    LinearSmoothedValue<float> damping, feedback, dryGain, wetGain1, wetGain2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reverb)
};
