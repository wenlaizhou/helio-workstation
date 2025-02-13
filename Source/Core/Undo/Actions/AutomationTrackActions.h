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

class MidiTrackSource;

#include "AutomationSequence.h"
#include "UndoAction.h"
#include "TreeNode.h"

//===----------------------------------------------------------------------===//
// Insert
//===----------------------------------------------------------------------===//

class AutomationTrackInsertAction final : public UndoAction
{
public:

    AutomationTrackInsertAction(MidiTrackSource &source,
        WeakReference<TreeNode> parentTreeItem) noexcept;
    
    AutomationTrackInsertAction(MidiTrackSource &source,
        WeakReference<TreeNode> parentTreeItem,
        SerializedData serializedState,
        const String &trackName) noexcept;

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    
    SerializedData serialize() const override;
    void deserialize(const SerializedData &data) override;
    void reset() override;
    
private:

    WeakReference<TreeNode> parentTreeItem;

    String trackId;
    String trackName;

    SerializedData trackState;
    
    JUCE_DECLARE_NON_COPYABLE(AutomationTrackInsertAction)
};


//===----------------------------------------------------------------------===//
// Remove
//===----------------------------------------------------------------------===//

class AutomationTrackRemoveAction final : public UndoAction
{
public:

    AutomationTrackRemoveAction(MidiTrackSource &source,
        WeakReference<TreeNode> parentTreeItem) noexcept;
    
    AutomationTrackRemoveAction(MidiTrackSource &source,
        WeakReference<TreeNode> parentTreeItem,
        const String &trackId) noexcept;

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;

    SerializedData serialize() const override;
    void deserialize(const SerializedData &data) override;
    void reset() override;
    
private:

    WeakReference<TreeNode> parentTreeItem;

    String trackId;
    int numEvents = 0;
    
    SerializedData serializedTreeItem;
    String trackName;
    
    JUCE_DECLARE_NON_COPYABLE(AutomationTrackRemoveAction)
};
