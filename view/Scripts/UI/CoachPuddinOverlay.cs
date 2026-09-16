using System;
using System.Collections;
using System.Collections.Generic;
using PuddinPool.Character;
using PuddinPool.Native;

namespace PuddinPool.UI
{
    public class CoachPuddinOverlay
    {
        public bool IsVisible { get; set; } = true;
        public CoachEmoteState DisplayedEmote { get; private set; } = CoachEmoteState.IDLE_WISE;
        public string DisplayedText { get; private set; } = "";
        public string TargetFullText { get; private set; } = "";

        public bool IsTyping { get; private set; } = false;
        public bool IsHintActive { get; private set; } = false;

        public Action<string> OnVoiceBlipTriggered; // "pop/mumble" sound trigger
        public Action<int, NativeVector2> OnHintPocketHighlighted; // pocketIndex, tangentVector

        private float _typewriterTimer = 0.0f;
        private int _typewriterIndex = 0;
        private const float CharInterval = 0.035f; // 35ms per character

        public void SetDialogue(string text, CoachEmoteState emote = CoachEmoteState.IDLE_WISE)
        {
            DisplayedEmote = emote;
            TargetFullText = text ?? "";
            DisplayedText = "";
            _typewriterIndex = 0;
            _typewriterTimer = 0.0f;
            IsTyping = true;
        }

        public void Update(float deltaTime)
        {
            if (!IsTyping) return;

            _typewriterTimer += deltaTime;
            if (_typewriterTimer >= CharInterval)
            {
                _typewriterTimer = 0.0f;
                if (_typewriterIndex < TargetFullText.Length)
                {
                    char c = TargetFullText[_typewriterIndex++];
                    DisplayedText += c;

                    // Trigger soft warm audio voice blip on non-whitespace characters
                    if (!char.IsWhiteSpace(c) && (_typewriterIndex % 2 == 0))
                    {
                        OnVoiceBlipTriggered?.Invoke("puddin_voice_mumble_pop");
                    }
                }
                else
                {
                    IsTyping = false;
                }
            }
        }

        // "Ask Elder Puddin" hint button in practice mode
        public void OnAskElderPuddinClicked(TrajectoryBuffer currentTrajectory)
        {
            IsHintActive = true;
            SetDialogue("Watch the tangent off that cut angle, son. Let's send it toward the side pocket.", CoachEmoteState.AIM_ADVICE);

            // Compute recommended pocket direction
            if (currentTrajectory.target_count > 1)
            {
                NativeVector2 targetDir = new NativeVector2(
                    currentTrajectory.target_points[1].x - currentTrajectory.target_points[0].x,
                    currentTrajectory.target_points[1].y - currentTrajectory.target_points[0].y
                );
                OnHintPocketHighlighted?.Invoke(1, targetDir);
            }
        }

        public void DismissHint()
        {
            IsHintActive = false;
        }
    }
}
