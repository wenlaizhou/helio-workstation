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

#include "Common.h"
#include "TimeSignatureDialog.h"
#include "MidiTrackActions.h"
#include "Config.h"
#include "CommandIDs.h"

TimeSignatureDialog::TimeSignatureDialog(Component &owner,
    WeakReference<UndoStack> undoStack,
    WeakReference<MidiTrack> targetTrack,
    WeakReference<TimeSignaturesSequence> targetSequence,
    const TimeSignatureEvent &editedEvent, bool shouldAddNewEvent) :
    undoStack(undoStack),
    targetTrack(targetTrack),
    targetSequence(targetSequence),
    originalEvent(editedEvent),
    ownerComponent(owner),
    defaultMeters(App::Config().getMeters()->getAll()),
    mode(shouldAddNewEvent ? Mode::AddTimelineTimeSignature :
        (targetSequence != nullptr ? Mode::EditTimelineTimeSignature : Mode::EditTrackTimeSignature))
{
    jassert(this->originalEvent.isValid() || this->targetTrack != nullptr);
    jassert(this->targetSequence != nullptr || this->targetTrack != nullptr);
    jassert(this->targetTrack != nullptr || this->originalEvent.getSequence() != nullptr);

    this->messageLabel = make<Label>();
    this->addAndMakeVisible(this->messageLabel.get());
    this->messageLabel->setFont(Globals::UI::Fonts::L);
    this->messageLabel->setJustificationType(Justification::centred);
    this->messageLabel->setInterceptsMouseClicks(false, false);

    this->removeEventButton = make<TextButton>();
    this->addAndMakeVisible(this->removeEventButton.get());
    this->removeEventButton->onClick = [this]()
    {
        switch (this->mode)
        {
        case Mode::EditTrackTimeSignature:
        case Mode::EditTimelineTimeSignature:
            this->removeTimeSignature();
            this->dismiss();
            break;
        case Mode::AddTimelineTimeSignature:
            this->undoAndDismiss();
            break;
        }
    };

    this->okButton = make<TextButton>();
    this->addAndMakeVisible(this->okButton.get());
    this->okButton->onClick = [this]()
    {
        if (this->textEditor->getText().isNotEmpty())
        {
            this->dismiss();
        }
    };

    this->textEditor = make<TextEditor>();
    this->addAndMakeVisible(this->textEditor.get());
    this->textEditor->setFont(Globals::UI::Fonts::L);

    const auto onLostFocus = [this]()
    {
        this->updateOkButtonState();

        auto *focusedComponent = Component::getCurrentlyFocusedComponent();

        if (nullptr != dynamic_cast<TextEditor *>(focusedComponent) &&
            this->textEditor.get() != focusedComponent)
        {
            return; // other editor is focused
        }

        if (this->textEditor->getText().isNotEmpty() &&
            focusedComponent != this->okButton.get() &&
            focusedComponent != this->removeEventButton.get())
        {
            this->dismiss(); // apply on return key
        }
        else
        {
            this->textEditor->grabKeyboardFocus();
        }
    };

    this->textEditor->onReturnKey = onLostFocus;
    this->textEditor->onFocusLost = onLostFocus;

    this->textEditor->onTextChange = [this]()
    {
        this->updateOkButtonState();

        const auto meterString = this->textEditor->getText();
        if (meterString.isEmpty())
        {
            return;
        }
        
        int numerator;
        int denominator;
        Meter::parseString(meterString, numerator, denominator);

        const auto newEvent = this->originalEvent
            .withNumerator(numerator)
            .withDenominator(denominator);

        this->sendEventChange(newEvent);
    };

    this->textEditor->onEscapeKey = [this]()
    {
        this->undoAndDismiss();
    };

    MenuPanel::Menu menu;
    for (int i = 0; i < this->defaultMeters.size(); ++i)
    {
        const auto meter = this->defaultMeters[i];
        menu.add(MenuItem::item(Icons::empty,
            CommandIDs::SelectTimeSignature + i, meter->getLocalizedName()));
    }

    this->presetsCombo = make<MobileComboBox::Container>();
    this->addAndMakeVisible(this->presetsCombo.get());
    this->presetsCombo->initWith(this->textEditor.get(), menu);

    if (this->mode == Mode::EditTrackTimeSignature && !this->originalEvent.isValid())
    {
        jassert(this->targetTrack != nullptr);

        // this is the case when we're adding a time signature to a track,
        // so let's try to suggest the one that seems to fit by bar length:
        auto newMeter = this->defaultMeters.getFirst();
        for (int i = this->defaultMeters.size(); i --> 0 ;)
        {
            if (fmodf(this->targetTrack->getSequence()->getLengthInBeats(),
                this->defaultMeters[i]->getBarLengthInBeats()) == 0.f)
            {
                newMeter = this->defaultMeters[i];
                break;
            }
        }

        this->sendEventChange(this->originalEvent
            .withNumerator(newMeter->getNumerator())
            .withDenominator(newMeter->getDenominator()));
    }

    if (this->mode == Mode::AddTimelineTimeSignature)
    {
        jassert(this->targetSequence != nullptr);
        this->undoStack->beginNewTransaction();
        this->targetSequence->insert(this->originalEvent, true);

        this->messageLabel->setText(TRANS(I18n::Dialog::timeSignatureAddCaption), dontSendNotification);
        this->removeEventButton->setButtonText(TRANS(I18n::Dialog::cancel));
        this->okButton->setButtonText(TRANS(I18n::Dialog::add));
    }
    else
    {
        this->messageLabel->setText(TRANS(I18n::Dialog::timeSignatureEditCaption), dontSendNotification);
        this->removeEventButton->setButtonText(TRANS(I18n::Dialog::delete_));
        this->okButton->setButtonText(TRANS(I18n::Dialog::apply));
    }

    this->textEditor->setText(this->originalEvent.toString(), dontSendNotification);
    
    this->setSize(370, 185);
    this->updatePosition();
    this->updateOkButtonState();
}

TimeSignatureDialog::~TimeSignatureDialog() = default;

void TimeSignatureDialog::resized()
{
    this->messageLabel->setBounds(this->getCaptionBounds());
    this->presetsCombo->setBounds(this->getContentBounds(0.5f));

    const auto buttonsBounds(this->getButtonsBounds());
    const auto buttonWidth = buttonsBounds.getWidth() / 2;

    this->okButton->setBounds(buttonsBounds.withTrimmedLeft(buttonWidth));
    this->removeEventButton->setBounds(buttonsBounds.withTrimmedRight(buttonWidth + 1));

    this->textEditor->setBounds(this->getRowBounds(0.5f, DialogBase::textEditorHeight));
}

void TimeSignatureDialog::parentHierarchyChanged()
{
    this->updatePosition();
}

void TimeSignatureDialog::parentSizeChanged()
{
    this->updatePosition();
}

void TimeSignatureDialog::handleCommandMessage(int commandId)
{
    if (commandId == CommandIDs::DismissModalDialogAsync)
    {
        this->undoAndDismiss();
    }
    else
    {
        const int targetIndex = commandId - CommandIDs::SelectTimeSignature;
        if (targetIndex >= 0 && targetIndex < this->defaultMeters.size())
        {
            const auto &meter = this->defaultMeters[targetIndex];
            this->textEditor->grabKeyboardFocus();
            this->textEditor->setText(meter->getTimeAsString(), true);
        }
    }
}

void TimeSignatureDialog::inputAttemptWhenModal()
{
    this->postCommandMessage(CommandIDs::DismissModalDialogAsync);
}

void TimeSignatureDialog::updateOkButtonState()
{
    this->okButton->setEnabled(this->textEditor->getText().isNotEmpty());
}

void TimeSignatureDialog::sendEventChange(const TimeSignatureEvent &newEvent)
{
    switch (this->mode)
    {
    case Mode::EditTrackTimeSignature:
        jassert(this->targetTrack != nullptr);
        if (this->hasMadeChanges)
        {
            this->undoStack->undo();
        }

        this->undoStack->beginNewTransaction();
        this->targetTrack->setTimeSignatureOverride(newEvent, true, sendNotification);
        this->originalEvent = newEvent;
        this->hasMadeChanges = true;
        break;
    case Mode::EditTimelineTimeSignature:
        jassert(this->targetSequence != nullptr);
        if (this->hasMadeChanges)
        {
            this->undoStack->undo();
        }

        this->undoStack->beginNewTransaction();
        this->targetSequence->change(this->originalEvent, newEvent, true);
        this->hasMadeChanges = true;
        break;
    case Mode::AddTimelineTimeSignature:
        jassert(this->targetSequence != nullptr);
        this->undoStack->undo();
        this->targetSequence->insert(newEvent, true);
        this->originalEvent = newEvent;
        break;
    }
}

void TimeSignatureDialog::removeTimeSignature()
{
    switch (this->mode)
    {
    case Mode::EditTrackTimeSignature:
        jassert(this->targetTrack != nullptr);
        if (this->hasMadeChanges)
        {
            this->undoStack->undo();
        }

        this->undoStack->beginNewTransaction();
        this->targetTrack->setTimeSignatureOverride({}, true, sendNotification);
        this->hasMadeChanges = true;
        break;
    case Mode::EditTimelineTimeSignature:
        jassert(this->targetSequence != nullptr);
        if (this->hasMadeChanges)
        {
            this->undoStack->undo();
        }

        this->undoStack->beginNewTransaction();
        this->targetSequence->remove(this->originalEvent, true);
        this->hasMadeChanges = true;
        break;
    case Mode::AddTimelineTimeSignature:
        jassert(this->targetSequence != nullptr);
        this->undoStack->undo();
        break;
    }
}

void TimeSignatureDialog::undoAndDismiss()
{
    switch (this->mode)
    {
    case Mode::EditTrackTimeSignature:
    case Mode::EditTimelineTimeSignature:
        if (this->hasMadeChanges)
        {
            this->undoStack->undo();
        }
        break;
    case Mode::AddTimelineTimeSignature:
        // undo anyway to cancel new event insertion at the start:
        this->undoStack->undo();
        break;
    }

    this->dismiss();
}

UniquePointer<Component> TimeSignatureDialog::editingDialog(Component &owner,
    WeakReference<UndoStack> undoStack, const TimeSignatureEvent &event)
{
    if (auto *sequence = static_cast<TimeSignaturesSequence *>(event.getSequence()))
    {
        return make<TimeSignatureDialog>(owner, undoStack,
            nullptr, sequence, event, false);
    }
    else
    {
        jassert(event.getTrack() != nullptr);
        return make<TimeSignatureDialog>(owner, undoStack,
            event.getTrack(), nullptr, event, false);
    }
}

UniquePointer<Component> TimeSignatureDialog::addingDialog(Component &owner,
    WeakReference<UndoStack> undoStack,
    WeakReference<TimeSignaturesSequence> targetSequence, float targetBeat)
{
    return make<TimeSignatureDialog>(owner, undoStack, nullptr,
        targetSequence, TimeSignatureEvent(targetSequence.get(), targetBeat), true);
}
