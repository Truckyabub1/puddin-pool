using System;

namespace PuddinPool.Character
{
    public enum CoachEmoteState
    {
        IDLE_WISE = 0,
        AIM_ADVICE = 1,
        GREAT_SHOT = 2,
        SCRATCH_CONSOLING = 3,
        HARD_BANK_CHEER = 4
    }

    public class CoachPuddinData
    {
        public CoachEmoteState CurrentEmote { get; private set; } = CoachEmoteState.IDLE_WISE;
        public string ActiveDialogue { get; private set; } = "Welcome to the felt, son. Line 'em up and let's see what you've got.";

        public Action<CoachEmoteState, string> OnStateChanged;

        public void SetEmote(CoachEmoteState state, string dialogue = null)
        {
            CurrentEmote = state;
            if (!string.IsNullOrEmpty(dialogue))
            {
                ActiveDialogue = dialogue;
            }
            OnStateChanged?.Invoke(CurrentEmote, ActiveDialogue);
        }

        // React dynamically to physics collision outcomes
        public void OnShotCompleted(bool wasScratch, int ballsPocketed, int cushionsHit, float cutAngleDeg)
        {
            if (wasScratch)
            {
                SetEmote(CoachEmoteState.SCRATCH_CONSOLING,
                    "Table’s forgiving, long as you learn where that cue ball wanted to go. Dust it off, son!");
            }
            else if (ballsPocketed > 0)
            {
                if (cushionsHit >= 2 || cutAngleDeg > 65.0f)
                {
                    SetEmote(CoachEmoteState.HARD_BANK_CHEER,
                        "Now that's geometry with respect for the cushion! Splendid bank shot, son!");
                }
                else
                {
                    SetEmote(CoachEmoteState.GREAT_SHOT,
                        "That's how you let the cue stick do the talking! Clean as a whistle.");
                }
            }
            else
            {
                SetEmote(CoachEmoteState.IDLE_WISE,
                    "Smooth stroke, quiet mind. The balls always know what you truly told them.");
            }
        }

        public void OnAimingState(float cutAngleDeg, float distanceMeters, bool isSnookered)
        {
            if (isSnookered)
            {
                SetEmote(CoachEmoteState.AIM_ADVICE,
                    "Think three rails ahead. Every wall is an open door if you hit it right.");
            }
            else if (distanceMeters > 1.2f && cutAngleDeg > 45.0f)
            {
                SetEmote(CoachEmoteState.AIM_ADVICE,
                    "Take your time, son! Plan your angles carefully, the shot reveals itself.");
            }
            else
            {
                SetEmote(CoachEmoteState.AIM_ADVICE,
                    "Soft hands on that cue. Guide it, don't force it.");
            }
        }
    }
}
