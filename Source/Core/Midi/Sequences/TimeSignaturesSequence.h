/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "MidiSequence.h"
#include "TimeSignatureEvent.h"

class TimeSignaturesSequence final : public MidiSequence
{
public:

    explicit TimeSignaturesSequence(MidiTrack &track,
        ProjectEventDispatcher &dispatcher) noexcept;

    //===------------------------------------------------------------------===//
    // Import/export
    //===------------------------------------------------------------------===//

    void importMidi(const MidiMessageSequence &sequence, short timeFormat) override;

    //===------------------------------------------------------------------===//
    // Undoable track editing
    //===------------------------------------------------------------------===//
    
    MidiEvent *insert(const TimeSignatureEvent &signatureToCopy, bool undoable);
    bool remove(const TimeSignatureEvent &signature, bool undoable);
    bool change(const TimeSignatureEvent &signature,
        const TimeSignatureEvent &newSignature,
        bool undoable);

    //===------------------------------------------------------------------===//
    // Callbacks
    //===------------------------------------------------------------------===//

    Function<void(const String &text)> getEventChangeCallback(const TimeSignatureEvent &event);

    //===------------------------------------------------------------------===//
    // Serializable
    //===------------------------------------------------------------------===//

    SerializedData serialize() const override;
    void deserialize(const SerializedData &data) override;
    void reset() override;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeSignaturesSequence)
    JUCE_DECLARE_WEAK_REFERENCEABLE(TimeSignaturesSequence)
};
