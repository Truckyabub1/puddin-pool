using System;
using System.Collections.Generic;

namespace PuddinPool.UI
{
    public struct EmoteSticker
    {
        public string Id;
        public string Title;
        public string Description;
        public string AssetPath;
    }

    public class EmoteDrawer
    {
        public bool IsDrawerOpen { get; private set; } = false;
        public List<EmoteSticker> AvailableStickers { get; private set; }

        public Action<EmoteSticker> OnEmoteDispatched;

        public EmoteDrawer()
        {
            AvailableStickers = new List<EmoteSticker>
            {
                new EmoteSticker
                {
                    Id = "puddin_knowing_nod",
                    Title = "Knowing Nod",
                    Description = "Coach Puddin nods knowingly with upright cue stick in hand.",
                    AssetPath = "Characters/CoachPuddin/Emotes/nodding_cue.png"
                },
                new EmoteSticker
                {
                    Id = "puddin_proud_9ball",
                    Title = "Golden 9-Ball",
                    Description = "Coach Puddin holds up the 9-ball with a proud, beaming grin.",
                    AssetPath = "Characters/CoachPuddin/Emotes/holding_9ball.png"
                },
                new EmoteSticker
                {
                    Id = "puddin_examine_angle",
                    Title = "Angle Check",
                    Description = "Coach Puddin adjusts his wire glasses, squinting down the rail.",
                    AssetPath = "Characters/CoachPuddin/Emotes/adjusting_glasses.png"
                }
            };
        }

        public void OpenDrawer() => IsDrawerOpen = true;
        public void CloseDrawer() => IsDrawerOpen = false;

        public void SelectEmote(string stickerId)
        {
            var sticker = AvailableStickers.Find(s => s.Id == stickerId);
            if (!string.IsNullOrEmpty(sticker.Id))
            {
                OnEmoteDispatched?.Invoke(sticker);
                CloseDrawer();
            }
        }
    }
}
